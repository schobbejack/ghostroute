#include <signal.h>

#include <array>
#include <generator>
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>

#include <ghostroute/format_extensions.hpp>
#include <ghostroute/icmp_header.hpp>
#include <ghostroute/ip_socket.hpp>
#include <ghostroute/message.hpp>
#include <ghostroute/settings.hpp>

namespace ghostroute
{
	using msg_info = message::msg_info;
	using namespace ghostroute::utility;

	template<typename T>
	concept SendMessage = requires(T t, msghdr msg, ssize_t len) {
		t.send_message(msg, len);
	};

	static void send_msg(const SendMessage auto &sock, const msg_info &info, const in6_addr &hop, const std::span<std::byte> payload)
	{
		constexpr auto cmsg_buffer_len{CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(in6_pktinfo))};
		std::array<std::byte, cmsg_buffer_len> control_buffer{};
		iovec io{.iov_base = payload.data(), .iov_len = std::size(payload)};
		sockaddr_in6 saddr{info.source};
		msghdr send_msg{
			 .msg_name = &saddr,
			 .msg_namelen = sizeof(saddr),
			 .msg_iov = &io,
			 .msg_iovlen = 1,
			 .msg_control = control_buffer.data(),
			 .msg_controllen = std::size(control_buffer),
			 .msg_flags = 0};

		const auto hoplimit = std::max(static_cast<uint8_t>(160U - info.hop_limit), 65_ui8);
		auto cmsghdr = CMSG_FIRSTHDR(&send_msg);
		cmsghdr->cmsg_level = IPPROTO_IPV6;
		cmsghdr->cmsg_type = IPV6_HOPLIMIT;
		cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
		*(CMSG_DATA(cmsghdr)) = hoplimit;

		cmsghdr = CMSG_NXTHDR(&send_msg, cmsghdr);
		cmsghdr->cmsg_level = IPPROTO_IPV6;
		cmsghdr->cmsg_type = IPV6_PKTINFO;
		cmsghdr->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));
		auto pktinfo = reinterpret_cast<in6_pktinfo *>(CMSG_DATA(cmsghdr));
		pktinfo->ipi6_ifindex = info.interface_index;
		pktinfo->ipi6_addr = hop;

		ssize_t length{0};
		sock.send_message(send_msg, length);
	}

	template<typename Code>
	requires std::is_same_v<std::underlying_type_t<Code>, uint8_t>
	static void reply_icmp(const SendMessage auto &sock, const message &msg, const in6_addr &target, icmp_type type, Code code)
	{
		constexpr auto min_mtu{1280};
		constexpr auto ipv6_header_size{40};

		std::array<std::byte, min_mtu - ipv6_header_size> data_buffer{};

		const std::span data(data_buffer);

		auto icmpheader = data.template subspan<0, icmp_header::size>();
		const auto payload_length = std::min(std::size(data_buffer) - icmp_header::size, static_cast<size_t>(msg.length));
		auto payload = data.subspan(icmp_header::size, payload_length);

		std::print("icmp {} to {} from {} on if {}\n", std::to_underlying(type), msg.info.source.sin6_addr, target, msg.info.interface_index);

		icmp_header(icmpheader).type(type);
		icmp_header(icmpheader).code(code);

		std::span payload_source(reinterpret_cast<std::byte *>(msg.io.iov_base), std::size(payload));
		std::copy(payload_source.begin(), payload_source.end(), payload.begin());

		send_msg(sock, msg.info, target, data.subspan(0, std::size(icmpheader) + std::size(payload)));
	}

	static void reply_port_closed(const SendMessage auto &sock, const message &msg, const in6_addr &target)
	{
		using enum icmp_destination_unreachable_code;
		using enum icmp_type;
		reply_icmp(sock, msg, target, destination_unreachable, port_unreachable);
	}

	static void reply_hop_limit_exceeded(const SendMessage auto &sock, const message &msg, const in6_addr &hop)
	{
		using enum icmp_time_exceeded_code;
		using enum icmp_type;
		reply_icmp(sock, msg, hop, time_exceeded, hop_limit_exceeded);
	}

	template<class R>
	concept string_range = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, std::string>;
	static inline std::vector<in6_addr> parse_hops(const string_range auto &hops)
	{
		std::vector<in6_addr> parsed_hops;
		parsed_hops.reserve(hops.size());

		for(const auto &i : hops)
		{
			in6_addr addr{};
			if(1 != inet_pton(AF_INET6, i.c_str(), &addr))
			{
				throw std::runtime_error("Error parsing address");
			}
			parsed_hops.push_back(addr);
		}

		return parsed_hops;
	}
}


int main()
{
	using namespace ghostroute;
	auto settings = settings::read_config("settings.json");
	auto icmp_hops = parse_hops(settings.icmp_hops);
	auto udp_hops = parse_hops(settings.udp_hops);

	in6_addr target{};
	inet_pton(AF_INET6, settings.target.c_str(), &target);

	ip_socket<communication_domain::packet, protocol::eth_p_ipv6> receive_socket;
	ip_socket<communication_domain::inet6, protocol::icmpv6> send_socket;

	send_socket.sockopt(SOL_IPV6, IPV6_FREEBIND, 1);

	std::array<std::byte, 9018> receive_data_buffer;
	std::array<std::byte, 128> receive_control_buffer;

	static bool quit{false};
	signal(SIGINT, [](int sig)
		   {
			   quit = sig == SIGINT;
		   });

	std::print("Listening on {}\n\n", target);

	while(!quit)
	{
		for(auto message : receive_socket.poll_messages(receive_data_buffer, receive_control_buffer))
		{
			if(memcmp(&message.info.destination.s6_addr, &target.s6_addr, sizeof(target)))
			{
				continue;
			}

			const auto reply = [&](std::ranges::contiguous_range auto &hops)
			{
				if(message.info.hop_limit >= hops.size())
				{
					reply_port_closed(send_socket, message, target);
				}
				else
				{
					reply_hop_limit_exceeded(send_socket, message, hops[message.info.hop_limit]);
				}
			};

			if(message.info.proto == protocol::icmpv6)
			{
				reply(icmp_hops);
			}
			else if(message.info.proto == protocol::udp)
			{
				reply(udp_hops);
			}
		}
	}
	std::puts("Finished");
}