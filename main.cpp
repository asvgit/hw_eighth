#include <iostream>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using string = std::string;
using StringVec = std::vector<string>;

int main(int ac, char* av[]) {
	try {
		po::options_description desc("General options");

		StringVec inc_dir;
		StringVec exc_dir;
		string algorithm;
		desc.add_options()
			("help,h", "Show help")
			("include-dir,i", po::value<StringVec>(&inc_dir), "")
			("exclude-dir,e", po::value<StringVec>(&exc_dir), "")
			("max-depth,d", po::value<int>(), "")
			("file-size,s", po::value<int>(), "")
			("file,f", po::value<StringVec>(), "")
			("blocki-size,b", po::value<int>(), "")
			("cache-algorithm,c", po::value<string>(&algorithm), "");

		po::variables_map var_map;
		po::parsed_options parsed = po::command_line_parser(ac, av).options(desc).allow_unregistered().run();
		po::store(parsed, var_map);
		po::notify(var_map);

		for (auto &d : inc_dir) {
			std::cout << d << std::endl;
		}
		std::cout << algorithm << std::endl;

	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Something is wrong!" << std::endl;
	}
	return 0;
}
