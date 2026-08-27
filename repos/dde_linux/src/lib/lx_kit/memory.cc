/*
 * \brief  Lx_kit memory allocation backend
 * \author Stefan Kalkowski
 * \author Christian Helmuth
 * \date   2021-03-25
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

/* Genode includes */
#include <base/log.h>
#include <util/touch.h>

/* local includes */
#include <lx_kit/memory.h>
#include <lx_kit/map.h>
#include <lx_kit/byte_range.h>


/**********************************
 ** Mem_allocator implementation **
 **********************************/

void Lx_kit::Mem_allocator::free_buffer(void * addr)
{
	using Query_addr = Buffer_info::Query_addr;

	_map._virt_to_dma.apply(Query_addr(addr),
		[&] (Buffer_info const &info) {
			void const * virt_addr = (void const *)info.buffer.virt_addr();
			void const * bus_addr  = (void const *)info.buffer.bus_addr();

			_map._virt_to_dma.remove(Query_addr(virt_addr));
			_map._dma_to_virt.remove(Query_addr(bus_addr));

			destroy(_heap, &info.buffer);
		},
		[&] {
			warning(__func__, ": no memory buffer for addr: ", addr, " found");
		});
}


void * Lx_kit::Mem_allocator::alloc(size_t const size, size_t const align_bytes,
                                    void (*new_range_cb)(void const *, unsigned long))
{
	if (!size)
		return nullptr;

	auto cleared_allocation = [] (void * const ptr, size_t const size) {
		bzero(ptr, size);
		return ptr;
	};

	Align const align { .log2 = log2(align_bytes, 0u) };

	return _mem.alloc_aligned(size, align).convert<void *>(

		[&] (Allocator::Allocation &a) {
			a.deallocate = false;
			return cleared_allocation(a.ptr, size); },

		[&] (Alloc_error) {

			/*
			 * Restrict the minimum buffer size to avoid the creation of
			 * a separate dataspaces for tiny allocations.
			 */
			size_t const min_buffer_size = 256*1024;

			/*
			 * Allocate one excess byte that is not officially registered at
			 * the '_mem' ranges. This way, two virtual consecutive ranges
			 * (that must be assumed to belong to non-contiguous physical
			 * ranges) can never be merged when freeing an allocation. Such
			 * a merge would violate the assumption that a both the virtual
			 * and physical addresses of a multi-page allocation are always
			 * contiguous.
			 */
			Buffer &buffer = alloc_buffer(max(size + 1, min_buffer_size));

			if (_mem.add_range(buffer.virt_addr(), buffer.size() - 1).failed())
				warning("Lx_kit::Mem_allocator unable to extend virtual allocator");

			/* re-try allocation */
			void * const virt_addr = _mem.alloc_aligned(size, align).convert<void *>(

				[&] (Allocator::Allocation &a) {
					a.deallocate = false;
					return cleared_allocation(a.ptr, size); },

				[&] (Alloc_error) -> void * {
					error("memory allocation failed for ", size, " align ", align_bytes);
					return nullptr; }
			);

			if (virt_addr)
				new_range_cb((void *)buffer.virt_addr(), buffer.size() - 1);

			return virt_addr;
		}
	);
}


bool Lx_kit::Mem_allocator::free(const void * ptr)
{
	if (!_mem.valid_addr((addr_t)ptr))
		return false;

	using Size_at_error = Allocator_avl::Size_at_error;

	_mem.size_at(ptr).with_result(
		[&] (size_t)        { _mem.free(const_cast<void*>(ptr)); },
		[ ] (Size_at_error) {                                    });

	return true;
}


Genode::size_t Lx_kit::Mem_allocator::size(const void * ptr)
{
	if (!ptr) return 0;

	using Size_at_error = Allocator_avl::Size_at_error;

	return _mem.size_at(ptr).convert<size_t>([ ] (size_t s)      { return s;  },
	                                         [ ] (Size_at_error) { return 0U; });
}


Lx_kit::Mem_allocator::Mem_allocator(Genode::Env     &env,
                                     Heap            &heap,
                                     Dma::Connection &dma,
                                     Mem_map         &map,
                                     Cache            cache_attr)
: _env(env), _heap(heap), _dma(dma), _map(map), _cache_attr(cache_attr) {}


/*********************************
 ** Mem_external implementation **
 *********************************/

void Lx_kit::Mem_external::add(void *bus_addr, size_t size, void *virt_addr,
                               void (*new_range)(void const *, unsigned long))
{
	size = align_addr(size, AT_PAGE);

	Buffer &buffer =
		*new (_heap) Buffer((size_t)bus_addr, size, (size_t)virt_addr);

	/* map eager by touching all pages once */
	for (size_t sz = 0; sz < size; sz += 4096) {
		touch_read((unsigned char const volatile*)(buffer.virt_addr() + sz)); }

	_map._virt_to_dma.insert(buffer.virt_addr(), buffer);
	_map._dma_to_virt.insert(buffer.bus_addr(),  buffer);

	new_range(virt_addr, size);
}


void Lx_kit::Mem_external::remove(void * addr,
                                  void (*del_range)(void const *, unsigned long))
{
	using Query_addr = Mem_allocator::Buffer_info::Query_addr;

	_map._virt_to_dma.apply(Query_addr(addr),
		[&] (Mem_allocator::Buffer_info const &info) {

			void const * virt_addr = (void const *)info.buffer.virt_addr();
			void const * bus_addr  = (void const *)info.buffer.bus_addr();

			del_range(virt_addr, info.buffer.size());

			_map._virt_to_dma.remove(Query_addr(virt_addr));
			_map._dma_to_virt.remove(Query_addr(bus_addr));

			destroy(_heap, &info.buffer);
		},
		[&] {
			warning(__func__, ": no memory buffer for addr: ", addr, " found");
		});
}


/****************************
 ** Mem_map implementation **
 ****************************/

Genode::Dataspace_capability Lx_kit::Mem_map::attached_dataspace_cap(void * addr)
{
	return _virt_to_dma.apply(Buffer_info::Query_addr(addr),
		[&] (Buffer_info const &info) { return info.buffer.cap(); },
		[] { return Genode::Dataspace_capability(); });
}


Genode::addr_t Lx_kit::Mem_map::dma_addr(void * addr)
{
	return _virt_to_dma.apply(Buffer_info::Query_addr(addr),
		[&] (Buffer_info const &info) {
			addr_t const offset = (addr_t)addr - info.buffer.virt_addr();
			return info.buffer.bus_addr() + offset;
		},
		[] { return 0UL; });
}


Genode::addr_t Lx_kit::Mem_map::virt_addr(void * bus_addr)
{
	return _dma_to_virt.apply(Buffer_info::Query_addr(bus_addr),
		[&] (Buffer_info const &info) {
			addr_t const offset = (addr_t)bus_addr - info.buffer.bus_addr();
			return info.buffer.virt_addr() + offset;
		},
		[] { return 0UL; });
}


Genode::addr_t Lx_kit::Mem_map::virt_region_start(void * virt_addr)
{
	return _virt_to_dma.apply(Buffer_info::Query_addr(virt_addr),
		[&] (Buffer_info const &info) {
			return info.buffer.virt_addr();
		},
		[] { return 0UL; });
}
