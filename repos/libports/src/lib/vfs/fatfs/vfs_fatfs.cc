/*
 * \brief  FatFS VFS plugin
 * \author Christian Prochaska
 * \author Emery Hemingway
 * \date   2016-05-22
 *
 * See http://www.elm-chan.org/fsw/ff/00index_e.html
 * or documents/00index_e.html in the FatFS source.
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <vfs/env.h>
#include <os/path.h>

/* Genode block backend */
#include <fatfs/block.h>

namespace Vfs_fatfs {

/* FatFS includes */
#include <fatfs/ff.h>

	using namespace Genode;
	using namespace Genode::Vfs;
	using namespace Fatfs;

	class File_system;
};


class Vfs_fatfs::File_system : public Vfs::File_system
{
	private:

		using Path = Genode::Path<FF_MAX_LFN>;

		struct File_channel;
		struct Fatfs_dir_channel;
		struct Fatfs_file_watch_handle;
		struct Fatfs_dir_watch_handle;

		using File_channels           = List<File_channel>;
		using Fatfs_dir_watch_handles = List<Fatfs_dir_watch_handle>;
		using Fatfs_watch_handles     = List<Fatfs_file_watch_handle>;

		/**
		 * The FatFS library does not support opening a file
		 * for writing twice, so this plugin manages a tree of
		 * open files shared across open file channels.
		 */

		struct File : Avl_node<File>
		{
			Path                path;
			FIL                 fil;
			File_channels       channels;
			Fatfs_watch_handles watchers;

			bool opened() const {
				return (channels.first() || watchers.first()); }

			/************************
			 ** Avl node interface **
			 ************************/

			bool higher(File *other) {
				return (strcmp(other->path.base(), path.base()) > 0); }

			File *lookup(char const *path_str)
			{
				int const cmp = strcmp(path_str, path.base());
				if (cmp == 0)
					return this;

				File *f = Avl_node<File>::child(cmp);
				return f ? f->lookup(path_str) : nullptr;
			}
		};

		struct File_channel : Vfs::File_channel, File_channels::Element
		{
			Allocator   &_alloc;
			File_system &_fs;

			File *file = nullptr;
			bool modifying = false;

			File_channel(Allocator &alloc, Attr attr, File_system &fs)
			:
				Vfs::File_channel(attr), _alloc(alloc), _fs(fs)
			{ }

			~File_channel()
			{
				if (!file)
					return;

				file->channels.remove(this);

				bool notify = false;
				if (file->opened()) {
					notify = modifying;
				} else {
					_fs._close(*file);
				}

				if (notify)
					_fs._notify(*file);
			}

			Read_result read(At const at, Byte_range_ptr const &dst) override
			{
				if (!file) {
					error("Vfs_fatfs: Read_error::DENIED");
					return Read_error::DENIED;
				}

				FIL *fil = &file->fil;
				FRESULT fres = f_lseek(fil, at.pos);
				if (fres == FR_OK) {
					UINT bw = 0;
					fres = f_read(fil, dst.start, dst.num_bytes, &bw);
					return bw;
				}
				return Read_error::DENIED;
			}

			Write_result write(At const at, Const_byte_range_ptr const &src) override
			{
				if (!file)      return Write_error::DENIED;
				if (!writeable) return Write_error::DENIED;

				FRESULT fres = FR_OK;
				FIL *fil = &file->fil;
				FSIZE_t const wpos = at.pos;

				/* seek file pointer */
				if (f_tell(fil) != wpos) {
					/*
					 * seeking beyond the EOF will expand the file size
					 * and is not the expected behavior
					 */
					if (f_size(fil) < wpos)
						return Write_error::DENIED;

					fres = f_lseek(fil, wpos);
					/* check the seek again */
					if (f_tell(fil) != at.pos)
						return Write_error::DENIED;
				}

				if (fres == FR_OK) {
					UINT bw = 0;
					fres = f_write(fil, src.start, src.num_bytes, &bw);
					f_sync(fil);
					modifying = true;
					return bw;
				}

				if (fres == FR_TIMEOUT)
					return Write_error::RETRY;

				return Write_error::DENIED;
			}

			Resize_result resize(file_size len) override
			{
				if (!file)      return Resize_result::DENIED;
				if (!writeable) return Resize_result::DENIED;

				FIL *fil = &file->fil;
				FRESULT res = FR_OK;

				/* f_lseek will expand a file... */
				res = f_lseek(fil, len);
				if (f_tell(fil) != len)
					return Resize_result::DENIED;

				/* ... otherwise truncate will shorten to the seek position */
				if ((res == FR_OK) && (len < f_size(fil)))
					res = f_truncate(fil);

				modifying = true;

				return res == FR_OK ? Resize_result::OK : Resize_result::DENIED;
			}

			bool read_ready() const override { return true; }

			bool write_ready() const override
			{
				/*
				 * Wakeup from WRITE_ERR_WOULD_BLOCK not supported.
				 */
				return true;
			}

			/**
			 * Notify other channels if this channels has modified its file.
			 *
			 * Files are flushed to blocks after every write.
			 */
			Sync_result sync() override
			{
				if (file && modifying) {
					modifying = false;
					file->channels.remove(this);
					_fs._notify(*file);
					file->channels.insert(this);
				}
				return Sync_result::OK;
			}

			void destruct() override { destroy(_alloc, this); }
		};

		struct Fatfs_dir_channel : Vfs::Dir_channel
		{
			Allocator &_alloc;

			file_size cur_index = 0;
			Path const path;
			DIR dir;

			Fatfs_dir_channel(Allocator &alloc, char const *path)
			:
				_alloc(alloc), path(path)
			{ }

			void destruct() override { destroy(_alloc, this); }

			Read_result read(At const at, Byte_range_ptr const &dst) override
			{
				/* not very efficient, just N calls to f_readdir */

				if (dst.num_bytes < sizeof(Dirent))
					return Read_error::DENIED;

				size_t dir_index = size_t(at.pos / sizeof(Dirent));
				if (dir_index < cur_index) {
					/* reset the seek position */
					f_readdir(&dir, nullptr);
					cur_index = 0;
				}

				Dirent &vfs_dirent = *(Dirent*)dst.start;

				FILINFO info;
				FRESULT res;

				while (cur_index <= dir_index) {
					res = f_readdir (&dir, &info);
					if ((res != FR_OK) || (!info.fname[0])) {
						f_readdir(&dir, nullptr);
						cur_index = 0;
						return Read_eof();
					}
					cur_index++;
				}

				vfs_dirent = {
					.type = (info.fattrib & AM_DIR)
					      ? Dirent_type::DIRECTORY
					      : Dirent_type::CONTINUOUS_FILE,
					.rwx  = Node_rwx::rwx(),
					.name = { (char const *)info.fname }
				};
				return sizeof(Dirent);
			}
		};

		Vfs::Env  &_vfs_env;
		Parent_fs &_parent_fs;

		FATFS _fatfs;

		/* Tree of open FatFS file objects */
		Avl_tree<File> _open_files;

		/* Pre-allocated FIL */
		File *_next_file = nullptr;

		/**
		 * Return an open FatFS file matching path or null.
		 */
		File *_opened_file(char const *path)
		{
			return _open_files.first() ?
				_open_files.first()->lookup(path) : nullptr;
		}

		/**
		 * Notify the application for each handle on a given file.
		 */
		void _notify(File &file)
		{
			_parent_fs.notify_watchers(Span::from_cstring(file.path.base()));
		}

		/**
		 * Notify the application for each handle on the parent
		 * directory of a given path.
		 */
		void _notify_parent_of(char const *path)
		{
			with_compound_dir(Span::from_cstring(path), [&] (Span const &dir_path) {
				_parent_fs.notify_watchers(dir_path); });
		}

		/**
		 * Close an open FatFS file
		 */
		void _close(File &file)
		{
			/* close file */
			_open_files.remove(&file);
			f_close(&file.fil);

			if (_next_file == nullptr) {
				/* reclaim heap space */
				file.path.import("");
				_next_file = &file;
			} else {
				destroy(_vfs_env.alloc(), &file);
			}
		}

		/**
		 * Invalidate all channels on a FatFS file
		 * and close the file
		 */
		void _close_all(File &file)
		{
			/* invalidate channels */
			for (auto *handle = file.channels.first();
			     handle; handle = file.channels.first())
			{
				handle->file = nullptr;
				file.channels.remove(handle);
			}

			_close(file);
		}

	public:

		File_system(Vfs::Env &env, Parent_fs &parent_fs, Node const &config)
		:
			Vfs::File_system(Ident::from_node(config)),
			_vfs_env(env), _parent_fs(parent_fs)
		{
			{
				if (f_setcp(0) != FR_OK) {
					error("failed to set codepage to 0");
					throw FR_INVALID_PARAMETER;
				}
			}
			/* mount the file system */
			String<4> const drive_num { "0" };
			switch (f_mount(&_fatfs, (const TCHAR*)drive_num.string(), 1)) {
			case FR_OK: {
				TCHAR label[24] = { '\0' };
				f_getlabel((const TCHAR*)drive_num.string(), label, nullptr);
				log("FAT file system \"", (char const *)label, "\" mounted");
				return;
			}
			case FR_INVALID_DRIVE:
				error("invalid drive ", drive_num);           throw ~0;
			case FR_DISK_ERR:
				error("drive ", drive_num, " disk error");    throw ~0;
			case FR_NOT_READY:
				error("drive ", drive_num, " not ready");     throw ~0;
			case FR_NO_FILESYSTEM:
				error("no file system on drive ", drive_num); throw ~0;
			default:
				error("failed to mount drive ", drive_num);   throw ~0;
			}
		}

		void destruct() override { destroy(_vfs_env.alloc(), this); }

		Open_result open(char const *path, Open_attr attr, Allocator &alloc) override
		{
			File *file = _opened_file(path);

			if (file && attr.create)
				return Open_error::DENIED;

			if (file && f_error(&file->fil)) {
				error("FatFS: hard error on file '", path, "'");
				return Open_error::DENIED;
			};

			/* attempt allocation before modifying blocks */
			if (!_next_file)
				_next_file = new (_vfs_env.alloc()) File();

			File_channel &channel = *new (alloc)
				File_channel(alloc, { .writeable = attr.writeable }, *this);

			if (!file) {
				file = _next_file;
				FRESULT fres = f_open(
					&_next_file->fil, (TCHAR const *)path,
					FA_READ | FA_WRITE | (attr.create ? FA_CREATE_NEW : FA_OPEN_EXISTING));
				if (fres != FR_OK) {
					channel.destruct();
					/*
					 * We might distinguish FR_NO_FILE, FR_NO_PATH, FR_EXIST, FR_INVALID_NAME
					 */
					return Open_error::DENIED;
				}

				file->path.import(path);
				_open_files.insert(file);
				_next_file = nullptr;
			}

			if (attr.create)
				_notify_parent_of(path);

			file->channels.insert(&channel);
			channel.file = file;
			return channel;
		}

		Opendir_result opendir(char const *path, Allocator &alloc) override
		{
			Opendir_error error = Opendir_error::DENIED;

			/* attempt allocation before modifying blocks */
			try {
				Fatfs_dir_channel &channel = *new (alloc) Fatfs_dir_channel(alloc, path);

				FRESULT res = f_opendir(&channel.dir, (const TCHAR*)path);
				if (res == FR_OK)
					return channel;

				destroy(alloc, &channel);

				/* distinguish error regarding FR_NO_PATH? */
			}
			catch (Out_of_ram)  { error = Opendir_error::OUT_OF_RAM;  }
			catch (Out_of_caps) { error = Opendir_error::OUT_OF_CAPS; }
			return error;
		}

		Dataspace_capability dataspace(char const *path) override
		{
			warning(__func__, " not implemented in FAT plugin");
			return Dataspace_capability();
		}

		void release(char const *path, Dataspace_capability ds_cap) override { }

		unsigned num_dirent(char const *path) override
		{
			DIR      dir;
			FILINFO  fno;
			unsigned count = 0;

			if (f_opendir(&dir, (const TCHAR*)path) != FR_OK) return 0;

			fno.fname[0] = 0xFF;
			while ((f_readdir (&dir, &fno) == FR_OK) && fno.fname[0])
				++count;
			f_closedir(&dir);
			return count;
		}

		bool directory(char const *path) override
		{
			if (path[0] == '/' && path[1] == '\0') return true;

			FILINFO fno;

			return f_stat((const TCHAR*)path, &fno) == FR_OK ?
				(fno.fattrib & AM_DIR) : false;
		}

		bool dir_entry_exists(char const *path) override
		{
			FILINFO fno;

			if (_opened_file(path))
				return true;
			else
				return f_stat((const TCHAR*)path, &fno) == FR_OK;
		}

		Stat_result stat(char const *path, Stat &stat) override
		{
			stat = Stat { };

			FILINFO info;

			FRESULT const err = f_stat((const TCHAR*)path, &info);
			switch (err) {
			case FR_OK:
				stat.device = (addr_t)this;
				stat.type   = (info.fattrib & AM_DIR)
				            ? Dirent_type::DIRECTORY
				            : Dirent_type::CONTINUOUS_FILE;
				stat.rwx    = Node_rwx::rwx();

				/* XXX: size in f_stat is always zero */
				if ((stat.type == Dirent_type::CONTINUOUS_FILE) && (info.fsize == 0)) {
					File *file = _opened_file(path);
					if (file) {
						stat.size = f_size(&file->fil);
					} else {
						FIL fil;
						if (f_open(&fil, (TCHAR const *)path, FA_READ) == FR_OK) {
							stat.size = f_size(&fil);
							f_close(&fil);
						}
					}
				} else {
					stat.size = info.fsize;
				}
				return STAT_OK;

			case FR_NO_FILE:
			case FR_NO_PATH:
				return STAT_ERR_NO_ENTRY;

			default:
				error("unhandled FatFS::f_stat error ", (int)err);
				return STAT_ERR_NO_PERM;
			}
			return STAT_ERR_NO_PERM;
		}

		Unlink_result unlink(char const *path) override
		{
			/* close the file if it is open */
			if (File *file = _opened_file(path)) {
				_notify(*file);
				_close_all(*file);
			}

			switch (f_unlink((const TCHAR*)path)) {
			case FR_OK: break;
			case FR_NO_FILE:
			case FR_NO_PATH: return UNLINK_ERR_NO_ENTRY;
			default:         return UNLINK_ERR_NO_PERM;
			}

			_notify_parent_of(path);
			return UNLINK_OK;
		}

		Rename_result rename(char const *from, char const *to) override
		{
			if (File *to_file = _opened_file(to)) {
				_notify(*to_file);
				_close_all(*to_file);
				f_unlink((TCHAR const *)to);
			} else {
				FILINFO info;
				if (FR_OK == f_stat((TCHAR const *)to, &info)) {
					if (info.fattrib & AM_DIR) {
						return RENAME_ERR_NO_PERM;
					} else {
						f_unlink((TCHAR const *)to);
					}
				}
			}

			if (File *from_file = _opened_file(from)) {
				_notify(*from_file);
				_close_all(*from_file);
			}

			switch (f_rename((const TCHAR*)from, (const TCHAR*)to)) {
			case FR_OK: break;
			case FR_NO_FILE:
			case FR_NO_PATH: return RENAME_ERR_NO_ENTRY;
			default:         return RENAME_ERR_NO_PERM;
			}

			_notify_parent_of(from);
			if (strcmp(from, to) != 0)
				_notify_parent_of(to);
			return RENAME_OK;
		}

		Mkdir_result mkdir(char const *path, Timestamp) override
		{
			FRESULT res = f_mkdir((const TCHAR*)path);
			if (res != FR_OK)
				return (res == FR_EXIST) ? Mkdir_result::UPDATED
				                         : Mkdir_result::DENIED;
			return Mkdir_result::CREATED;
		}
};


extern "C" Genode::Vfs::File_system::Factory *vfs_file_system_factory(void)
{
	using namespace Genode;

	struct Factory : Vfs::File_system::Factory
	{
		using Fs = Vfs_fatfs::File_system;

		Instance::Attempt create(Vfs::Env &env, Vfs::Parent_fs &parent_fs,
		                         Genode::Node const &node) override
		{
			Fatfs::block_init(env.env(), env.alloc());
			return { *this, { *new (env.alloc()) Fs(env, parent_fs, node) } };
		}

		void _free(Instance &instance) override { instance.fs.destruct(); };
	};

	static Factory factory;
	return &factory;
}
