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

#ifndef _LX_KIT__MEMORY_H_
#define _LX_KIT__MEMORY_H_

#include <base/allocator_avl.h>
#include <base/cache.h>
#include <base/env.h>
#include <base/heap.h>
#include <lx_kit/byte_range.h>
#include <lx_kit/map.h>

namespace Dma { class Connection; }

namespace Lx_kit {
	using namespace Genode;
	class Mem_map;
	class Mem_allocator;
	class Mem_external;
}


class Lx_kit::Mem_allocator
{
	public:

		struct Buffer
		{
			virtual ~Buffer() {}

			virtual size_t bus_addr()  const   = 0;
			virtual size_t size()      const   = 0;
			virtual size_t virt_addr() const   = 0;
			virtual Dataspace_capability cap() = 0;
		};

	private:

		friend class Mem_map;
		friend class Mem_external;

		struct Buffer_info
		{
			struct Key { addr_t addr; } key;

			Buffer &buffer;

			size_t size() const { return buffer.size(); }

			bool higher(Key const other_key) const
			{
				return key.addr > other_key.addr;
			}

			struct Query_range
			{
				addr_t addr;
				size_t size;

				bool matches(Buffer_info const &bi) const
				{
					Lx_kit::Byte_range buf_range { bi.key.addr, bi.size() };
					Lx_kit::Byte_range range     { addr, size };

					return buf_range.intersects(range);
				}

				Key key() const { return Key { addr }; }
			};

			struct Query_addr : Query_range
			{
				Query_addr(void const * addr)
				: Query_range{(addr_t)addr, 1} { }
			};
		};

		Env                  &_env;
		Heap                 &_heap;
		Dma::Connection      &_dma;
		Mem_map              &_map;
		Cache                 _cache_attr;
		Allocator_avl         _mem         { &_heap };

	public:

		Mem_allocator(Env              &env,
		              Heap             &heap,
		              Dma::Connection  &dma,
		              Mem_map          &map,
		              Cache             cache_attr);

		Buffer              &alloc_buffer(size_t size);
		void                 free_buffer(void *addr);
		Dataspace_capability attached_dataspace_cap(void *addr);

		void * alloc(size_t size, size_t align,
		             void (*new_range_cb)(void const *virt_addr, unsigned long size));
		size_t size(const void * ptr);
		bool   free(const void * ptr);
};


class Lx_kit::Mem_external
{
	public:

		struct Buffer : Mem_allocator::Buffer
		{
			size_t const _bus_addr;
			size_t const _size;
			size_t const _virt_addr;

			Buffer(size_t bus_addr, size_t size, size_t virt_addr)
			:
				_bus_addr((size_t)bus_addr), _size(size),
				_virt_addr((size_t)virt_addr) {}

			size_t bus_addr() const override {
				return _bus_addr; }

			size_t size() const override {
				return _size; }

			size_t virt_addr() const override {
				return _virt_addr; }

			Dataspace_capability cap() override {
				return Dataspace_capability(); }
		};

	private:

		Heap    &_heap;
		Mem_map &_map;

	public:

		Mem_external(Heap &heap, Mem_map &map)
		: _heap(heap), _map(map) {}

		void add(void *bus_addr, size_t size, void *virt_addr,
		         void (*new_range)(void const *virt_addr, unsigned long size));
		void remove(void * virt_addr,
		            void (*del_range)(void const *virt_addr, unsigned long size));
};


class Lx_kit::Mem_map
{
	private:

		friend class Mem_allocator;
		friend class Mem_external;

		using Buffer = Mem_allocator::Buffer;
		using Buffer_info = Mem_allocator::Buffer_info;

		Heap &_heap;

		Map<Buffer_info> _virt_to_dma { _heap };
		Map<Buffer_info> _dma_to_virt { _heap };

	public:

		Mem_map(Heap &heap) : _heap(heap) {}

		Dataspace_capability attached_dataspace_cap(void *addr);

		addr_t dma_addr(void * addr);
		addr_t virt_addr(void * dma_addr);
		addr_t virt_region_start(void * virt_addr);
};

#endif /* _LX_KIT__MEMORY_H_ */
