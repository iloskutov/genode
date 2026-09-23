/*
 * \brief  File system for providing a read-only value as a file
 * \author Norman Feske
 * \date   2018-03-27
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_
#define _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_

/* Genode includes */
#include <vfs/single_file_system.h>

namespace Genode::Vfs {
	template <typename, unsigned BUF_SIZE = 128>
	class Readonly_value_file_system;
}


template <typename T, unsigned BUF_SIZE>
class Genode::Vfs::Readonly_value_file_system : public Single_file_system
{
	public:

		using Name = String<64>;

	private:

		using Buffer = String<BUF_SIZE + 1>;

		Buffer _buffer { };

		struct File_channel : Vfs::File_channel
		{
			Allocator    &_alloc;
			Buffer const &_buffer;

			File_channel(Allocator &alloc, Buffer const &buffer)
			:
				Vfs::File_channel({ .writeable = false }),
				_alloc(alloc), _buffer(buffer)
			{ }

			Read_result read(At const at, Byte_range_ptr const &dst) override
			{
				if (at.pos > _buffer.length())
					return Read_error::DENIED;

				char const * const src = _buffer.string() + at.pos;
				size_t const len = min(size_t(_buffer.length() - at.pos), dst.num_bytes);
				memcpy(dst.start, src, len);

				return len;
			}

			bool read_ready()  const override { return true; }
			bool write_ready() const override { return false; }

			virtual void destruct() override { destroy(_alloc, this); }
		};

	public:

		Readonly_value_file_system(Parent_fs &parent_fs, Name const &name,
		                           T const &initial_value)
		:
			Single_file_system(parent_fs, {
				.ident = { { "readonly_value ", name } },
				.name  = name,
				.rwx   = File::RO
			})
		{
			value(initial_value);
		}

		void value(T const &value)
		{
			Buffer const orig_buffer = _buffer;

			_buffer = Buffer(value);

			if (_buffer != orig_buffer)
				Single_file_system::_notify_watchers();
		}

		Open_result open(char const *path, Open_attr, Allocator &alloc) override
		{
			if (!_single_file(path))
				return Open_error::DENIED;

			try { return *new (alloc) File_channel(alloc, _buffer); }
			catch (Out_of_ram)  { return Open_error::OUT_OF_RAM; }
			catch (Out_of_caps) { return Open_error::OUT_OF_CAPS; }
		}

		Stat_result stat(char const *path, Stat &out) override
		{
			Stat_result result = Single_file_system::stat(path, out);
			out.size = _buffer.length();
			return result;
		}
};

#endif /* _INCLUDE__VFS__READONLY_VALUE_FILE_SYSTEM_H_ */
