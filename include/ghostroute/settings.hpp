#ifndef GHOSTROUTE_SETTINGS_HPP
#define GHOSTROUTE_SETTINGS_HPP

#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace ghostroute
{
	struct settings
	{
		std::string target;
		std::vector<std::string> icmp_hops;
		std::vector<std::string> udp_hops;

		static settings read_config(const char* path);
	};

	static void from_json(const nlohmann::json& j, settings& t)
	{
		j.at("target").get_to(t.target);
		j.at("icmp_hops").get_to(t.icmp_hops);
		j.at("udp_hops").get_to(t.udp_hops);
	}
}

#endif