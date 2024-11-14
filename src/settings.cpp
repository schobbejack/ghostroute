#include <ghostroute/settings.hpp>

using namespace ghostroute;

settings settings::read_config(const char *path)
{
	using json = nlohmann::json;
	std::ifstream file(path);

	json data = json::parse(file);

	auto parsed_settings = data.at("settings").get<settings>();

	return parsed_settings;
}
