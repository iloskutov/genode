/*
 * \brief  Syntax-agnostic API for parsing structured textual data
 * \author Norman Feske
 * \date   2025-06-17
 */

/*
 * Copyright (C) 2025 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__NODE_H_
#define _INCLUDE__BASE__NODE_H_

#include <util/hid.h>
#include <base/memory.h>

namespace Genode {
	class Buffered_node;
	class Generated_node;
}


struct Genode::Buffered_node : private Memory::Allocation::Attempt, Node
{
	static Byte_range_ptr _allocated(Memory::Constrained_allocator &alloc,
	                                 Memory::Allocation::Attempt &a, size_t num_bytes)
	{
		if (!num_bytes)
			return { nullptr, 0 };

		a = alloc.try_alloc(num_bytes);
		return a.convert<Byte_range_ptr>(
			[&] (Memory::Allocation &a) {
				return Byte_range_ptr((char *)a.ptr, a.num_bytes); },
			[&] (Alloc_error) {
				return Byte_range_ptr(nullptr, 0); });
	}

	Buffered_node(Memory::Constrained_allocator &alloc, Node const &node)
	:
		Memory::Allocation::Attempt(Alloc_error::DENIED),
		Node(node, _allocated(alloc, *this, node.num_bytes()))
	{ }

	using Node::print;
};


struct Genode::Generated_node
{
	Memory::Allocation::Attempt const allocation;

	using Result = Unique_attempt<Node, Buffer_error>;

	Result const node;

	Result _generate(Node::Type const &type, auto const &fn)
	{
		return allocation.convert<Result>(
			[&] (Memory::Allocation const &a) {
				Byte_range_ptr bytes((char *)a.ptr, a.num_bytes);
				return Generator::generate(bytes, type, fn).template convert<Result>(
					[&] (size_t n) -> Result {
						return { Const_byte_range_ptr(bytes.start, n) };
					},
					[&] (Buffer_error e) -> Result { return e; });
			},
			[&] (Alloc_error) { return Buffer_error::EXCEEDED; });
	}

	Generated_node(Memory::Constrained_allocator &alloc, size_t num_bytes,
	               Node::Type const &type, auto const &fn)
	:
		allocation(alloc.try_alloc(num_bytes)),
		node(_generate(type, fn))
	{ }
};

#endif /* _INCLUDE__BASE__NODE_H_ */
