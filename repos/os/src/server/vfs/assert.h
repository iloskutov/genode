/*
 * \brief  VFS result checks
 * \author Emery Hemingway
 * \date   2015-08-19
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VFS__ASSERT_H_
#define _VFS__ASSERT_H_

/* Genode includes */
#include <vfs/file_system.h>
#include <file_system_session/file_system_session.h>

namespace File_system {

	using namespace Genode::Vfs;

	static inline void assert_opendir(Opendir_result const &r)
	{
		r.with_error([&] (Opendir_error e) { switch (e) {
		case Opendir_error::DENIED:      throw Lookup_failed();
		case Opendir_error::RETRY:       throw Lookup_failed();
		case Opendir_error::OUT_OF_RAM:  throw Out_of_ram();
		case Opendir_error::OUT_OF_CAPS: throw Out_of_caps();
		} });
	}

	static inline void assert_symlink(Symlink_result r)
	{
		switch (r) {
		case Symlink_result::DENIED:  throw Lookup_failed();
		case Symlink_result::RETRY:   throw Lookup_failed();
		case Symlink_result::CREATED: break;
		case Symlink_result::UPDATED: break;
		}
	}

	static inline void assert_mkdir(Mkdir_result r)
	{
		switch (r) {
		case Mkdir_result::DENIED:       throw Lookup_failed();
		case Mkdir_result::RETRY:        throw Lookup_failed();
		case Mkdir_result::CREATED:      break;
		case Mkdir_result::UPDATED:      break;
		}
	}

	static inline void assert_unlink(Unlink_result r)
	{
		switch (r) {
		case Unlink_result::DENIED: throw Lookup_failed();
		case Unlink_result::OK:     break;
		}
	}

	static inline void assert_stat(Stat_result r)
	{
		switch (r) {
		case Stat_result::DENIED: throw Lookup_failed();
		case Stat_result::OK:     break;
		}
	}

	static inline void assert_rename(Rename_result r)
	{
		switch (r) {
		case Rename_result::DENIED: throw Lookup_failed();
		case Rename_result::OK:     break;
		}
	}
}

#endif /* _VFS__ASSERT_H_ */
