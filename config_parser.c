#include "config_parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

int32_t parse_config_file(char* file_name, config_params* params)
{
	FILE* file_ptr;
	char* param;
	char* param_value;
	char config_line[50];
	uint8_t loaded_params[] = { 0, 0, 0, 0, 0, 0, 0, 0,};

	if((file_ptr = fopen(file_name,"r")) == NULL)
	{
		printf("\nFile could not be opened!\n");
		return ERROR;
	}

	while(fscanf(file_ptr,"%s",&config_line) != EOF)
	{
		if((param = strtok(config_line,"=")) == NULL)
		{
			printf("\nInvalid config file format or values missing!\n");
			return ERROR;
		}

		if((param_value = strtok(NULL,"=")) == NULL)
		{
			printf("\nInvalid config file format or values missing!\n");
			return ERROR;
		}		

		if(strcmp(param,"frequency") == 0)
		{
			if(loaded_params[0] == (uint8_t)0)
			{
				if ((params->frequency = atoi(param_value)) == 0)
				{
					printf("\nFrequency must be given as a number!\n");
					return ERROR;
				}
				loaded_params[0] = (uint8_t)1;
			}
			else
			{
				printf("\nFrequency declared more than once in config file!\n");
				return ERROR;
			}		
		}
		if(strcmp(param,"bandwidth") == 0)
		{
			if(loaded_params[1] == (uint8_t)0)
			{			
				if ((params->bandwidth = atoi(param_value)) == 0)
				{
					printf("\nBandwidth must be given as a number!\n");
					return ERROR;
				}
				loaded_params[1] = (uint8_t)1;
			}
			else
			{
				printf("\nBandwidth declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"modulation") == 0 && strcmp(param_value,"DVB_T") == 0)
		{
			if(loaded_params[2] == (uint8_t)0)
			{	
				params->modulation = DVB_T;
				loaded_params[2] = (uint8_t)1;
			}
			else
			{
				printf("\nModulation declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"audio_pid") == 0)
		{
			if(loaded_params[3] == (uint8_t)0)
			{
				if ((params->audio_pid = atoi(param_value)) == 0)
				{
					printf("\nAudio PID must be given as a number!\n");
					return ERROR;
				}
				loaded_params[3] = (uint8_t)1;
			}
			else
			{
				printf("\nAudio PID declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"video_pid") == 0)
		{
			if(loaded_params[4] == (uint8_t)0)
			{
				if ((params->video_pid = atoi(param_value)) == 0)
				{
					printf("\nVideo PID must be given as a number!\n");
					return ERROR;
				}
				loaded_params[4] = (uint8_t)1;
			}			
			else
			{
				printf("\nVideo PID declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"audio_type") == 0 && strcmp(param_value,"AUDIO_TYPE_MPEG_AUDIO") == 0)
		{
			if(loaded_params[5] == (uint8_t)0)
			{
				params->audio_type = AUDIO_TYPE_MPEG_AUDIO;
				loaded_params[5] = (uint8_t)1;
			}
			else
			{
				printf("\nAudio type declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"video_type") == 0 && strcmp(param_value,"VIDEO_TYPE_MPEG2") == 0)
		{
			if(loaded_params[6] == (uint8_t)0)
			{
				params->video_type = VIDEO_TYPE_MPEG2;
				loaded_params[6] = (uint8_t)1;
			}
			else
			{
				printf("\nVideo type declared more than once in config file!\n");
				return ERROR;
			}
		}
		if(strcmp(param,"program_number") == 0)
		{
			if(loaded_params[7] == (uint8_t)0)
			{
				if ((params->program_number = atoi(param_value)) == 0)
				{
					printf("\nProgram number must be given as a number!\n");
					return ERROR;
				}
				if (params->program_number < 1 || params->program_number > 7)
				{
					printf("\nProgram number must be in range 1-7!\n");
					return ERROR;
				}
				loaded_params[7] = (uint8_t)1;
			}
			else
			{
				printf("\nProgram number declared more than once in config file!\n");
				return ERROR;
			}
		}
	}

	if(fclose(file_ptr) == EOF)
	{
		printf("\nFile stream could not be closed!\n");
		return ERROR;
	}

	uint8_t checksum = loaded_params[0]+loaded_params[1]+loaded_params[2]+loaded_params[3]+loaded_params[4]+loaded_params[5]+loaded_params[6]+loaded_params[7];

	if(checksum != 8)
	{
		printf("\nConfig file not valid!\n");
		return ERROR;
	}

	printf("\nConfig file successfully parsed!\n");

	return NO_ERROR;
}
