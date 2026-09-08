/*
 * \brief  Directory-service interface
 * \author Norman Feske
 * \date   2011-02-17
 */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__DIRECTORY_SERVICE_H_
#define _INCLUDE__VFS__DIRECTORY_SERVICE_H_

#include <vfs/types.h>

namespace Genode::Vfs {
	class Vfs_handle;
	struct Directory_service;
}


struct Genode::Vfs::Directory_service : Interface
{
	virtual Dataspace_capability dataspace(char const *path)
	{
		(void)path; return { };
	}

	virtual void release(char const *path, Dataspace_capability) { (void)path; }


	enum General_error { ERR_FD_INVALID, NUM_GENERAL_ERRORS };

	/**
	 * Flags of 'mode' argument of open syscall
	 */
	enum Open_mode {
		OPEN_MODE_RDONLY  = 0,
		OPEN_MODE_WRONLY  = 1,
		OPEN_MODE_RDWR    = 2,
		OPEN_MODE_ACCMODE = 3,
		OPEN_MODE_CREATE  = 0x0800, /* libc O_EXCL */
	};

	enum Open_result
	{
		OPEN_ERR_UNACCESSIBLE,
		OPEN_ERR_NO_PERM,
		OPEN_ERR_EXISTS,
		OPEN_ERR_NAME_TOO_LONG,
		OPEN_ERR_NO_SPACE,
		OPEN_ERR_OUT_OF_RAM,
		OPEN_ERR_OUT_OF_CAPS,
		OPEN_OK
	};

	virtual Open_result open(char const *path, unsigned mode,
	                         Vfs_handle **, Allocator &)
	{
		(void)path; (void)mode; return OPEN_ERR_UNACCESSIBLE;
	}

	virtual Opendir_result opendir(char const *path, Allocator &)
	{
		(void)path; return Opendir_error::DENIED;
	}

	/**
	 * Close handle resources and deallocate handle
	 *
	 * Note: it might be necessary to call 'sync()' before 'close()'
	 *       to ensure that previously written data has been completely
	 *       processed.
	 */
	virtual void close(Vfs_handle *) { }

	/**
	 * Subscribe to watch notifications for the given relative path
	 *
	 * The file system is expected to call 'Parent_dir::notify_watchers'
	 * whenver the given file or directory is modified.
	 *
	 * If the file or directory exists at 'watch' time, 'notify_watchers'
	 * is expected to be called immediately.
	 */
	virtual Watch_result watch(char const *path) { (void)path; return Ok(); }

	/**
	 * Unsubscribe from watch notifications for the given relative path
	 */
	virtual void unwatch(char const *path) { (void)path; }

	struct Stat
	{
		file_size     size;
		Dirent_type   type;
		Node_rwx      rwx;
		unsigned long device;
		Timestamp     modification_time;
	};

	enum Stat_result { STAT_ERR_NO_ENTRY = NUM_GENERAL_ERRORS,
	                   STAT_ERR_NO_PERM, STAT_OK };

	virtual Stat_result stat(char const *path, Stat &)
	{
		(void)path; return STAT_ERR_NO_ENTRY;
	}

	struct Dirent
	{
		struct Name
		{
			enum { MAX_LEN = 128 };
			char buf[MAX_LEN] { };

			Name() { };
			Name(char const *name) { copy_cstring(buf, name, sizeof(buf)); }
		};

		Dirent_type type;
		Node_rwx    rwx;
		Name        name;

		/**
		 * Sanitize dirent members
		 *
		 * This method must be called after receiving a 'Dirent' as
		 * a plain data copy.
		 */
		void sanitize()
		{
			/* enforce null termination */
			name.buf[Name::MAX_LEN - 1] = 0;
		}
	};

	/**
	 * Create directory
	 *
	 * If the directory already exists, its modification time is updated to the
	 * provided timestamp.
	 */
	virtual Mkdir_result mkdir(char const *path, Timestamp)
	{
		(void)path; return Mkdir_result::DENIED;
	}

	/**
	 * Return number of directory entries located at given path
	 */
	virtual unsigned num_dirent(char const *path) { (void)path; return 0; }

	virtual bool directory(char const *path) { (void)path; return false; }

	/**
	 * Return leaf path or nullptr if the path does not exist
	 */
	virtual bool dir_entry_exists(char const *path) { (void)path; return false; }

	/**
	 * Create symbolic link
	 *
	 * If the symlink already exists, its link target and modification time is
	 * updated.
	 */
	virtual Symlink_result symlink(char const *path, char const *target, Timestamp)
	{
		(void)path; (void)target; return Symlink_result::DENIED;
	}

	/**
	 * De-reference symlink
	 *
	 * \return index of de-referenced path element, starting at index 0
	 *
	 * If the returned value equals the number of path elements, the path
	 * does not contain a symlink.
	 *
	 * If a symlink was deferenced, the 'dst' buffer contains the symlink's
	 * target, including a null termination.
	 */
	virtual Follow_result follow(char const *path, Byte_range_ptr const &dst)
	{
		(void)path; (void)dst;return Follow_error::DENIED;
	}


	/************
	 ** Unlink **
	 ************/

	enum Unlink_result { UNLINK_ERR_NO_ENTRY,  UNLINK_ERR_NO_PERM,
	                     UNLINK_ERR_NOT_EMPTY, UNLINK_OK };

	virtual Unlink_result unlink(char const *path)
	{
		(void)path; return UNLINK_ERR_NO_ENTRY;
	}


	/************
	 ** Rename **
	 ************/

	enum Rename_result { RENAME_ERR_NO_ENTRY, RENAME_ERR_CROSS_FS,
	                     RENAME_ERR_NO_PERM,  RENAME_OK };

	virtual Rename_result rename(char const *from, char const *to)
	{
		(void)from; (void)to; return RENAME_ERR_NO_ENTRY;
	}
};

#endif /* _INCLUDE__VFS__DIRECTORY_SERVICE_H_ */
