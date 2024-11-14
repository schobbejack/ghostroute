#ifndef GHOSTROUTE_FORMAT_EXTENSIONS_HPP
#define GHOSTROUTE_FORMAT_EXTENSIONS_HPP

#include <arpa/inet.h>

#include <format>

template<>
struct std::formatter<in6_addr> : std::formatter<const char *>
{
	template<typename FormatContext>
	auto format(const in6_addr &v, FormatContext &ctx) const
	{
		auto &&out = ctx.out();
		if(std::array<char, INET6_ADDRSTRLEN> addr_str{}; inet_ntop(AF_INET6, &v, addr_str.data(), std::size(addr_str)))
		{
			return std::formatter<const char *>::format(addr_str.data(), ctx);
		}
		return format_to(out, "<invalid address>");
	}

	template<typename FormatContext>
	constexpr auto parse(FormatContext &ctx)
	{
		return ctx.begin();
	}
};

#endif