// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2025 Cyril Hrubis <metan@ucw.cz>
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <dlfcn.h>

#include "libretro.h"

#include <core/gp_core.h>
#include <filters/gp_resize.h>
#include <text/gp_text.h>
#include <backends/gp_backends.h>

#include <alsa/asoundlib.h>

#include "keymap.h"

static snd_pcm_t *alsa_pcm;

static gp_backend *backend;
static gp_pixel_type pixel_type = GP_PIXEL_UNKNOWN;

static uint16_t joypad_keys[RETRO_DEVICE_ID_JOYPAD_R3+1];
static int16_t mouse_state[RETRO_DEVICE_ID_POINTER_COUNT];
static volatile int should_exit;

static struct core_retro {
	void *handle;
	bool initialized;

	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	bool (*retro_load_game)(const struct retro_game_info *game);
	void (*retro_unload_game)(void);
} core_retro;

static retro_keyboard_event_t keyboard_callback;

#define load_sym(V, S) do {\
		if (!((*(void**)&V) = dlsym(core_retro.handle, #S))) { \
			fprintf(stderr, "Failed to load symbol '" #S "'': %s", dlerror()); \
			exit(1); \
		} \
} while (0)

#define load_retro_sym(S) load_sym(core_retro.S, S)

static int audio_init(int frequency)
{
	int err;

	if ((err = snd_pcm_open(&alsa_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Failed to open default device: %s", snd_strerror(err));
		return 1;
	}

	err = snd_pcm_set_params(alsa_pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 2, frequency, 1, 64 * 1000);

	if (err < 0) {
		fprintf(stderr, "Failed to configure default device: %s", snd_strerror(err));
		return 1;
	}

	return 0;
}

static void audio_deinit(void)
{
	snd_pcm_close(alsa_pcm);
}

static size_t audio_write(const void *buf, unsigned frames)
{
	if (!alsa_pcm)
		return 0;

	int written = snd_pcm_writei(alsa_pcm, buf, frames);

	if (written < 0) {
		fprintf(stderr, "Alsa warning/error #%i: ", -written);
		snd_pcm_recover(alsa_pcm, written, 0);

		return 0;
	}

	return written;
}

static void core_log(enum retro_log_level level, const char *fmt, ...)
{
	char buffer[4096];
	static const char * levelstr[] = { "dbg", "inf", "wrn", "err" };
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	fprintf(stderr, "[%s] %s", levelstr[level], buffer);
	fflush(stderr);

	if (level == RETRO_LOG_ERROR)
		exit(EXIT_FAILURE);
}

static void handle_controller_info(const struct retro_controller_info *ports)
{
	int port = 0, keyboard_plugged = 0;
	unsigned int i;

	while (ports[port].types) {
		const struct retro_controller_description *desc = ports[port].types;

		printf("Port %02i:\n", port);

		for (i = 0; i < ports[port].num_types; i++) {
			/* If core supports keyboard plug it in! */
			if (desc->id == RETRO_DEVICE_KEYBOARD && !keyboard_plugged) {
				keyboard_plugged = 1;
				core_retro.retro_set_controller_port_device(port, RETRO_DEVICE_KEYBOARD);
			}

			printf(" controller '%s' id %u\n", desc->desc, desc->id);
		}

		port++;
	}
}

static void set_keyboard_callback(const struct retro_keyboard_callback *callback)
{
	GP_DEBUG(1, "Keyboard callback set to %p\n", callback->callback);

	keyboard_callback = callback->callback;
}

static bool core_environment(unsigned cmd, void *data)
{
	const enum retro_pixel_format *format;
	bool *bval;

	switch (cmd) {
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
		struct retro_log_callback *cb = (struct retro_log_callback *)data;
		cb->log = core_log;
	break;
	}
	case RETRO_ENVIRONMENT_GET_CAN_DUPE:
		bval = (bool*)data;
		*bval = true;
	break;
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		format = (enum retro_pixel_format *)data;

		switch (*format) {
		case RETRO_PIXEL_FORMAT_0RGB1555:
			return false;
		case RETRO_PIXEL_FORMAT_XRGB8888:
			pixel_type = GP_PIXEL_xRGB8888;
			return true;
		case RETRO_PIXEL_FORMAT_RGB565:
			pixel_type = GP_PIXEL_RGB565;
			return true;
		default:
			core_log(RETRO_LOG_DEBUG, "Unknown pixel type %u", *format);
			return false;
		}
	break;
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		*(const char **)data = ".";
		return true;
	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
		handle_controller_info(data);
		return true;
	case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
		set_keyboard_callback(data);
		return false;
	default:
		core_log(RETRO_LOG_DEBUG, "Unhandled env #%u\n", cmd);
		return false;
	}

	return true;
}


static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
	gp_pixmap pix;
	uint8_t *data_cpy = NULL;

	if (!data)
		return;

	gp_fill(backend->pixmap, 0x000000);

	if (pixel_type == GP_PIXEL_RGB565) {
		data_cpy = malloc(pitch * height);

		memcpy(data_cpy, data, pitch * height);

		unsigned int h, i;

		for (h = 0; h < height; h++) {
			uint8_t *row = data_cpy + h * pitch;

			for (i = 0; i < pitch/2; i++)
				GP_SWAP(row[2*i], row[2*i+1]);
		}

		gp_pixmap_init_ex(&pix, width, height, pixel_type, pitch, data_cpy, 0);
	} else {
		gp_pixmap_init_ex(&pix, width, height, pixel_type, pitch, (void*)data, 0);
	}

	int rat = GP_MIN(gp_backend_w(backend)/width, gp_backend_h(backend)/height);

	if (rat > 1) {
		gp_pixmap dst;

		gp_coord x_off = (gp_backend_w(backend) - width * rat) / 2;
		gp_coord y_off = (gp_backend_h(backend) - height * rat) / 2;

		gp_sub_pixmap(backend->pixmap, &dst, x_off, y_off, width * rat, height * rat);

		gp_pixmap *src = gp_pixmap_convert_alloc(&pix, gp_backend_pixel_type(backend));

		gp_filter_resize(src, &dst, GP_INTERP_NN, NULL);

		gp_pixmap_free(src);
	} else {
		gp_blit_xyxy(&pix, 0, 0, width-1, height-1, backend->pixmap, 0, 0);
	}

	gp_backend_flip(backend);

	free(data_cpy);
}

static int map_joypad_key(uint32_t key)
{
	switch (key) {
	case GP_KEY_A:
		return RETRO_DEVICE_ID_JOYPAD_A;
	case GP_KEY_B:
		return RETRO_DEVICE_ID_JOYPAD_B;
	case GP_KEY_L:
		return RETRO_DEVICE_ID_JOYPAD_L;
	case GP_KEY_R:
		return RETRO_DEVICE_ID_JOYPAD_R;
	case GP_KEY_X:
		return RETRO_DEVICE_ID_JOYPAD_X;
	case GP_KEY_Y:
		return RETRO_DEVICE_ID_JOYPAD_Y;
	case GP_KEY_UP:
		return RETRO_DEVICE_ID_JOYPAD_UP;
	case GP_KEY_DOWN:
		return RETRO_DEVICE_ID_JOYPAD_DOWN;
	case GP_KEY_LEFT:
		return RETRO_DEVICE_ID_JOYPAD_LEFT;
	case GP_KEY_RIGHT:
		return RETRO_DEVICE_ID_JOYPAD_RIGHT;
	case GP_KEY_ENTER:
		return RETRO_DEVICE_ID_JOYPAD_START;
	case GP_KEY_BACKSPACE:
		return RETRO_DEVICE_ID_JOYPAD_SELECT;
	default:
		return -1;
	}
}

static int16_t scale_mouse_coords(uint32_t pos, uint32_t max)
{
	return 0;
}

static void handle_keyboard_callback(gp_event *ev)
{
	if (!keyboard_callback)
		return;

	if (ev->code == GP_EV_KEY_REPEAT)
		return;

	enum retro_key key = map_key(ev->val);

	if (key == RETROK_UNKNOWN)
		return;

	keyboard_callback(ev->code, key, 0, 0);
}

static void core_input_poll(void)
{
	gp_event *ev;
	int joypad_key;

	while ((ev = gp_backend_ev_poll(backend))) {
		switch (ev->type) {
		case GP_EV_REL:
			switch (ev->code) {
			case GP_EV_REL_POS:
				mouse_state[RETRO_DEVICE_ID_POINTER_X] = scale_mouse_coords(ev->st->cursor_x, 0);
				mouse_state[RETRO_DEVICE_ID_POINTER_Y] = scale_mouse_coords(ev->st->cursor_y, 0);
			break;
			}
		break;
		case GP_EV_KEY:
			handle_keyboard_callback(ev);

			if (ev->val == GP_BTN_LEFT)
				mouse_state[RETRO_DEVICE_ID_POINTER_PRESSED] = !!ev->code;

			joypad_key = map_joypad_key(ev->val);

			if (joypad_key == -1)
				continue;

			if (ev->code)
				joypad_keys[joypad_key] = 1;
			else
				joypad_keys[joypad_key] = 0;
		break;
		case GP_EV_SYS:
			switch (ev->code) {
			case GP_EV_SYS_QUIT:
				should_exit = 1;
			break;
			case GP_EV_SYS_RESIZE:
				gp_backend_resize_ack(backend);
			break;
			}
		break;
		}
	}
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
	if (port || index)
		return 0;

	switch (device) {
	case RETRO_DEVICE_JOYPAD:
		return joypad_keys[id];
	case RETRO_DEVICE_MOUSE:
		return mouse_state[id];
	}

	GP_DEBUG(1, "Unhandled device %i\n", device);

	return 0;
}

static void core_audio_sample(int16_t left, int16_t right)
{
	int16_t buf[2] = {left, right};
	audio_write(buf, 1);
}

static size_t core_audio_sample_batch(const int16_t *data, size_t frames)
{
	return audio_write(data, frames);
}

static void core_load(const char *sofile)
{
	void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

	memset(&core_retro, 0, sizeof(core_retro));
	core_retro.handle = dlopen(sofile, RTLD_LAZY);

	if (!core_retro.handle) {
		fprintf(stderr, "Failed to load core: %s", dlerror());
		exit(1);
	}

	dlerror();

	load_retro_sym(retro_init);
	load_retro_sym(retro_deinit);
	load_retro_sym(retro_api_version);
	load_retro_sym(retro_get_system_info);
	load_retro_sym(retro_get_system_av_info);
	load_retro_sym(retro_set_controller_port_device);
	load_retro_sym(retro_reset);
	load_retro_sym(retro_run);
	load_retro_sym(retro_load_game);
	load_retro_sym(retro_unload_game);
	load_retro_sym(retro_serialize_size);
	load_retro_sym(retro_serialize);
	load_retro_sym(retro_unserialize);

	load_sym(set_environment, retro_set_environment);
	load_sym(set_video_refresh, retro_set_video_refresh);
	load_sym(set_input_poll, retro_set_input_poll);
	load_sym(set_input_state, retro_set_input_state);
	load_sym(set_audio_sample, retro_set_audio_sample);
	load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

	set_environment(core_environment);
	set_video_refresh(core_video_refresh);
	set_input_poll(core_input_poll);
	set_input_state(core_input_state);
	set_audio_sample(core_audio_sample);
	set_audio_sample_batch(core_audio_sample_batch);

	core_retro.retro_init();
	core_retro.initialized = true;

	puts("Core loaded");
}

static int core_load_game(const char *filename)
{
	struct retro_system_av_info av = {};
	struct retro_system_info system = {};
	struct retro_game_info info = {.path = filename, .data = NULL};
	FILE *file;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open '%s': %s", filename, strerror(errno));
		return 1;
	}

	fseek(file, 0, SEEK_END);
	info.size = ftell(file);
	rewind(file);

	core_retro.retro_get_system_info(&system);

	if (!system.need_fullpath) {
		info.data = malloc(info.size);
		if (!info.data) {
			fprintf(stderr, "Malloc failed\n");
			goto err0;
		}

		if (!fread((void*)info.data, info.size, 1, file)) {
			fprintf(stderr, "Failed to load '%s': %s", filename, strerror(errno));
			goto err1;
		}
	}

	if (!core_retro.retro_load_game(&info)) {
		fprintf(stderr, "The core failed to load the content.");
		goto err1;
	}

	core_retro.retro_get_system_av_info(&av);
	audio_init(av.timing.sample_rate);

	return 0;
err1:
	free((void*)info.data);
err0:
	fclose(file);
	return 1;
}

static void core_unload(void)
{
	if (core_retro.initialized)
		core_retro.retro_deinit();

	if (core_retro.handle)
		dlclose(core_retro.handle);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s <core.so> <game>", argv[0]);
		exit(1);
	}

	backend = gp_backend_init(NULL, 0, 0, "gpretro");

	gp_fill(backend->pixmap, 0x000000);
	gp_print(backend->pixmap, NULL, gp_backend_w(backend)/2, gp_backend_h(backend)/2,
	         GP_ALIGN_CENTER | GP_VALIGN_CENTER, 0xffffff, 0x000000,
		 "Loading '%s'", argv[2]);
	gp_backend_flip(backend);

	core_load(argv[1]);

	if (core_load_game(argv[2]))
		exit(1);

	while (!should_exit)
		core_retro.retro_run();

	core_unload();
	audio_deinit();

	gp_backend_exit(backend);

	return 0;
}
