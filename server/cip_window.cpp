#include "stdafx.h"
#include <stdint.h>
#include <x264.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <Windows.h>
#include <wingdi.h>
#include "cip_window.h"
#include "cip_protocol.h"
#include "WebsocketServer.h"

#pragma comment(lib, "C:\\x264-64\\libx264.lib")

using namespace std;

typedef uint8_t uint8;


void toeven(size_t *num)
{
	if (*num % 4) {
		*num = *num - (*num % 4);
	}
}

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
	return (66 * r + 129 * g + 25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
	return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
	return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

#define MAKEROWY(NAME, R, G, B, BPP) \
void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
int x;                                                                       \
for (x = 0; x < width; ++x) {                                                \
dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
src_argb0 += BPP;                                                          \
dst_y += 1;                                                                \
}                                                                            \
}                                                                              \
void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
uint8* dst_u, uint8* dst_v, int width) {                \
const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
int x;                                                                       \
for (x = 0; x < width - 1; x += 2) {                                         \
uint8 ab = (src_rgb0[B] + src_rgb0[B + BPP] +                              \
src_rgb1[B] + src_rgb1[B + BPP]) >> 2;                          \
uint8 ag = (src_rgb0[G] + src_rgb0[G + BPP] +                              \
src_rgb1[G] + src_rgb1[G + BPP]) >> 2;                          \
uint8 ar = (src_rgb0[R] + src_rgb0[R + BPP] +                              \
src_rgb1[R] + src_rgb1[R + BPP]) >> 2;                          \
dst_u[0] = RGBToU(ar, ag, ab);                                             \
dst_v[0] = RGBToV(ar, ag, ab);                                             \
src_rgb0 += BPP * 2;                                                       \
src_rgb1 += BPP * 2;                                                       \
dst_u += 1;                                                                \
dst_v += 1;                                                                \
}                                                                            \
if (width & 1) {                                                             \
uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
dst_u[0] = RGBToU(ar, ag, ab);                                             \
dst_v[0] = RGBToV(ar, ag, ab);                                             \
}                                                                            \
}

MAKEROWY(ARGB, 2, 1, 0, 4)

int ARGBToI420(const uint8_t* src_argb, int src_stride_argb,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	int y;
	void(*ARGBToUVRow)(const uint8_t* src_argb0, int src_stride_argb,
		uint8* dst_u, uint8_t* dst_v, int width) = ARGBToUVRow_C;
	void(*ARGBToYRow)(const uint8* src_argb, uint8_t* dst_y, int pix) =
		ARGBToYRow_C;
	if (!src_argb ||
		!dst_y || !dst_u || !dst_v ||
		width <= 0 || height == 0) {
		return -1;
	}
	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		src_argb = src_argb + (height - 1) * src_stride_argb;
		src_stride_argb = -src_stride_argb;
	}

	for (y = 0; y < height - 1; y += 2) {
		ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
		ARGBToYRow(src_argb, dst_y, width);
		ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
		src_argb += src_stride_argb * 2;
		dst_y += dst_stride_y * 2;
		dst_u += dst_stride_u;
		dst_v += dst_stride_v;
	}
	if (height & 1) {
		ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
		ARGBToYRow(src_argb, dst_y, width);
	}
	return 0;
}
extern mutex windows_lock;

int cip_window_stream_init(cip_window_t *window)
{
	uint16_t width = window->width;
	uint16_t height = window->height;

	toeven((size_t*)&width);
	toeven((size_t*)&height);
	window->even_width = width;
	window->even_height = height;


	if (height <= 4 || width <= 4) { /* image is too small to stream */
		return 1;
	}
	x264_param_t *param = &window->param;
	window->i_frame = 0;

	x264_param_default_preset(param, "veryfast", "zerolatency");

	param->i_csp = X264_CSP_I420;
	param->i_width = width;
	param->i_height = height;
	param->i_slice_max_size = 14900000;

	param->b_vfr_input = 0;
	param->b_repeat_headers = 1;
	param->b_annexb = 1;
	param->rc.f_rf_constant = 24;

	if (x264_param_apply_profile(param, "baseline") < 0) {
		printf("[Error] x264_param_apply_profile\n");
		return -1;
	}

	if (x264_picture_alloc(&window->pic, param->i_csp, param->i_width, param->i_height) < 0) {
		printf("[Error] x264_picture_alloc\n");
		return -1;
	}
	window->encoder = x264_encoder_open(param);
	if (!window->encoder) {
		printf("[Error] x264_encoder_open\n");
		goto fail2;
	}

	window->stream_ready = 1;

	return 0;

fail2:
	x264_picture_clean(&window->pic);
	return 1;
}

void cip_window_stream_reset(cip_window_t *window)
{
	window->stream_reset = 1;
}

extern map<int, cip_window_t*> windows;
extern WebsocketServer wsServer;
mutex stream_lock;

void cip_window_stream_start(cip_window_t *window)
{
	//window->stream_lock.lock();
	window->stream_on = 1;
	//window->stream_lock.unlock();
}

void cip_window_stream_stop(cip_window_t *window)
{
	window->stream_on = 0;
}

DWORD WINAPI cip_window_frame_thread(LPVOID lpParam)
{
	cip_window_t *window = (cip_window_t*)lpParam;
	while (1) {
		if (window->stream_end) {
			if (window->stream_ready) {
				x264_picture_clean(&window->pic);
				x264_encoder_close(window->encoder);
			}
			
			break;
		}
		if (!window->stream_on) {
			Sleep(10);
			continue;
		}
		if (window->stream_reset) {
			x264_picture_clean(&window->pic);
			x264_encoder_close(window->encoder);
			cip_window_stream_init(window);
		}
		cip_window_frame_send(window, 0);
		Sleep(10);
	}
	return 0;
}

void cip_window_frame_send(cip_window_t *window, int force_keyframe)
{
	if (!window->visible) {
		return;
	}
	if (!window->stream_ready) {
		cip_window_stream_init(window);
		window->stream_ready = 1;
	}

	int width = window->even_width;
	int height = window->even_height;
	if (width <= 4 || height <= 4) {
		return;
	}
	x264_picture_t *pic = &window->pic;
	x264_picture_t picout;
	int i_frame_size;
	x264_nal_t *nal = NULL;
	int32_t i_nal = 0;

	HWND hwnd = (HWND)window->wid;
	//HDC winDC = GetWindowDC(hwnd);
	HDC winDC = GetDC(NULL);
	if (!winDC) {
		return;
	}
	HDC memDC = CreateCompatibleDC(winDC);
	HBITMAP hbmp = CreateCompatibleBitmap(winDC, width, height);
	BYTE* memory = 0;
	SelectObject(memDC, hbmp);
	//PrintWindow(hwnd, memDC, 0);
	BitBlt(memDC, 0, 0, width, height, winDC, window->x, window->y, SRCCOPY);

	BITMAPINFOHEADER bmi = { 0 };
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biWidth = width;
	bmi.biHeight = -height;
	bmi.biCompression = BI_RGB;
	//bmi.biSizeImage = bitmap_dx * bitmap_dy * 4;

	BYTE *data;
	data = (BYTE*)malloc(4 * width * height);
	GetDIBits(memDC, hbmp, 0, height, data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	
	ARGBToI420(data, width * 4,
		pic->img.plane[0], width,
		pic->img.plane[1], width / 2,
		pic->img.plane[2], width / 2,
		width, height);
	free(data);
	if (force_keyframe) {
		pic->i_type = X264_TYPE_KEYFRAME;
	}
	pic->i_pts = window->i_frame++;

	i_frame_size = x264_encoder_encode(window->encoder, &nal, &i_nal, &window->pic, &picout);
	if (i_frame_size < 0) {
		printf("[Error] x265_encoder_encode\n");
		return;
	}
	//printf("i_frame_size:%d\n", i_frame_size);
	int i;
	for (i = 0; i < i_nal; ++i) {
		/* broadcast event */
		int length = sizeof(cip_event_window_frame_ws_t) + nal[i].i_payload;
		char *buf = (char*)malloc(length);
		//printf("nal length:%d\n", length);
		cip_event_window_frame_ws_t *p = (cip_event_window_frame_ws_t*)buf;
		p->type = CIP_EVENT_WINDOW_FRAME;
		p->wid = window->wid;
		memcpy(buf + sizeof(cip_event_window_frame_ws_t), nal[i].p_payload, nal[i].i_payload);
		wsServer.broadcast(buf, length, 2);
		free(buf);
	}
	pic->i_type = X264_TYPE_AUTO;

	DeleteDC(memDC);
	ReleaseDC(hwnd, winDC);
}