#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

void music_loop();
void speed_ctrl_loop();
void play_music(std::string fname);
void music_init(int argc, char *argv[]);

#endif
