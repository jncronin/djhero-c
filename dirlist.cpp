#include <lvgl/lvgl.h>
#include "dirlist.h"

static std::string root = "/home/jncronin/Music";

std::string get_root() { return root; }
void set_root(std::string r) { root = r; }

static bool dirsort(struct dent* a, struct dent* b)
{
	if(a->is_parent)
		return true;
	if(b->is_parent)
		return false;
	if(is_directory(a->p) && !is_directory(b->p))
		return true;
	if(!is_directory(a->p) && is_directory(b->p))
		return false;

	return a->p.filename().native().compare(b->p.filename().native()) < 0;
}

std::vector<struct dent*> enum_dir(path dir)
{
	std::vector<struct dent*> v;

    /* If the directory is empty, aim to mount it */
    if(equivalent(dir, path(root)))
    {
        int cnt = 0;
        for(auto x : directory_iterator(dir))
            cnt++;
        
        if(cnt == 0)
        {
            std::string mnt_cmd = std::string("/bin/mount ") + dir.native();
            system(mnt_cmd.c_str());
        }
    }

	if(!equivalent(dir, path(root)))
	{
		auto prev_dent = new dent();
		prev_dent->name = SYMBOL_DIRECTORY"  ..";
		prev_dent->p = dir.parent_path();
		prev_dent->is_parent = true;
		v.push_back(prev_dent);
	}

	for(auto x : directory_iterator(dir))
	{
		auto p = x.path();
		if(is_directory(p) || is_regular_file(p) && p.extension() == ".mp3")
		{
			auto cdent = new dent();

			if(is_directory(p))
				cdent->name = std::string(SYMBOL_DIRECTORY"  ") + p.filename().native();
			else
				cdent->name = std::string(SYMBOL_AUDIO"  ") + p.filename().native();
			cdent->p = p;
			cdent->is_parent = false;
			v.push_back(cdent);
		}
	}

	// sort
	std::sort(v.begin(), v.end(), dirsort);

	return v;
}