#pragma once
#include <map>
#include "cip_channel.h"

using namespace std;

typedef struct {
	int count_channel_id;
	map<int, cip_channel_def_t> channel_defs;
} cip_context_t;