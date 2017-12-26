#include "remote_controller.h"
#include "graphics_controller.h"
#include "config_parser.h"
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

static uint8_t volume_lvl = 5;
static uint8_t volume_mute = 1;
static pthread_cond_t zapper_exit_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t zapper_exit_mutex = PTHREAD_MUTEX_INITIALIZER;

/* remote controller callback */
int32_t callback(uint16_t code, uint16_t type, uint32_t value)
{
	if (value == EV_VALUE_KEYPRESS)
	{
		switch (code)
		{
			case KEYCODE_P_1:
				change_program(0);
				break;
			case KEYCODE_P_2:
				change_program(1);
				break;
			case KEYCODE_P_3:
				change_program(2);
				break;
			case KEYCODE_P_4:
				change_program(3);
				break;
			case KEYCODE_P_5:
				change_program(4);
				break;
			case KEYCODE_P_6:
				change_program(5);
				break;
			case KEYCODE_P_7:
				change_program(6);
				break;
			case KEYCODE_P_PLUS:
				next_program();
				break;
			case KEYCODE_P_MINUS:
				previous_program();
				break;
			case KEYCODE_INFO:
				get_prog_info();
				break;
			case KEYCODE_OK:
				get_prog_list();
				break;
			case KEYCODE_BACK:
				remove_prog_list();
				break;
			case KEYCODE_V_PLUS:
				if(volume_lvl < 10)
					volume_lvl++;
				set_volume((uint32_t)volume_lvl);
				print_volume((uint32_t)volume_lvl);
				break;
			case KEYCODE_V_MINUS:
				if(volume_lvl > 0)
					volume_lvl--;
				set_volume((uint32_t)volume_lvl);
				print_volume((uint32_t)volume_lvl);
			case KEYCODE_V_MUTE:
				volume_mute = 1 - volume_mute;
				set_volume((uint32_t)volume_lvl*volume_mute); 
				print_volume((uint32_t)volume_lvl*volume_mute);
				break;
			case KEYCODE_EXIT:
				pthread_mutex_lock(&zapper_exit_mutex);
				pthread_cond_signal(&zapper_exit_condition);
				pthread_mutex_unlock(&zapper_exit_mutex);
				break;
		}
	}

	return NO_ERROR;
}

int32_t main(int32_t argc, char** argv)
{
	int32_t error = NO_ERROR;
	config_params params;
	if (argc != 2)
	{
		printf("\nConfig file path not given!\n");
		return ERROR;
	}
	if (error = parse_config_file(argv[1], &params))
	{
		return error;
	}
	if (error = remote_init())
	{
		return error;
	}
	if (error = remote_callback_register(callback))
	{
		goto rc_deinit;
	}
	if (error = stream_init(print_prog_num, print_black_screen, print_info_banner, print_prog_list, &params))
	{
		goto str_deinit;
	}

	if (error = graphics_init())
	{
		goto graph_deinit;
	}

	pthread_mutex_lock(&zapper_exit_mutex);
	pthread_cond_wait(&zapper_exit_condition, &zapper_exit_mutex);
	pthread_mutex_unlock(&zapper_exit_mutex);	
	
	graph_deinit:
	if (graphics_deinit())
	{
		return ERROR;
	}

	str_deinit:
	if (stream_deinit())
	{
		return ERROR;
	}

	if (remote_callback_unregister())
	{
		return ERROR;
	}

	rc_deinit:
	if (remote_deinit())
	{
		return ERROR;
	}

	return error;
}
