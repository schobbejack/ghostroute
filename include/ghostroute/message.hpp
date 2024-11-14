#ifndef GHOSTROUTE_MESSAGE_HPP
#define GHOSTROUTE_MESSAGE_HPP

#include <netinet/ip.h>

#include <ranges>

#include <ghostroute/protocol.hpp>

namespace ghostroute
{
	struct message
	{
		struct msg_info
		{
			msg_info() = default;

			explicit msg_info(msghdr &msg);

			protocol proto{protocol::udp};
			uint8_t hop_limit{0};
			sockaddr_in6 source{};
			in6_addr destination{};
			int interface_index{0};
		};

		message(std::ranges::contiguous_range auto &payload_buffer, std::ranges::contiguous_range auto &control_buffer) :
		io{.iov_base = payload_buffer.data(), .iov_len = std::size(payload_buffer)},
		msg{
			 .msg_name = &saddr,
			 .msg_namelen = sizeof(saddr),
			 .msg_iov = &io,
			 .msg_iovlen = 1,
			 .msg_control = control_buffer.data(),
			 .msg_controllen = std::size(control_buffer),
			 .msg_flags = 0}
		{
		}

		iovec io;
		sockaddr_storage saddr{};
		msghdr msg;
		ssize_t length{0};
		msg_info info{};
	};

}
#endif