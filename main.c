#include "remote_controller.h"
#include "graphics_controller.h"
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

static uint8_t volume_lvl = 0;
static uint8_t volume_mute = 1;
static pthread_cond_t zapper_exit_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t zapper_exit_mutex = PTHREAD_MUTEX_INITIALIZER;

int32_t callback(uint16_t code, uint16_t type, uint32_t value)
{
	if(code == KEYCODE_P_1 && value == EV_VALUE_KEYPRESS)
	{
		change_program(0);
	}
	if(code == KEYCODE_P_2 && value == EV_VALUE_KEYPRESS)
	{
		change_program(1);
	}
	if(code == KEYCODE_P_3 && value == EV_VALUE_KEYPRESS)
	{
		change_program(2);
	}
	if(code == KEYCODE_P_4 && value == EV_VALUE_KEYPRESS)
	{
		change_program(3);
	}
	if(code == KEYCODE_P_5 && value == EV_VALUE_KEYPRESS)
	{
		change_program(4);
	}
	if(code == KEYCODE_P_6 && value == EV_VALUE_KEYPRESS)
	{
		change_program(5);
	}
	if(code == KEYCODE_P_7 && value == EV_VALUE_KEYPRESS)
	{
		change_program(6);
	}
	if(code == KEYCODE_P_PLUS && value == EV_VALUE_KEYPRESS)
	{
		next_program();
	}
	if(code == KEYCODE_P_MINUS && value == EV_VALUE_KEYPRESS)
	{
		previous_program();	
	}
	if(code == KEYCODE_INFO && value == EV_VALUE_KEYPRESS)
	{
		get_prog_info();
	}
	if(code == KEYCODE_OK && value == EV_VALUE_KEYPRESS)
	{
		get_prog_list();
	}
	if(code == KEYCODE_BACK && value == EV_VALUE_KEYPRESS)
	{
		remove_prog_list();
	}
	if(code == KEYCODE_V_PLUS && value == EV_VALUE_KEYPRESS)
	{
		if(volume_lvl < 10)
			volume_lvl++;
		set_volume((uint32_t)volume_lvl);
		print_volume((uint32_t)volume_lvl);
	}
	if(code == KEYCODE_V_MINUS && value == EV_VALUE_KEYPRESS)
	{
		if(volume_lvl > 0)
			volume_lvl--;
		set_volume((uint32_t)volume_lvl);
		print_volume((uint32_t)volume_lvl);
	}
	if(code == KEYCODE_V_MUTE && value == EV_VALUE_KEYPRESS)
	{
		volume_mute = 1 - volume_mute;
		set_volume((uint32_t)volume_lvl*volume_mute); 
		print_volume((uint32_t)volume_lvl*volume_mute);
	}
	if(code == KEYCODE_EXIT && value == EV_VALUE_KEYPRESS)
	{
		pthread_cond_signal(&zapper_exit_condition);
	}
	return 0;
}

int32_t main(int32_t argc, char** argv)
{
	remote_init();
	remote_callback_register(callback);
	stream_init(print_prog_num,print_black_screen,print_info_banner,print_prog_list);
	graphics_init();
	pthread_cond_wait(&zapper_exit_condition, &zapper_exit_mutex);	
	graphics_deinit();
	stream_deinit();
	remote_callback_unregister();
	remote_deinit();
	return 0;
}
