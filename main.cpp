#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using string = std::string;
using size_t = std::size_t;
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
			("file-size,s", po::value<int>(&file_size)->default_value(1), "Minimal file size")
			("file,f", po::value<StringVec>(&filemasks), "File name mask")
			("block-size,b", po::value<int>(&block_size)->default_value(1024), "Procedure block size")
			("hash-algorithm,a", po::value<string>(&algorithm), "Algorithm: crc32, md5");

		po::parsed_options parsed = po::command_line_parser(ac, av).options(desc).allow_unregistered().run();
		po::store(parsed, var_map);
		po::notify(var_map);
	}
};

namespace fs = boost::filesystem;

class Matcher {
	struct File {
		string full_name;
		size_t size;
		std::ifstream stream;
		StringVec spec;

		File(const string &file, const size_t s) : full_name(file), size(s), stream(file) {}
	};
	using FilePtr = std::shared_ptr<File>;
public:
	Matcher(const int block) : m_block_size(block) {}
	void AddFile(const string &filename, const size_t size) {
		FilePtr new_file(new File(filename, size));
		for (auto &fv : m_files) {
			for (auto &file : fv) {
				if (Cmp(file, new_file)) {
					fv.push_back(new_file);
					return;
				}
			}
		}
		m_files.push_back(std::vector<FilePtr>());
		m_files.back().push_back(new_file);
	}
private:
	int m_block_size;
	std::vector<std::vector<FilePtr>> m_files;

	bool Cmp(FilePtr &first, FilePtr &second) {
		int i(0);
		for (; HasSpec(first, i) && HasSpec(second, i); ++i)
			if (GetSpec(first, i) != GetSpec(second, i))
				return false;
		if (HasSpec(first, i) || HasSpec(second, i))
			return false;
		return true;
	}

	bool HasSpec(FilePtr &file, const int ind) {
		return file->spec.size() > ind
			|| m_block_size * (ind + 1) <= file->size;
	}

	string& GetSpec(FilePtr &file, const int ind) {
		if (ind < file->spec.size())
			return file->spec[ind];

		if(file->stream.is_open()) {
			int start = file->spec.size() * m_block_size + 1;
			int end = std::min((file->spec.size() + 1) * m_block_size, file->size);
			file->stream.seekg(start);
			std::string buf;
			const int buf_size = end - start;
			buf.resize(buf_size);
			file->stream.read(&buf[0], buf_size);
			std::cout << "Save spec '" << buf << "' for file: " << file->full_name << std::endl;
			file->spec.push_back(buf);
			return buf;
		}
		std::cerr << "Fail read file " << file->full_name << std::endl;
		throw;
	}
};

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
			if (m_opt->var_map.count("include-dir"))
				TraverseDir(path, m_opt->max_depth);
			else
				TraverseDir(path);
		}
		// if (m_opt->var_map.count("hash-algorithm")) {
		//     std::cout << m_opt->algorithm << std::endl;
		// }
	}
private:
	OptPtr m_opt;
	MatchPtr m_match;

	void TraverseDir(const fs::path &dir, const int step = -1) {
		std::cout << "Target dir: " << dir.filename() << std::endl;
		fs::directory_iterator end_it;
		for (fs::directory_iterator it(dir); it != end_it; ++it) {
			std::cout << "\t" << it->path().filename() << std::endl;
			if (fs::is_regular_file(it->path())) {
				m_match->AddFile(it->path().filename().string(), fs::file_size(it->path()));
				continue;
			}
			if (step && fs::is_directory(it->path())) {
				TraverseDir(it->path(), step - 1);
			}

		}
	}
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
