/*
 * \brief  Vfs handle for a Nic client.
 * \author Johannes Schlatow
 * \date   2022-01-26
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__LIB__VFS__TAP__NIC_FILE_SYSTEM_H_
#define _SRC__LIB__VFS__TAP__NIC_FILE_SYSTEM_H_

#include <net/mac_address.h>
#include <nic/packet_allocator.h>
#include <nic_session/connection.h>
#include <vfs/single_file_system.h>


namespace Vfs_nic {

	using namespace Genode;
	using namespace Genode::Vfs;

	class File_system;
}


struct Vfs_nic::File_system : Single_file_system
{
	class File_channel;

	File_system(Parent_fs &parent_fs, char const *name)
	:
		Single_file_system(parent_fs, {
			.ident = { { "data ", name } },
			.name  = name,
			.rwx   = File::RW_TRANSACTIONAL
		})
	{ }
};


class Vfs_nic::File_system::File_channel : public Vfs::File_channel
{
	public:

		using Label = String<64>;

	private:

		static constexpr size_t PKT_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE;
		static constexpr size_t BUF_SIZE = Uplink::Session::QUEUE_SIZE * PKT_SIZE;

		Allocator            &_alloc;
		Genode::Env          &_env;
		Vfs::Env::User       &_vfs_user;
		Nic::Packet_allocator _pkt_alloc;
		Nic::Connection       _nic;
		bool                  _link_state { false };

		bool _notifying = false;
		bool _blocked   = false;

		Io_signal_handler<File_channel> _link_state_handler { _env.ep(), *this, &File_channel::_handle_link_state};
		Io_signal_handler<File_channel> _read_avail_handler { _env.ep(), *this, &File_channel::_handle_read_avail };
		Io_signal_handler<File_channel> _ack_avail_handler  { _env.ep(), *this, &File_channel::_handle_ack_avail };

		void _handle_ack_avail()
		{
			while (_nic.tx()->ack_avail()) {
				_nic.tx()->release_packet(_nic.tx()->get_acked_packet()); }
		}

		void _handle_read_avail()
		{
			if (!read_ready())
				return;

			if (_blocked) {
				_blocked = false;
				_vfs_user.wakeup_vfs_user();
			}

			if (_notifying) {
				_notifying = false;
				read_ready_response();
			}
		}

		void _handle_link_state()
		{
			_link_state = _nic.link_state();
			_handle_read_avail();
		}

	public:

		File_channel(Allocator              &alloc,
		             Attr                    attr,
		             Genode::Env            &env,
		             Vfs::Env::User         &vfs_user,
		             Label            const &label,
		             Net::Mac_address const &)
		:
			Vfs::File_channel(attr),
			_alloc(alloc), _env(env), _vfs_user(vfs_user), _pkt_alloc(&alloc),
			_nic(_env, &_pkt_alloc, BUF_SIZE, BUF_SIZE, label.string())
		{
			_nic.link_state_sigh(_link_state_handler);
			_link_state = _nic.link_state();
			_nic.tx_channel()->sigh_ack_avail   (_ack_avail_handler);
			_nic.rx_channel()->sigh_ready_to_ack(_read_avail_handler);
			_nic.rx_channel()->sigh_packet_avail(_read_avail_handler);
		}

		void notify_read_ready() override { _notifying = true; }

		void mac_address(Net::Mac_address const &) { }

		Net::Mac_address mac_address() {
			return _nic.mac_address(); }

		/************************
		 * Vfs_handle interface *
		 ************************/

		bool read_ready() const override
		{
			auto &nonconst_this = const_cast<File_channel &>(*this);
			auto &rx = *nonconst_this._nic.rx();

			return _link_state && rx.packet_avail() && rx.ready_to_ack();
		}

		bool write_ready() const override
		{
			/* wakeup from WRITE_ERR_WOULD_BLOCK not supported */
			return _link_state;
		}

		Read_result read(At, Byte_range_ptr const &dst) override
		{
			if (!read_ready()) {
				_blocked = true;
				return Read_error::RETRY;
			}

			size_t out_count = 0;

			/* process a single packet from rx stream */
			Packet_descriptor const rx_pkt { _nic.rx()->get_packet() };

			if (rx_pkt.size() > 0 &&
				 _nic.rx()->packet_valid(rx_pkt)) {

				const char *const rx_pkt_base {
					_nic.rx()->packet_content(rx_pkt) };

				out_count = min(rx_pkt.size(), dst.num_bytes);
				memcpy(dst.start, rx_pkt_base, out_count);

				_nic.rx()->acknowledge_packet(rx_pkt);
			}

			return out_count;
		}

		Write_result write(At, Const_byte_range_ptr const &src) override
		{
			_handle_ack_avail();

			if (!_nic.tx()->ready_to_submit())
				return Write_error::RETRY;

			try {
				Packet_descriptor tx_pkt {
					_nic.tx()->alloc_packet(src.num_bytes) };

				void *tx_pkt_base {
					_nic.tx()->packet_content(tx_pkt) };

				memcpy(tx_pkt_base, src.start, src.num_bytes);

				_nic.tx()->submit_packet(tx_pkt);
				return src.num_bytes;

			} catch (...) {

				warning("exception while trying to forward packet from driver "
				        "to Nic connection TX");

				return Write_error::DENIED;
			}
		}

		void destruct() override { destroy(_alloc, this); }
};

#endif /* _SRC__LIB__VFS__TAP__NIC_FILE_SYSTEM_H_ */
