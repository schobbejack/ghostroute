#ifndef GHOSTROUTE_IP_SOCKET_HPP
#define GHOSTROUTE_IP_SOCKET_HPP

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>

#include <generator>
#include <memory>

#include <ghostroute/message.hpp>
#include <ghostroute/protocol.hpp>
#include <ghostroute/exceptions.hpp>

namespace ghostroute
{
	template<communication_domain D, protocol P>
	struct ip_socket final
	{
	public:
		using socket_type = std::pair<communication_domain, protocol>;
		static constexpr auto INVALID_SOCKET{-1};

		static constexpr auto proto{P};
		static constexpr auto domain{D};

		ip_socket()
		{
			int s = socket(std::to_underlying(D), (P == protocol::eth_p_ipv6 ? SOCK_DGRAM : SOCK_RAW) | SOCK_NONBLOCK, std::to_underlying(P));
			if(INVALID_SOCKET == s)
			{
				throw exception::socket_exception(errno);
			}
			managed_socket = s;
		}

		ip_socket &operator=(const ip_socket &) = delete;
		ip_socket &operator=(ip_socket &&rhs) noexcept
		{
			if(INVALID_SOCKET != managed_socket)
			{
				close(managed_socket);
			}
			managed_socket = rhs.managed_socket;
			rhs.managed_socket = INVALID_SOCKET;
			return *this;
		}

		ip_socket(const ip_socket &) = delete;

		~ip_socket()
		{
			if(INVALID_SOCKET != managed_socket)
			{
				close(managed_socket);
			}
		}

		void sockopt(int level, int name, int value) const
		{
			if(0 != setsockopt(managed_socket, level, name, &value, sizeof(value)))
			{
				throw exception::socket_exception(errno);
			}
		}

		enum class result
		{
			ok,
			shutdown,
			need_more_data
		};

		result get_message(msghdr &msg, ssize_t &length) const
		{
			length = recvmsg(managed_socket, &msg, 0);
			if(length > 0)
			{
				return result::ok;
			}
			if(length < 0)
			{
				if(errno == EAGAIN)
				{
					return result::need_more_data;
				}
				throw exception::socket_exception(errno);
			}
			return result::shutdown;
		}

		std::generator<message> poll_messages(std::ranges::contiguous_range auto &payload_buffer, std::ranges::contiguous_range auto &control_buffer) const
		{
			pollfd fd{};
			fd.fd = managed_socket;
			fd.events = POLLIN;

			if(poll(&fd, 1, 500) < 0 && errno != EINTR)
			{
				throw exception::socket_exception(errno);
			}

			message message(payload_buffer, control_buffer);

			if(fd.revents & POLLIN)
			{
				while(ip_socket::result::ok == get_message(message.msg, message.length))
				{
					message.info = message::msg_info(message.msg);
					co_yield message;
				}
			}
		}

		result send_message(const msghdr &msg, ssize_t &length) const
		{
			length = sendmsg(managed_socket, &msg, 0);
			if(length > 0)
			{
				return result::ok;
			}
			if(length < 0)
			{
				throw exception::socket_exception(errno);
			}
			return result::shutdown;
		}

		int get_socket_descriptor() const noexcept
		{
			return managed_socket;
		}

		ip_socket(ip_socket &&other) noexcept :
		managed_socket{other.managed_socket}
		{
			other.managed_socket = INVALID_SOCKET;
		}

	private:
		int managed_socket{INVALID_SOCKET};
	};
}
#endif