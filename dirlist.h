#ifndef DIRLIST_H
#define DIRLIST_H

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

struct dent
{
	std::string name;
	path p;
	bool is_parent;
};

std::string get_root();
void set_root(std::string r);
std::vector<struct dent*> enum_dir(path dir);

#endif
