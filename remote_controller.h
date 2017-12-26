#ifndef RC_H
#define RC_H

#include <stdint.h>
#include <linux/input.h>

/* input key codes */
#define KEYCODE_EXIT 102
#define KEYCODE_P_PLUS 62
#define KEYCODE_P_MINUS 61
#define KEYCODE_V_PLUS 63
#define KEYCODE_V_MINUS 64
#define KEYCODE_V_MUTE 60
#define KEYCODE_INFO 358
#define KEYCODE_OK 28
#define KEYCODE_BACK 1
#define KEYCODE_P_1 2
#define KEYCODE_P_2 3
#define KEYCODE_P_3 4
#define KEYCODE_P_4 5
#define KEYCODE_P_5 6
#define KEYCODE_P_6 7
#define KEYCODE_P_7 8

/* input event values for 'EV_KEY' type */
#define EV_VALUE_RELEASE    0
#define EV_VALUE_KEYPRESS   1
#define EV_VALUE_AUTOREPEAT 2

int32_t remote_init();
int32_t remote_deinit();
int32_t remote_callback_register(int32_t (*function_pointer)(uint16_t, uint16_t, uint32_t));
int32_t remote_callback_unregister();

#endif
