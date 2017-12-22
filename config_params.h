#ifndef CONFIG_PARAMS_H
#define CONFIG_PARAMS_H

#include "tdp_api.h"

typedef struct _config_params{
	uint32_t frequency;
	uint32_t bandwidth;
	t_Module modulation;
	uint32_t audio_pid;
	uint32_t video_pid;
	tStreamType audio_type;
	tStreamType video_type;
	uint8_t program_number;
}config_params;

#endif
