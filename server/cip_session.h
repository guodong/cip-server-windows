#pragma once
#include <map>
#include "cip_channel.h"

using namespace std;

typedef struct cip_channel cip_channel_t;

typedef struct cip_session{
	int sid;
	map<int, cip_channel_t> channels;
} cip_session_t;

