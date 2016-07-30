#pragma once
#include "cip_session.h"

typedef struct cip_session cip_session_t;

typedef struct cip_channel{
	int status; // status of channel
	int type; // channel type
	void *handle; // io handle of channel
	cip_session_t *session;
} cip_channel_t;

typedef struct
{
	int id;
	char name[64];
} cip_channel_def_t;