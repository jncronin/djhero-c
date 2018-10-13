#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <vector>

void music_loop();
void speed_ctrl_loop();
void play_music(std::string fname);
void music_init(int argc, char *argv[]);
void play_music_list(std::vector<std::string> fnames,
                std::string image, int start_at);
void music_unhide();

#endif
