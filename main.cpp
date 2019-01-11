#include <iostream>
#include <vector>
#include <memory>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using string = std::string;
using StringVec = std::vector<string>;

struct ProgramOptions {
	po::options_description desc;
	po::variables_map var_map;
	StringVec inc_dir;
	StringVec exc_dir;
	StringVec filemasks;
	string algorithm;
	int max_depth;
	int file_size;
	int block_size;

	ProgramOptions(int ac, char* av[]) : desc("General options") {
		desc.add_options()
			("help,h", "Show help")
			("include-dir,i", po::value<StringVec>(&inc_dir), "Target directory for procedure")
			("exclude-dir,e", po::value<StringVec>(&exc_dir), "Skip directory")
			("max-depth,d", po::value<int>(&max_depth), "Traversing levels number for procedure")
			("file-size,s", po::value<int>(&file_size), "Minimal file size")
			("file,f", po::value<StringVec>(&filemasks), "File name mask")
			("block-size,b", po::value<int>(&block_size)->default_value(1), "Procedure block size")
			("hash-algorithm,a", po::value<string>(&algorithm), "Algorithm: crc32, md5");

		po::parsed_options parsed = po::command_line_parser(ac, av).options(desc).allow_unregistered().run();
		po::store(parsed, var_map);
		po::notify(var_map);
	}
};

namespace fs = boost::filesystem;

class Matcher {};

using OptPtr = std::shared_ptr<ProgramOptions>;
using MatchPtr = std::shared_ptr<Matcher>;

class DirTraverser {
public:
	DirTraverser(const OptPtr& opt, const MatchPtr &match)
		: m_opt(opt), m_match(match) {}

	void operator() () {
		for (auto &d : m_opt->inc_dir) {
			fs::path path(d);
			if (!fs::exists(path)) {
				std::cerr << "Failed to find: " << d << std::endl;
				continue;
			}
			if (!fs::is_directory(path)) {
				std::cerr << "Is not directory: " << d << std::endl;
				continue;
			}

			std::cout << "Target dir: " << d << std::endl;

			fs::directory_iterator end_it;
			for (fs::directory_iterator it(path); it != end_it; ++it) {
				std::cout << "\t" << it->path().filename() << std::endl;
			}
		}
		if (m_opt->var_map.count("hash-algorithm")) {
			std::cout << m_opt->algorithm << std::endl;
		}
	}
private:
	OptPtr m_opt;
	MatchPtr m_match;
};

int main(int ac, char* av[]) {
	try {
		auto opt = std::make_shared<ProgramOptions>(ac, av);
		if (opt->var_map.count("help")) {
			std::cout << opt->desc << std::endl;
			return 0;
		}
		DirTraverser dt(opt, std::make_shared<Matcher>());
		dt();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Something is wrong!" << std::endl;
	}
	return 0;
}
