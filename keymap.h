// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2025 Cyril Hrubis <metan@ucw.cz>
 */

#ifndef KEYMAP_H
#define KEYMAP_H

static enum retro_key map_key(uint32_t key)
{
	switch (key) {
	case GP_KEY_BACKSPACE:
		return RETROK_BACKSPACE;
	case GP_KEY_TAB:
		return RETROK_TAB;
	case GP_KEY_ENTER:
		return RETROK_RETURN;
	case GP_KEY_PAUSE:
		return RETROK_PAUSE;
	case GP_KEY_ESC:
		return RETROK_ESCAPE;
	case GP_KEY_SPACE:
		return RETROK_SPACE;

	case GP_KEY_COMMA:
		return RETROK_COMMA;
	case GP_KEY_MINUS:
		return RETROK_MINUS;
	case GP_KEY_DOT:
		return RETROK_PERIOD;
	case GP_KEY_SLASH:
		return RETROK_SLASH;

	case GP_KEY_0:
		return RETROK_0;
	case GP_KEY_1:
		return RETROK_1;
	case GP_KEY_2:
		return RETROK_2;
	case GP_KEY_3:
		return RETROK_3;
	case GP_KEY_4:
		return RETROK_4;
	case GP_KEY_5:
		return RETROK_5;
	case GP_KEY_6:
		return RETROK_6;
	case GP_KEY_7:
		return RETROK_7;
	case GP_KEY_8:
		return RETROK_8;
	case GP_KEY_9:
		return RETROK_9;

	case GP_KEY_SEMICOLON:
		return RETROK_SEMICOLON;

	case GP_KEY_EQUAL:
		return RETROK_EQUALS;

	case GP_KEY_BACKSLASH:
		return RETROK_BACKSLASH;

	case GP_KEY_GRAVE:
		return RETROK_BACKQUOTE;

	case GP_KEY_A:
		return RETROK_a;
	case GP_KEY_B:
		return RETROK_b;
	case GP_KEY_C:
		return RETROK_c;
	case GP_KEY_D:
		return RETROK_d;
	case GP_KEY_E:
		return RETROK_e;
	case GP_KEY_F:
		return RETROK_f;
	case GP_KEY_G:
		return RETROK_g;
	case GP_KEY_H:
		return RETROK_h;
	case GP_KEY_I:
		return RETROK_i;
	case GP_KEY_J:
		return RETROK_j;
	case GP_KEY_K:
		return RETROK_k;
	case GP_KEY_L:
		return RETROK_l;
	case GP_KEY_M:
		return RETROK_m;
	case GP_KEY_N:
		return RETROK_n;
	case GP_KEY_O:
		return RETROK_o;
	case GP_KEY_P:
		return RETROK_p;
	case GP_KEY_Q:
		return RETROK_q;
	case GP_KEY_R:
		return RETROK_r;
	case GP_KEY_S:
		return RETROK_s;
	case GP_KEY_T:
		return RETROK_t;
	case GP_KEY_U:
		return RETROK_u;
	case GP_KEY_V:
		return RETROK_v;
	case GP_KEY_W:
		return RETROK_w;
	case GP_KEY_X:
		return RETROK_x;
	case GP_KEY_Y:
		return RETROK_y;
	case GP_KEY_Z:
		return RETROK_z;

	case GP_KEY_LEFT_BRACE:
		return RETROK_LEFTBRACE;

	case GP_KEY_RIGHT_BRACE:
		return RETROK_RIGHTBRACE;

	case GP_KEY_DELETE:
		return RETROK_DELETE;

	case GP_KEY_KP_0:
		return RETROK_KP0;
	case GP_KEY_KP_1:
		return RETROK_KP1;
	case GP_KEY_KP_2:
		return RETROK_KP2;
	case GP_KEY_KP_3:
		return RETROK_KP3;
	case GP_KEY_KP_4:
		return RETROK_KP4;
	case GP_KEY_KP_5:
		return RETROK_KP5;
	case GP_KEY_KP_6:
		return RETROK_KP6;
	case GP_KEY_KP_7:
		return RETROK_KP7;
	case GP_KEY_KP_8:
		return RETROK_KP8;
	case GP_KEY_KP_9:
		return RETROK_KP9;
	case GP_KEY_KP_DOT:
		return RETROK_KP_PERIOD;
	case GP_KEY_KP_SLASH:
		return RETROK_KP_DIVIDE;
	case GP_KEY_KP_ASTERISK:
		return RETROK_KP_MULTIPLY;
	case GP_KEY_KP_MINUS:
		return RETROK_KP_MINUS;
	case GP_KEY_KP_PLUS:
		return RETROK_KP_PLUS;
	case GP_KEY_KP_ENTER:
		return RETROK_KP_ENTER;
	case GP_KEY_KP_EQUAL:
		return RETROK_KP_EQUALS;

	case GP_KEY_UP:
		return RETROK_UP;
	case GP_KEY_DOWN:
		return RETROK_DOWN;
	case GP_KEY_RIGHT:
		return RETROK_RIGHT;
	case GP_KEY_LEFT:
		return RETROK_LEFT;
	case GP_KEY_INSERT:
		return RETROK_INSERT;
	case GP_KEY_HOME:
		return RETROK_HOME;
	case GP_KEY_END:
		return RETROK_END;
	case GP_KEY_PAGE_UP:
		return RETROK_PAGEUP;
	case GP_KEY_PAGE_DOWN:
		return RETROK_PAGEDOWN;

	case GP_KEY_F1:
		return RETROK_F1;
	case GP_KEY_F2:
		return RETROK_F2;
	case GP_KEY_F3:
		return RETROK_F3;
	case GP_KEY_F4:
		return RETROK_F4;
	case GP_KEY_F5:
		return RETROK_F5;
	case GP_KEY_F6:
		return RETROK_F6;
	case GP_KEY_F7:
		return RETROK_F7;
	case GP_KEY_F8:
		return RETROK_F8;
	case GP_KEY_F9:
		return RETROK_F9;
	case GP_KEY_F10:
		return RETROK_F10;
	case GP_KEY_F11:
		return RETROK_F11;
	case GP_KEY_F12:
		return RETROK_F12;
	case GP_KEY_F13:
		return RETROK_F13;
	case GP_KEY_F14:
		return RETROK_F14;
	case GP_KEY_F15:
		return RETROK_F15;

	case GP_KEY_RIGHT_SHIFT:
		return RETROK_RSHIFT;
	case GP_KEY_LEFT_SHIFT:
		return RETROK_LSHIFT;
	case GP_KEY_RIGHT_CTRL:
		return RETROK_RCTRL;
	case GP_KEY_LEFT_CTRL:
		return RETROK_LCTRL;
	case GP_KEY_LEFT_ALT:
		return RETROK_LALT;
	case GP_KEY_RIGHT_ALT:
		return RETROK_RALT;
	case GP_KEY_RIGHT_META:
		return RETROK_RMETA;
	case GP_KEY_LEFT_META:
		return RETROK_LMETA;

	default:
		return RETROK_UNKNOWN;
	}
}

#endif /* KEYMAP_H */
