/*
 * \brief  libusb file system
 * \author Christian Prochaska
 * \date   2022-01-31
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


/* Genode includes */
#include <base/allocator_avl.h>
#include <vfs/single_file_system.h>
#include <vfs/env.h>

namespace Vfs_libusb {

	using namespace Genode;
	using namespace Genode::Vfs;

	class File_system;
}


/*
 * These functions are implemented in the Genode backend of libusb.
 */
extern void libusb_genode_backend_init(Genode::Env &, Genode::Allocator &,
                                       Genode::Signal_context_capability);

extern bool libusb_genode_backend_signaling;

class Vfs_libusb::File_system : public Vfs::Single_file_system
{
	private:

		Vfs::Env &_env;

		class File_channel : public Vfs::File_channel
		{
			private:

				Allocator      &_alloc;
				Genode::Env    &_env;
				Vfs::Env::User &_vfs_user;

				Io_signal_handler<File_channel> _handler {
					_env.ep(), *this, &File_channel::_handle };

				void _handle()
				{
					libusb_genode_backend_signaling = true;
					_vfs_user.wakeup_vfs_user();
				}

			public:

				File_channel(Allocator         &alloc,
				             Genode::Env       &env,
				             Vfs::Env::User    &vfs_user)
				:
					Vfs::File_channel({ .writeable = false }),
					_alloc(alloc), _env(env), _vfs_user(vfs_user)
				{
					log("libusb: waiting until device is plugged...");
					libusb_genode_backend_init(env, alloc, _handler);
					log("libusb: device is plugged");
				}

				bool read_ready() const override {
					return libusb_genode_backend_signaling; }

				Read_result read(At, Byte_range_ptr const &) override {
					return Read_error::DENIED; }

				bool write_ready() const override {
					return true; }

				void destruct() override { destroy(_alloc, this); }
		};

	public:

		File_system(Vfs::Env &env, Parent_fs &parent_fs, Node const &config)
		:
			Single_file_system(parent_fs, {
				.ident = Ident::from_node(config),
				.name  = File::Name::from_node(config),
				.rwx   = File::RO
			}),
			_env(env) { }

		void destruct() override { destroy(_env.alloc(), this); }

		Open_result open(char const *path, Open_attr, Allocator &alloc) override
		{
			if (!_single_file(path))
				return Open_error::DENIED;

			return *new (alloc) File_channel(alloc, _env.env(), _env.user());
		}
};


extern "C" Genode::Vfs::File_system::Factory *vfs_file_system_factory(void)
{
	using namespace Genode;

	struct Factory : Vfs::File_system::Factory
	{
		using Fs = Vfs_libusb::File_system;

		Instance::Attempt create(Vfs::Env &env, Vfs::Parent_fs &parent_fs,
		                         Node const &node) override
		{
			return { *this, { *new (env.alloc()) Fs(env, parent_fs, node) } };
		}

		void _free(Instance &instance) override { instance.fs.destruct(); };
	};

	static Factory factory;
	return &factory;
}
