#pragma once
#include <x264.h>
#include <stdint.h>
#include <mutex>
#include <thread>

using namespace std;

typedef struct {
	int wid;
	x264_param_t param;
	x264_t *encoder;
	x264_picture_t pic;
	int i_frame;
	uint8_t bare;
	uint8_t visible;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t even_width;
	uint16_t even_height;
	int stream_ready;
	std::mutex stream_lock;
	int stream_on;
	int stream_end;
	int stream_reset;
	HANDLE stream_thread;
} cip_window_t;

void cip_window_frame_send(cip_window_t *window, int force_keyframe);
int cip_window_stream_init(cip_window_t *window);
void cip_window_stream_reset(cip_window_t *window);
void cip_window_stream_start(cip_window_t *window);
void cip_window_stream_stop(cip_window_t *window);
DWORD WINAPI cip_window_frame_thread(LPVOID lpParam);