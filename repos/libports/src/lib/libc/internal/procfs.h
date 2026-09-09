/*
 * \brief  procfs emulation
 * \author Johannes Schlatow
 * \date   2026-09-09
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LIBC__INTERNAL__PROCFS_H_
#define _LIBC__INTERNAL__PROCFS_H_

/* libc-internal includes */
#include <internal/fds.h>
#include <internal/file_operations.h>

namespace Libc {
	struct Procfs;
}

struct Libc::Procfs
{
	using Path = File_descriptor::Path;

	static Path _pid_path_from_config(Config const &);
	static Path _self_path_from_config(Config const &);

	Fs           &_fs;
	Config const &_config;
	Path          _pid_path   { _pid_path_from_config(_config)  };
	Path          _self_path  { _self_path_from_config(_config) };
	bool          _fd_present { false };

	Path _fd_path(int id) const;

	void add_fd_file            (int id, Path const &target);
	void add_fd_file_from_kernel(int id, Path const &target);
	void remove_fd_file         (int id);

	Absolute_path resolve_proc_self(Absolute_path) const;

	Procfs(Fs &fs, Config const &config);
};

#endif /* _LIBC__INTERNAL__PROCFS_H_ */
