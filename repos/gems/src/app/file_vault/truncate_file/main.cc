/*
 * \brief  Small utility for truncating a given file
 * \author Martin Stein
 * \date   2021-03-19
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <os/vfs.h>

using namespace Genode;

struct Main
{
	Env &env;
	Heap heap { env.ram(), env.rm() };
	Attached_rom_dataspace config { env, "config" };
	Root_directory vfs = config.node().with_sub_node("vfs",
		[&] (Node const &config) -> Root_directory { return { env, heap, config }; },
		[&] ()                   -> Root_directory { return { env, heap, Node() }; });
	Vfs::File_system &fs { vfs.fs() };
	Directory::Path path { config.node().attribute_value("path", Directory::Path { }) };
	Number_of_bytes size { config.node().attribute_value("size", Number_of_bytes { }) };

	Main(Env &env) : env(env)
	{
		bool create = false;
		Vfs::Directory_service::Stat stat { };
		if (fs.stat(path.string(), stat) != Vfs::Directory_service::STAT_OK)
			create = true;

		fs.open(path.string(), { .writeable = true, .create = create }, heap).with_result(
			[&] (Vfs::File_channel &c) {
				c.resize(size);
				c.destruct();
				env.parent().exit(0);
			},
			[&] (Vfs::Open_error) {
				error("failed to create file '", path, "'");
				env.parent().exit(-1);
			});
	}
};

void Component::construct(Env &env) { static Main main(env); }
