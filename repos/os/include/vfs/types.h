/*
 * \brief  Types used by VFS
 * \author Norman Feske
 * \date   2014-04-07
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__TYPES_H_
#define _INCLUDE__VFS__TYPES_H_

#include <util/string.h>
#include <util/progress.h>
#include <util/allocation.h>
#include <util/list_model.h>
#include <base/node.h>
#include <base/env.h>
#include <base/signal.h>
#include <base/allocator.h>
#include <dataspace/client.h>
#include <os/path.h>

namespace Genode::Vfs {

	enum { MAX_PATH_LEN = 512 };

	using file_size = unsigned long long;

	struct Timestamp { uint64_t ms_since_1970; };

	enum class Dirent_type {
		DIRECTORY,
		SYMLINK,
		CONTINUOUS_FILE,
		TRANSACTIONAL_FILE
	};

	struct Node_rwx
	{
		bool readable;
		bool writeable;
		bool executable;

		static Node_rwx ro()  { return { .readable   = true,
		                                 .writeable  = false,
		                                 .executable = false }; }

		static Node_rwx wo()  { return { .readable   = false,
		                                 .writeable  = true,
		                                 .executable = false }; }

		static Node_rwx rw()  { return { .readable   = true,
		                                 .writeable  = true,
		                                 .executable = false }; }

		static Node_rwx rx()  { return { .readable   = true,
		                                 .writeable  = false,
		                                 .executable = true }; }

		static Node_rwx rwx() { return { .readable   = true,
		                                 .writeable  = true,
		                                 .executable = true }; }
	};

	namespace File
	{
		enum class Read  { NOTHING, ANYWHERE };
		enum class Write { DENIED, CONTINUOUS, TRANSACTIONAL };
	};

	/**
	 * Seek position in bytes, for read and write operations
	 */
	struct At { file_size pos; };

	using Absolute_path = Path<MAX_PATH_LEN>;

	struct Scanner_policy_path_element
	{
		static bool identifier_char(char c, unsigned /* i */)
		{
			return (c != '/') && (c != 0);
		}

		static bool end_of_quote(const char *s)
		{
			return s[0] != '\\' && s[1] == '\"';
		}
	};

	struct Parent_fs : Noncopyable, Interface
	{
		virtual void notify_watchers(Span const &) = 0;
	};

	static inline void with_compound_dir(Span const &path, auto const &fn)
	{
		char const *s = path.start; size_t n = path.num_bytes;

		/* if path is directory, drop trailing slash, ignore multiple slashes */
		while (n > 0 && s[n - 1] == '/') n--;

		/* search from end to front for the slash of the compound directory */
		while (n > 0 && s[n - 1] != '/') n--;

		if (n) fn(Span(s, n));
	}

	static inline void for_each_path_elem(Span const &path, auto const &fn)
	{
		path.split('/', [&] (Span const &elem) {
			if (elem.num_bytes)
				fn(elem); });
	}

	using Watch_result = Attempt<Ok, Alloc_error>;

	enum class Write_error { RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	using Write_result = Attempt<size_t, Write_error>;

	enum class Read_error { RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	using Read_result = Attempt<size_t, Read_error>;

	struct Read_eof : Read_result { Read_eof() : Read_result(0) { }; };

	enum class Resize_result { OK, RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	enum class Sync_result { OK, RETRY };

	enum class Update_mtime_result { OK, RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	enum class Mkdir_result       { CREATED, UPDATED, RETRY, DENIED };

	enum class Symlink_result     { CREATED, UPDATED, RETRY, DENIED };

	struct Path_elem
	{
		unsigned index;

		/**
		 * Return true if index matches last 'path' element
		 */
		bool last(Span const &path) const
		{
			unsigned total = 0;
			for_each_path_elem(path, [&] (Span const &) { total++; });
			return (index + 1) == total;
		}
	};

	enum class Follow_error { RETRY, NO_SYMLINK, DENIED };

	using Follow_result = Attempt<Path_elem, Follow_error>;

	/**
	 * Walk path in search of the first 'Path_elem' that satisfies 'cond_fn'
	 *
	 * The functor 'fn' is called for the first matching partial path.
	 * If 'cond_fn' never applies, NO_SYMLINK is returned.
	 */
	static inline Follow_result follow_path(Span const &path,
	                                        auto const &cond_fn, auto const &fn)
	{
		String<MAX_PATH_LEN> partial_path { "" };
		Path_elem path_elem { };
		bool found = false;

		for_each_path_elem(path, [&] (Span const &elem) {
			if (found)
				return;

			partial_path = { partial_path, "/", elem };
			if (cond_fn(partial_path)) {
				found = true;
				return;
			}
			path_elem.index++;
		});

		if (found)
			return fn(path_elem, partial_path);

		return Follow_error::NO_SYMLINK;
	}

	enum class Read_ready_result  { YES, RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	enum class Write_ready_result { YES, RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	struct Dir_channel : Interface, Noncopyable
	{
		enum class Read_error { RETRY, DENIED };

		using Read_result = Attempt<size_t, Read_error>;

		struct Read_eof : Read_result { Read_eof() : Read_result(0) { }; };

		/**
		 * Initiate or complete read of directory entries
		 *
		 * On success, the method returns the number of read bytes.
		 * If zero, the end of file is reached.
		 *
		 * \return Read_error::RETRY  if the read operation is not yet
		 *                            complete and must by tried again once
		 *                            external I/O has progressed
		 */
		virtual Read_result read(At, Byte_range_ptr const &dst) = 0;

		virtual void destruct() = 0;
	};

	enum class Opendir_error { RETRY, DENIED, OUT_OF_RAM, OUT_OF_CAPS };

	using Opendir_result = Unique_attempt<Dir_channel &, Opendir_error>;

	template <typename TO, typename FROM>
	static inline TO converted_error(FROM e)
	{
		switch (e) {
		case FROM::RETRY:       return TO::RETRY;
		case FROM::DENIED:      return TO::DENIED;
		case FROM::OUT_OF_RAM:  return TO::OUT_OF_RAM;
		case FROM::OUT_OF_CAPS: return TO::OUT_OF_CAPS;
		}
		return TO::DENIED;
	}

	struct Env;
	struct Root;
}

#endif /* _INCLUDE__VFS__TYPES_H_ */
