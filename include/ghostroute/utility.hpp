#ifndef GHOSTROUTE_UTILITY_HPP
#define GHOSTROUTE_UTILITY_HPP

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace ghostroute::utility
{
	template<typename T, size_t... N, class U = std::make_unsigned_t<T>>
	static constexpr U swap_endianness_impl(T v, std::index_sequence<N...>)
	{
		return (((v >> (N * 8) & 0xff) << ((sizeof(v) - 1 - N) * 8)) | ...);
	}

	static constexpr auto host_to_network(std::integral auto value) noexcept
	{
		if constexpr(std::endian::native == std::endian::big)
		{
			return value;
		}

		return swap_endianness_impl(value, std::make_index_sequence<sizeof(value)>{});
	}

	consteval uint8_t operator"" _ui8(unsigned long long arg) noexcept
	{
		return static_cast<uint8_t>(arg);
	}
}

#endif