#ifndef GHOSTROUTE_PROTOCOL_HPP
#define GHOSTROUTE_PROTOCOL_HPP

#include <linux/if_ether.h>
#include <netinet/ip.h>

#include <ghostroute/utility.hpp>

namespace ghostroute
{
	enum class protocol
	{
		udp = IPPROTO_UDP,
		icmpv6 = IPPROTO_ICMPV6,
		raw = IPPROTO_RAW,
		eth_p_ipv6 = utility::host_to_network(uint16_t{ETH_P_IPV6}),
		unsupported = ~0
	};

	enum class communication_domain
	{
		inet6 = AF_INET6,
		packet = AF_PACKET
	};
}

#endif