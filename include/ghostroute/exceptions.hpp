#ifndef GHOSTROUTE_EXCEPTIONS_HPP
#define GHOSTROUTE_EXCEPTIONS_HPP

#include <cstring>
#include <stdexcept>

namespace ghostroute::exception
{
	struct ghostroute_exception : std::runtime_error
	{
		using runtime_error::runtime_error;
		~ghostroute_exception() override = default;
	};

	struct errno_exception : ghostroute_exception
	{
		explicit errno_exception(int error) : ghostroute_exception(strerror(error))
		{}
		~errno_exception() override = default;
	};

	struct socket_exception : errno_exception
	{
		using errno_exception::errno_exception;
		~socket_exception() override = default;
	};

	struct message_exception : ghostroute_exception
	{
		using ghostroute_exception::ghostroute_exception;
		~message_exception() override = default;
	};

}

#endif