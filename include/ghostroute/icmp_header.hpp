#ifndef GHOSTROUTE_ICMP_HEADER_HPP
#define GHOSTROUTE_ICMP_HEADER_HPP

#include <cstdint>
#include <span>
#include <utility>

namespace ghostroute
{
	enum class icmp_type : uint8_t
	{
		destination_unreachable = 1,
		packet_too_big = 2,
		time_exceeded = 3,
		echo_request = 128,
		echo_reply = 129
	};

	enum class icmp_destination_unreachable_code : uint8_t
	{
		no_route = 0,
		administratively_prohibited = 1,
		out_of_source_scope = 2,
		address_unreachable = 3,
		port_unreachable = 4,
		source_policiy_failed = 5,
		reject = 6
	};

	enum class icmp_time_exceeded_code : uint8_t
	{
		hop_limit_exceeded = 0,
		fragment_reassembly_time_exceeded = 1
	};

	struct icmp_header final
	{
		static constexpr auto size{8};

		icmp_header() = delete;
		explicit icmp_header(std::span<std::byte, size> buf) :
		buffer{buf}
		{
		}

		icmp_type type() const noexcept { return static_cast<icmp_type>(buffer[0]); }
		void type(icmp_type value) const noexcept { buffer[0] = static_cast<std::byte>(value); }

		template<typename T>
		T code() const noexcept { return static_cast<T>(std::to_underlying(buffer[1])); }
		template<typename T>
		void code(T value) const noexcept { buffer[1] = static_cast<std::byte>(value); }

	private:
		std::span<std::byte, size> buffer;
	};
}
#endif