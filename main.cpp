#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

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
			if (Cmp(fv.front(), new_file)) {
				std::cout << "add to group of " << fv.front()->full_name  << " file" << new_file->full_name << std::endl;
				fv.push_back(new_file);
				return;
			}
		}
		std::cout << "Make new group " << new_file->full_name << std::endl;
		m_files.push_back(std::vector<FilePtr>());
		m_files.back().push_back(new_file);
	}

	void Print() {
		for (auto &vf : m_files) {
			for (auto &f : vf)
				std::cout << f->full_name << std::endl;
			std::cout << std::endl;
		}
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
			|| file->spec.size() * m_block_size < file->size;
	}

	string& GetSpec(FilePtr &file, const int ind) {
		if (ind < file->spec.size())
			return file->spec[ind];

		if(file->stream.is_open()) {
			int start = file->spec.size() * m_block_size;
			int end = std::min((file->spec.size() + 1) * m_block_size, file->size);
			file->stream.seekg(start);
			std::string buf;
			const int buf_size = end - start;
			buf.resize(buf_size);
			file->stream.read(&buf[0], buf_size);
			std::cout << "Save spec for file: " << file->full_name << std::endl;
			file->spec.push_back(buf);
			return file->spec.back();
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
			if (m_opt->var_map.count("max-depth"))
				TraverseDir(path, m_opt->max_depth);
			else
				TraverseDir(path);
		}

		std::cout << "Print groups" << std::endl;
		m_match->Print();
	}
private:
	OptPtr m_opt;
	MatchPtr m_match;

	void TraverseDir(const fs::path &dir, const int step = -1) {
		std::cout << "Target dir: " << fs::canonical(dir) << std::endl;
		fs::directory_iterator end_it;
		for (fs::directory_iterator it(dir); it != end_it; ++it) {
			const string full_name = fs::canonical(it->path()).string();
			std::cout << "\t" << fs::canonical(it->path()) << std::endl;
			if (fs::is_regular_file(it->path())
					&& fs::file_size(it->path()) > m_opt->file_size
					&& CheckFileMasks(it->path().filename().string())) {
				m_match->AddFile(full_name, fs::file_size(it->path()));
				continue;
			}
			if (step && fs::is_directory(it->path())
					&& std::find(m_opt->exc_dir.begin(), m_opt->exc_dir.end(), full_name) == m_opt->exc_dir.end()) {
				TraverseDir(it->path(), step - 1);
			}

		}
	}

	bool CheckFileMasks(const string &filename) {
		if (!m_opt->var_map.count("file"))
			return true;

		for (auto &expr : m_opt->filemasks) {
			boost::regex regex(expr);
			if (boost::regex_match(filename, regex))
				return true;
		}
		return false;
	}
};

int main(int ac, char* av[]) {
	try {
		auto opt = std::make_shared<ProgramOptions>(ac, av);
		if (opt->var_map.count("help")) {
			std::cout << opt->desc << std::endl;
			return 0;
		}
		DirTraverser dt(opt, std::make_shared<Matcher>(opt->block_size));
		dt();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Something is wrong!" << std::endl;
	}
	return 0;
}
