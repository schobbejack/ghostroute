#include <ghostroute/message.hpp>

#include <arpa/inet.h>
#include <linux/if_packet.h>

#include <cstring>
#include <format>
#include <iostream>
#include <utility>

using msg_info = ghostroute::message::msg_info;

static msg_info from_raw_packet(const msghdr &msg, msg_info &info)
{
	if(msg.msg_iovlen != 1)
	{
		throw std::runtime_error("iov len != 1");
	}
	const auto ll = static_cast<const sockaddr_ll *>(msg.msg_name);

	static constexpr auto address_size{sizeof(in6_addr)};
	static constexpr auto nextheader_offset{6};
	static constexpr auto hoplimit_offset{7};
	static constexpr auto source_address_offset{8};
	static constexpr auto destination_address_offset{source_address_offset + address_size};

	std::span<const std::byte> data{reinterpret_cast<const std::byte *>(msg.msg_iov[0].iov_base), msg.msg_iov[0].iov_len};

	info.hop_limit = std::to_underlying(data[hoplimit_offset]);
	switch(std::to_underlying(data[nextheader_offset]))
	{
		case IPPROTO_UDP:
		case IPPROTO_ICMPV6:
			info.proto = static_cast<ghostroute::protocol>(data[nextheader_offset]);
			break;
		default:
			info.proto = ghostroute::protocol::unsupported;
	}

	info.source.sin6_addr = *reinterpret_cast<const in6_addr *>(data.subspan<source_address_offset, address_size>().data());
	info.source.sin6_family = AF_INET6;
	info.destination = *reinterpret_cast<const in6_addr *>(data.subspan<destination_address_offset, address_size>().data());
	info.interface_index = ll->sll_ifindex;
	return info;
}

msg_info::msg_info(msghdr &msg) :
msg_info()
{
	auto addr = static_cast<sockaddr_storage *>(msg.msg_name);
	if(addr->ss_family == AF_PACKET)
	{
		*this = from_raw_packet(msg, *this);
	}
	else
	{
		throw std::runtime_error("Unsupported family");
	}
}