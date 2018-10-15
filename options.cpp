#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

#include "menuscreen.h"
#include "dirlist.h"
#include "options.h"

extern lv_obj_t *list;

void pulse_audio_ports();

static void pa_cb(struct dent *dent, struct dent *parent)
{
	pulse_audio_ports();
}

static void v0_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 0%");
}
static void v25_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 25%");
}
static void v50_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 50%");
}
static void v75_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 75%");
}
static void v100_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 100%");
}

static void netaddr_cb(struct dent *dent, struct dent *parent)
{
	std::vector<struct dent *> v;

	// Add back button
	auto prev_dent = new struct dent();
	prev_dent->name = SYMBOL_DIRECTORY"  ..";
	prev_dent->cb = options_cb;
	v.push_back(prev_dent);

	// Iterate interfaces
	struct ifaddrs *ifap;
	int r = getifaddrs(&ifap);
	if(r == 0)
	{
		struct ifaddrs *cif = ifap;

		while(cif != NULL)
		{
			switch(cif->ifa_addr->sa_family)
			{
				case AF_INET:
					{
						char addr[INET_ADDRSTRLEN+1];
						inet_ntop(AF_INET, &((struct sockaddr_in *)cif->ifa_addr)->sin_addr,
								addr, INET_ADDRSTRLEN);
						
						auto ad_dent = new struct dent();
						ad_dent->name = std::string(cif->ifa_name) + ": " +
							std::string(addr);
						ad_dent->cb = NULL;
						v.push_back(ad_dent);
					}
					break;

				case AF_INET6:
					{
						char addr[INET6_ADDRSTRLEN+1];
						inet_ntop(AF_INET6, &((struct sockaddr_in *)cif->ifa_addr)->sin_addr,
								addr, INET6_ADDRSTRLEN);
						
						auto ad_dent = new struct dent();
						ad_dent->name = std::string(cif->ifa_name) + ": " +
							std::string(addr);
						ad_dent->cb = NULL;
						v.push_back(ad_dent);
					}
					break;
			}
		
			cif = cif->ifa_next;
		}
		freeifaddrs(ifap);
	}

	populate_list(&list, NULL, v);
}


void options_cb(struct dent *dent, struct dent *parent)
{
	std::vector<struct dent *> v;

	// Add back button
	auto prev_dent = new struct dent();
	prev_dent->name = SYMBOL_DIRECTORY"  ..";
	prev_dent->cb = root_cb;
	v.push_back(prev_dent);

	// Network config
	auto net_dent = new struct dent();
	net_dent->name = "Network Addresses";
	net_dent->cb = netaddr_cb;
	v.push_back(net_dent);

	// Pulse audio
	auto pa_dent = new struct dent();
	pa_dent->name = "Reset Amplifier";
	pa_dent->cb = pa_cb;
	v.push_back(pa_dent);

	// Volume controls
	auto v0 = new struct dent();
	v0->name = "Volume 0%";
	v0->cb = v0_cb;
	v.push_back(v0);

	auto v25 = new struct dent();
	v25->name = "Volume 25%";
	v25->cb = v25_cb;
	v.push_back(v25);

	auto v50 = new struct dent();
	v50->name = "Volume 50%";
	v50->cb = v50_cb;
	v.push_back(v50);

	auto v75 = new struct dent();
	v75->name = "Volume 75%";
	v75->cb = v75_cb;
	v.push_back(v75);

	auto v100 = new struct dent();
	v100->name = "Volume 100%";
	v100->cb = v100_cb;
	v.push_back(v100);

	populate_list(&list, NULL, v);
}

