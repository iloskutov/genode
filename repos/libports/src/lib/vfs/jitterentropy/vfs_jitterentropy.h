/*
 * \brief  Jitterentropy based random file system
 * \author Josef Soentgen
 * \date   2014-08-19
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _JITTERENTROPY_FILE_SYSTEM_H_
#define _JITTERENTROPY_FILE_SYSTEM_H_

/* Genode includes */
#include <vfs/single_file_system.h>

/* jitterentropy includes */
#include <jitterentropy.h>

namespace Vfs_jitterentropy {

	using namespace Genode;
	using namespace Genode::Vfs;

	class File_system;
}


class Vfs_jitterentropy::File_system : public Single_file_system
{
	private:

		Allocator        &_alloc;
		struct rand_data *_ec_stir;
		bool              _initialized;

		bool _init_jitterentropy(Allocator &alloc)
		{
			/* initialize private allocator backend */
			jitterentropy_init(alloc);

			int err = jent_entropy_init();
			if (err) {
				error("jitterentropy library could not be initialized!");
				return false;
			}

			/* use the default behaviour as specified in jitterentropy(3) */
			_ec_stir = jent_entropy_collector_alloc(0, 0);
			if (!_ec_stir) {
				error("jitterentropy could not allocate entropy collector!");
				return false;
			}

			return true;
		}

		class File_channel : public Vfs::File_channel
		{
			private:

				Allocator        &_alloc;
				struct rand_data *_ec_stir;
				bool             &_initialized;

			public:

				File_channel(Allocator         &alloc,
				             struct rand_data  *ec_stir,
				             bool              &initialized)
				:
					Vfs::File_channel({ .writeable = false }),
					_alloc(alloc), _ec_stir(ec_stir), _initialized(initialized)
				{ }

				Read_result read(At, Byte_range_ptr const &dst) override
				{
					if (!_initialized)
						return Read_error::DENIED;

					enum { MAX_BUF_LEN = 256UL };
					char buf[MAX_BUF_LEN];

					size_t const len = min(dst.num_bytes, MAX_BUF_LEN);

					if (jent_read_entropy(_ec_stir, buf, len) < 0)
						return Read_error::DENIED;

					memcpy(dst.start, buf, len);

					return len;
				}

				bool read_ready()  const override { return true; }
				bool write_ready() const override { return false; }

				void destruct() override { destroy(_alloc, this); }
		};

	public:

		File_system(Parent_fs &parent_fs, Allocator &alloc, Node const &config)
		:
			Single_file_system(parent_fs, {
				.ident = Ident::from_node(config),
				.name  = File::Name::from_node(config),
				.rwx   = File::RO
			}),
			_alloc(alloc),
			_ec_stir(0),
			_initialized(_init_jitterentropy(alloc))
		{ }

		~File_system()
		{
			if (_initialized)
				jent_entropy_collector_free(_ec_stir);
		}

		void destruct() override { destroy(_alloc, this); }

		Open_result open(char const *path, Open_attr, Allocator &alloc) override
		{
			if (!_single_file(path))
				return Open_error::DENIED;

			return *new (alloc) File_channel(alloc, _ec_stir, _initialized);
		}
};

#endif /* _JITTERENTROPY_FILE_SYSTEM_H_ */
