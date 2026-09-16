/*
 * \brief  File system for providing a value as a file
 * \author Josef Soentgen
 * \author Sebastian Sumpf
 * \date   2018-11-24
 */

/*
 * Copyright (C) 2018-2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VALUE_FILE_SYSTEM_H_
#define _VALUE_FILE_SYSTEM_H_

/* Genode includes */
#include <vfs/single_file_system.h>

namespace Genode::Vfs {
	template <typename, unsigned BUF_SIZE = 64>
	class Value_file_system;
}


template <typename T, unsigned BUF_SIZE>
class Genode::Vfs::Value_file_system : public Single_file_system
{
	public:

		using Name = String<64>;

	private:

		using Buffer = String<BUF_SIZE + 1>;

		Buffer _buffer { };

		struct File_channel : Vfs::File_channel
		{
			Allocator         &_alloc;
			Value_file_system &_value_fs;
			Buffer            &_buffer { _value_fs._buffer };

			File_channel(Allocator &alloc, Attr attr, Value_file_system &value_fs)
			:
				Vfs::File_channel(attr), _alloc(alloc), _value_fs(value_fs)
			{ }

			Read_result read(At const at, Byte_range_ptr const &dst) override
			{
				if (at.pos >= _buffer.length())
					return Read_eof();

				char const * const src = _buffer.string() + at.pos;
				size_t const len = min((size_t)(_buffer.length() - at.pos), dst.num_bytes);

				memcpy(dst.start, src, len);
				return len;
			}

			Write_result write(At const at, Const_byte_range_ptr const &src) override
			{
				if (!writeable || at.pos > BUF_SIZE)
					return Write_error::DENIED;

				size_t const len = min(size_t(BUF_SIZE - at.pos), src.num_bytes);

				_buffer = Buffer(Cstring(src.start, len));

				_value_fs._notify_watchers();

				return len;
			}

			Resize_result resize(file_size size) override
			{
				if (!writeable || size >= BUF_SIZE)
					return Resize_result::DENIED;

				return Resize_result::OK;
			}

			bool read_ready()  const override { return true; }
			bool write_ready() const override { return true; }

			void destruct() override { destroy(_alloc, this); }
		};

	public:

		Value_file_system(Parent_fs &parent_fs, Name const &name,
		                  Buffer const &initial_value)
		:
			Single_file_system(parent_fs, {
				.ident = { { "value ", name } },
				.name  = name,
				.rwx   = File::RW_TRANSACTIONAL
			})
		{
			value(initial_value);
		}

		void value(Buffer const &value)
		{
			_buffer = Buffer(value);
		}

		T value()
		{
			T val { 0 };
			ascii_to(_buffer.string(), val);

			return val;
		}

		Buffer buffer() const  { return _buffer; }

		Open_result open(char const *path, Open_attr attr, Allocator &alloc) override
		{
			if (!_single_file(path))
				return Open_error::DENIED;

			try {
				return *new (alloc)
					File_channel(alloc, { .writeable = attr.writeable }, *this);
			}
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

#endif /* _VALUE_FILE_SYSTEM_H_ */
