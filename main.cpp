#include <boost/filesystem.hpp>
#include <iostream>
#include <lvgl/lvgl.h>
#include <lv_drivers/display/fbdev.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <gd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pigpio.h>

#include "dirlist.h"
#include "menuscreen.h"
#include "music.h"
#include "overlay.h"

#if LV_MEM_CUSTOM == 0
#error Please set lv_conf.h LV_MEM_CUSTOM to 1
#endif

using namespace boost::filesystem;
using std::cout;
using std::endl;

static bool kb_read(lv_indev_data_t *data);

static lv_obj_t *scr_list;
lv_group_t *list_grp;

// Run a pigpio command
static const char newl[] = "\n";
void gpio_cmd(std::string cmd)
{
	auto fd = fopen("/dev/pigpio", "w");
	if(fd == NULL) 
	{
		std::cerr << "unable to open pigpio" << std::endl;
	}
	else
	{
		fwrite(cmd.c_str(), 1, cmd.size(), fd);
		fwrite(newl, 1, 1, fd);
		fclose(fd);
	}
}

// Is a game currently playing?  Used to ignore input
bool game_playing = false;

// Is music playing?
bool music_playing = false;

// Now playing screen
lv_obj_t *scr_play;
lv_obj_t *play_img;
lv_obj_t *play_lab;

// Keyboard driver
lv_indev_t *kbd;

// Data for evdev keyboard driver
static uint32_t kb_last_key;
static lv_indev_state_t kb_state;
static struct libevdev *dev = NULL;

// Send a character to the ttyACM0 device
void send_serial(char c)
{
	int fd = open("/dev/ttyACM0", O_WRONLY);
	if(fd != -1)
	{
		write(fd, &c, 1);
		close(fd);
	}
}

static uint32_t keycode_to_ascii(uint32_t ie_key)
{
	switch(ie_key)
	{
		case KEY_RIGHT:
			return LV_GROUP_KEY_RIGHT;
		case KEY_LEFT:
			return LV_GROUP_KEY_LEFT;
		case KEY_UP:
			return LV_GROUP_KEY_UP;
		case KEY_DOWN:
			return LV_GROUP_KEY_DOWN;
		case KEY_ESC:
			return LV_GROUP_KEY_ESC;
		case KEY_ENTER:
			return LV_GROUP_KEY_ENTER;
		case KEY_R:
			return 'r';
		case KEY_G:
			return 'g';
		case KEY_B:
			return 'b';
		case KEY_BACKSPACE:
			return '\b';
		default:
			return ie_key;
	}
}

void pulse_audio_ports()
{
	gpio_cmd("w 2 0");
	usleep(20000);
	gpio_cmd("w 2 1");
	usleep(20000);
	gpio_cmd("w 3 0");
	usleep(20000);
	gpio_cmd("w 3 1");
	usleep(20000);
	gpio_cmd("w 2 0");
	usleep(20000);
	gpio_cmd("w 2 1");
}

int main(int argc, char *argv[])
{
	int fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
	if(fd == -1)
	{
		printf("unable to open device\n");
		return -1;
	}

	int rc = libevdev_new_from_fd(fd, &dev);

	// set big button LED to constant
	send_serial('9');

	// set up gpios - 22 is motor control active low
	// 2 and 3 are amplifier reset ports
	gpio_cmd("m 22 w");
	gpio_cmd("m 2 w");
	gpio_cmd("m 3 w");
	gpio_cmd("w 22 1");
	pulse_audio_ports();

	// set up led/laser as pwm but off for now
	gpio_cmd("m 12 w");
	gpio_cmd("pfs 12 10");
	gpio_cmd("p 12 0");

	lv_init();
	music_init(argc, argv);
	overlay_init();

	// Framebuffer
	fbdev_init();
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.disp_flush = fbdev_flush;
	lv_disp_drv_register(&disp_drv);

	// Keyboard via evdev interface
	lv_indev_drv_t ipt;
	lv_indev_drv_init(&ipt);
	ipt.type = LV_INDEV_TYPE_KEYPAD;
	ipt.read = kb_read;
	kbd = lv_indev_drv_register(&ipt);
	
	// Initialise screen
	scr_list = menuscreen_create();

	lv_scr_load(scr_list);

	while(true)
	{
		if(game_playing)
		{
			usleep(100000);
		}
		else
		{
			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

			if(rc == LIBEVDEV_READ_STATUS_SUCCESS)
			{
				if(ev.type == EV_KEY)
				{
					if(ev.value == 1)
					{
						kb_last_key = ev.code;
						kb_state = LV_INDEV_STATE_PR;
					}
					else if(ev.value == 0)
					{
						kb_state = LV_INDEV_STATE_REL;
					}
				}
			}

			speed_ctrl_loop();
			overlay_loop();
			if(music_playing)
			{
				music_loop();
			}
				
			lv_task_handler();
			usleep(5000);
			lv_tick_inc(5);
		}
	}

	return 0;
}

static bool kb_read(lv_indev_data_t *data)
{
	static uint32_t last_key;
	static lv_indev_state_t last_state;

	uint32_t new_key = keycode_to_ascii(kb_last_key);

	// detect any key presses to awaken the buttons on the music screen
	if(kb_state != last_state || last_key != new_key)
	{
		// don't unhide if an overlay button is pressed
		if(new_key != 'r' && new_key != 'g' && new_key != 'b')
		{
			music_unhide();
			printf("unhiding due to state %u and key %u\n", kb_state, new_key);
		}
		last_state = kb_state;
		last_key = new_key;

		// handle overlay buttons
		if(last_state == LV_INDEV_STATE_PR)
		{
			if(last_key == 'r')
				overlay_play(0);
			else if(last_key == 'g')
				overlay_play(1);
			else if(last_key == 'b')
				overlay_play(2);
		}
	}

	data->state = kb_state;
	data->key = new_key;

	return false;
}

