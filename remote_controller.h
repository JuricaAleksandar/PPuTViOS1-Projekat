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

/* input event values for 'EV_KEY' type */
#define EV_VALUE_RELEASE    0
#define EV_VALUE_KEYPRESS   1
#define EV_VALUE_AUTOREPEAT 2

int32_t remote_init();
int32_t remote_deinit();
int32_t remote_callback_register(int32_t (*function_pointer)(uint16_t, uint16_t, uint32_t));
int32_t remote_callback_unregister();
static void* remote_loop(void* param);
static int32_t get_keys(int32_t count, uint8_t* buf, int32_t* event_read);

#endif
