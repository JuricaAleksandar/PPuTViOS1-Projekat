#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "errno.h"
#include "stream_controller.h"
#include "table_parser.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* Command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

static config_params* params;

/* program change timer */
static timer_t pr_change_timer_id;
static struct sigevent pr_change_signal_event;
static struct itimerspec pr_change_timer_spec;
static struct itimerspec pr_change_timer_spec_old;

/* status of audio and video stream */
static stream_status status;

/* callback functions for signaling graphics rendering thread */
static int32_t (*prog_info_callback)(uint16_t,uint16_t,uint16_t,uint8_t);
static int32_t (*ch_prog_callback)(uint16_t,uint8_t);
static int32_t (*bgd_callback)();
static int32_t (*prog_list_callback)(service_info*,uint16_t);
static stream_params str_prm;
static pthread_mutex_t stream_params_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t stream_thread_running = 1;
static pthread_t stream_thread;
static t_Error error;
static uint32_t pr_counter = 0; // current program
static uint32_t player_handle = 0;
static uint32_t source_handle = 0;
static uint32_t filter_handle = 0;
static uint32_t audio_stream_handle = 0;
static uint32_t video_stream_handle = 0;  
static pthread_cond_t status_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
static PAT_st PAT; // information from program allocation table
static SDT_st SDT; // information from service description table

static void* pr_change_timer()
{	
	memset(&pr_change_timer_spec,0,sizeof(pr_change_timer_spec));
	timer_settime(pr_change_timer_id,0,&pr_change_timer_spec,&pr_change_timer_spec_old);
	ch_prog_callback(pr_counter + 1, 0);

	return (void*)NO_ERROR;
}

/* tuner lock callback */
static int32_t tuner_status_callback(t_LockStatus lock_status)
{
    if (lock_status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&status_mutex);
        pthread_cond_signal(&status_condition);
        pthread_mutex_unlock(&status_mutex);
        printf("\n\n\tCALLBACK LOCKED\n\n");
    }
    else
    {
        printf("\n\n\tCALLBACK NOT LOCKED\n\n");
    }
    return NO_ERROR;
}

static int32_t filter_callback(uint8_t /* pointer to data filtered by demux */*buffer)
{	
	/* if table id is 0x0 parse data as PAT */
	if (buffer[0] == (uint8_t)0x0)
	{
		parse_PAT(buffer, &PAT);
	}
	/* if table id is 0x2 parse data as PMT */
	else if (buffer[0] == (uint8_t)0x2)
	{
		uint16_t j = 0;
		uint16_t prn = (((uint16_t)buffer[3])<<8) + *(buffer+4);
		while( prn != PAT.pmts[j].pr_num )
			j++;
		parse_PMT(buffer, PAT.pmts+j);
	}
	/* if table id is 0x42 parse data as SDT */
	else if (buffer[0] == (uint8_t)0x42)
	{
		parse_SDT(buffer, &SDT, PAT.pmt_count);
	}
	pthread_mutex_lock(&status_mutex);
	pthread_cond_signal(&status_condition);
	pthread_mutex_unlock(&status_mutex);
	return NO_ERROR;
}

static void* stream_loop(void* param)
{	
	while(stream_thread_running)
	{
		pthread_mutex_lock(&stream_params_mutex);

		if (str_prm.change_program)
		{
			bgd_callback(); // signal graphics to print black screen

			if (status.audio_on) // remove audio stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream removed");
				status.audio_on = 0;
			}
			if (status.video_on) // remove video stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Video stream removed");
				status.video_on = 0;
			}
	
			/* set new demux filter */
			error = Demux_Free_Filter(player_handle, filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux free filter");
	
			error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux set filter"); 

			pthread_mutex_lock(&status_mutex);
			pthread_cond_wait(&status_condition, &status_mutex);   
			pthread_mutex_unlock(&status_mutex);

			/* create player stream for video if video exists for current program */
			if (PAT.pmts[pr_counter].video_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, params->video_type, &video_stream_handle);     
				ASSERT_TDP_THREAD_RESULT(error, "Video stream created");
				status.video_on = 1;
				sleep(1);
			}

			/* create player stream for audio if audio exists for current program */
			if (PAT.pmts[pr_counter].audio_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, params->audio_type, &audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream created");
				status.audio_on = 1;
			}

			/* signal graphics controller to remove black screen and print program number */
			if (PAT.pmts[pr_counter].video_pid == 65000)
			{
				ch_prog_callback(pr_counter + 1, 1);
			}
			else
			{
				pr_change_timer_spec.it_value.tv_sec = 2;
				pr_change_timer_spec.it_value.tv_nsec = 400000000;
				timer_settime(pr_change_timer_id,0,&pr_change_timer_spec,&pr_change_timer_spec_old);
			}

			str_prm.change_program = 0;
		}
		if (str_prm.next_program == 1)
		{
			if (pr_counter + 1 == PAT.pmt_count)
				pr_counter = 0;
			else
				pr_counter++;	
	
			bgd_callback(); // signal graphics to print black screen

			if (status.audio_on == 1) // remove audio stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream removed");
				status.audio_on = 0;
			}
			if (status.video_on == 1) // remove video stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Video stream removed");
				status.video_on = 0;
			}
	
			/* set new demux filter */
			error = Demux_Free_Filter(player_handle, filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux free filter");
	
			error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux set filter");

			pthread_mutex_lock(&status_mutex);
			pthread_cond_wait(&status_condition, &status_mutex);   
			pthread_mutex_unlock(&status_mutex);

			/* create player stream for video if video exists for current program */
			if (PAT.pmts[pr_counter].video_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, params->video_type, &video_stream_handle);     
				ASSERT_TDP_THREAD_RESULT(error, "Video stream created");
				status.video_on = 1;
				sleep(1);
			}

			/* create player stream for audio if audio exists for current program */
			if (PAT.pmts[pr_counter].audio_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, params->audio_type, &audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream created");
				status.audio_on = 1;
			}

			/* signal graphics controller to remove black screen and print program number */
			if (PAT.pmts[pr_counter].video_pid == 65000)
			{
				ch_prog_callback(pr_counter + 1, 1);
			}
			else
			{
				pr_change_timer_spec.it_value.tv_sec = 2;
				pr_change_timer_spec.it_value.tv_nsec = 400000000;
				timer_settime(pr_change_timer_id,0,&pr_change_timer_spec,&pr_change_timer_spec_old);
			}

			str_prm.next_program = 0;
		}	
		if (str_prm.previous_program == 1)
		{
			if (pr_counter  == 0)
				pr_counter = PAT.pmt_count - 1;
			else
				pr_counter--;	
	
			bgd_callback(); // signal graphics to print black screen

			if (status.audio_on == 1) // remove audio stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream removed");
				status.audio_on = 0;
			}
			if (status.video_on == 1) // remove video stream if it exists
			{
				error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Video stream removed");
				status.video_on = 0;
			}

			/* set new demux filter */
			error = Demux_Free_Filter(player_handle, filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux free filter");
	
			error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux set filter");

			pthread_mutex_lock(&status_mutex);
			pthread_cond_wait(&status_condition, &status_mutex);   
			pthread_mutex_unlock(&status_mutex);

			/* create player stream for video if video exists for current program */
			if (PAT.pmts[pr_counter].video_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, params->video_type, &video_stream_handle);     
				ASSERT_TDP_THREAD_RESULT(error, "Video stream created");
				status.video_on = 1;
				sleep(1);
			}		

			/* create player stream for audio if audio exists for current program */
			if (PAT.pmts[pr_counter].audio_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, params->audio_type, &audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream created");
				status.audio_on = 1;
			}

			/* signal graphics controller to remove black screen and print program number */
			if (PAT.pmts[pr_counter].video_pid == 65000)
			{
				ch_prog_callback(pr_counter + 1, 1);
			}
			else
			{
				pr_change_timer_spec.it_value.tv_sec = 2;
				pr_change_timer_spec.it_value.tv_nsec = 400000000;
				timer_settime(pr_change_timer_id,0,&pr_change_timer_spec,&pr_change_timer_spec_old);
			}
			
			str_prm.previous_program = 0;
		}	
		if (str_prm.get_prog_info == 1)
		{
			/* signal graphics controller to show info banner */
			prog_info_callback(/* program number */PAT.pmts[pr_counter].pr_num,
								/* audio pid */PAT.pmts[pr_counter].audio_pid,
								/* video pid */PAT.pmts[pr_counter].video_pid,
								/* teletext indicator */PAT.pmts[pr_counter].has_teletext);
			str_prm.get_prog_info = 0;
		}
		if (str_prm.get_prog_list == 1)
		{
			/* signal graphics controller to show program list */
			prog_list_callback(/* list of programs (their names and types) */SDT.services,
											/* number of programs */PAT.pmt_count);
			str_prm.get_prog_list = 0;
		}
		pthread_mutex_unlock(&stream_params_mutex);
	}

	return (void*)NO_ERROR;
}

int32_t stream_init(int32_t (*callback1)(uint16_t,uint8_t),int32_t (*callback2)(),int32_t (*callback3)(uint16_t,uint16_t,uint16_t,uint8_t),int32_t (*callback4)(service_info*,uint16_t), config_params* conf_params)
{
	pr_change_signal_event.sigev_notify = SIGEV_THREAD;
	pr_change_signal_event.sigev_notify_function = (void*)pr_change_timer;
	pr_change_signal_event.sigev_value.sival_ptr = NULL;
	pr_change_signal_event.sigev_notify_attributes = NULL;	
	timer_create(/* system clock */ CLOCK_REALTIME,                
             /* timer config */ &pr_change_signal_event,                      
            /* variable address for timer id */ &pr_change_timer_id);

	/* set stream status */
	status.audio_on = 0;
	status.video_on = 0;

	stream_thread_running = 0;

	str_prm.next_program = 0;
	str_prm.previous_program = 0;
	str_prm.change_program = 0;
	str_prm.get_prog_info = 0;
	
	params = conf_params; 

	pr_counter = params->program_number - 1; // load program counter from config file

	ch_prog_callback = callback1; // to signal graphics controller about program change
	bgd_callback = callback2; // to signal graphics controller to print black screen
	prog_info_callback = callback3; // to signal graphics controller to print info banner
	prog_list_callback = callback4; // to signal graphics controller to print program list

    struct timespec lockStatusWaitTime;
    struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    /* Lock to frequency  (wait for tuner status notification)*/
	error = Tuner_Init();
    ASSERT_TDP_RESULT(error,"Tuner initiallization -");
	
	/* register tuner lock callback */
	error = Tuner_Register_Status_Callback(tuner_status_callback);
	ASSERT_TDP_RESULT(error,"Callback registration -");

	error = Tuner_Lock_To_Frequency(params->frequency, params->bandwidth, params->modulation);
	ASSERT_TDP_RESULT(error,"Locking -");
	
	pthread_mutex_lock(&status_mutex);
	if (ETIMEDOUT == pthread_cond_timedwait(&status_condition, &status_mutex, &lockStatusWaitTime))
    {
        printf("\n\nLock timeout exceeded!\n\n");
        return ERROR;
    }
    pthread_mutex_unlock(&status_mutex);

	error = Player_Init(&player_handle);
	ASSERT_TDP_RESULT(error,"Player init");
    
	error = Player_Source_Open(player_handle,&source_handle);
	ASSERT_TDP_RESULT(error,"Player source open");

	/* set filter to demux(filter PAT) */
	error = Demux_Set_Filter(player_handle, 0x0000, 0x00, &filter_handle);
    ASSERT_TDP_RESULT(error, "Demux set filter");
	
	/* set callback to demux */
	error = Demux_Register_Section_Filter_Callback(filter_callback);
    ASSERT_TDP_RESULT(error, "Demux register section filter callback");
    
    pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);

	/* set new filter to demux(filter SDT) */
	error = Demux_Free_Filter(player_handle, filter_handle);
	ASSERT_TDP_RESULT(error, "Demux free filter");
	
	error = Demux_Set_Filter(player_handle, 0x11, 0x42 , &filter_handle);
	ASSERT_TDP_RESULT(error, "Demux set filter");

	pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);

	/* set new filter to demux(filter PMT) */
	error = Demux_Free_Filter(player_handle, filter_handle);
	ASSERT_TDP_RESULT(error, "Demux free filter");
	
	error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
	ASSERT_TDP_RESULT(error, "Demux set filter");

	pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);
	
	/* if config file pids do not match pids received in pmt program exits */
	if (params->audio_pid != PAT.pmts[pr_counter].audio_pid)
	{
		printf("\nInvalid audio pid!\n");
		return ERROR;
	}

	if (params->video_pid != PAT.pmts[pr_counter].video_pid && PAT.pmts[pr_counter].video_pid != 65000)
	{
		printf("\nInvalid video pid!\n");
		return ERROR;
	}

	/* create player stream for audio if audio exists for current program */
	if (PAT.pmts[pr_counter].audio_pid != 65000)
	{
		error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, params->audio_type, &audio_stream_handle);
		ASSERT_TDP_RESULT(error, "Audio stream created");
		status.audio_on = 1;
	}

	/* create player stream for video if video exists for current program */
	if (PAT.pmts[pr_counter].video_pid != 65000)
	{
		error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, params->video_type, &video_stream_handle);     
		ASSERT_TDP_RESULT(error, "Video stream created");
		status.video_on = 1;
	}
	
	stream_thread_running = 1;	

	/* create stream controller thread */
	if (pthread_create(&stream_thread, NULL, stream_loop, NULL))
	{
		printf("\nFailed to create stream controller thread!\n");
		stream_thread_running = 0;
		return ERROR;
	}

	/* signal graphics controller to print program number */
	if (PAT.pmts[pr_counter].video_pid == 65000)
	{
		ch_prog_callback(pr_counter + 1, 1);
	}
	else
	{
		ch_prog_callback(pr_counter + 1, 0);
	}
}

int32_t stream_deinit()
{  
	uint8_t i;
	if (stream_thread_running)
	{
		stream_thread_running = 0;

		void* retVal;

		if (pthread_join(stream_thread,&retVal))
		{
			printf("\nFailed to join stream controller thread!\n");
			return ERROR;
		}

		if ((int32_t)retVal == ERROR)
		{
			printf("\nError in stream controller thread!\n");
			return ERROR;
		}
	}
	/* remove player audio stream if it exists */
	if (status.audio_on)
	{
		error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
		ASSERT_TDP_RESULT(error, "Audio stream removed");
		status.audio_on = 0;
	}

	/* remove player video stream if it exists */
	if (status.video_on)
	{
		error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
		ASSERT_TDP_RESULT(error, "Video stream removed");
		status.video_on = 0;
	}
	 /* Free demux filter */
    error = Demux_Free_Filter(player_handle, filter_handle);
    ASSERT_TDP_RESULT(error, "Demux free filter");

    /* Close previously opened source */
    error = Player_Source_Close(player_handle, source_handle);
    ASSERT_TDP_RESULT(error, "Player source close");
    
    /* Deinit player */
    error = Player_Deinit(player_handle);
    ASSERT_TDP_RESULT(error, "Player deinit");
    
    /* Deinit tuner */
    error = Tuner_Deinit();
    ASSERT_TDP_RESULT(error, "Tuner deinit");

	free(PAT.pmts);
	
	for(i = 0; i < PAT.pmt_count; i++)
	{
		free(SDT.services[i].service_name);
	}
     
	free(SDT.services);

    return NO_ERROR;
}

int32_t change_program(uint8_t program)
{
	pthread_mutex_lock(&stream_params_mutex);
	if (program != pr_counter)
	{
		str_prm.change_program = 1;
		pr_counter = program;
	}
	else
	{
		if (PAT.pmts[pr_counter].video_pid == 65000)
		{
			ch_prog_callback(pr_counter + 1, 1);
		}
		else
		{
			ch_prog_callback(pr_counter + 1, 0);
		}
	}
	pthread_mutex_unlock(&stream_params_mutex);

	return NO_ERROR;
}

/* set flag for switching to next program */
int32_t next_program()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.next_program = 1;
	pthread_mutex_unlock(&stream_params_mutex);

	return NO_ERROR;
}

/* set flag for switching to previous program */
int32_t previous_program()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.previous_program = 1;
	pthread_mutex_unlock(&stream_params_mutex);

	return NO_ERROR;
}

/* set volume level */
int32_t set_volume(uint32_t volume)
{
	error = Player_Volume_Set(player_handle, volume*160400000);	
	ASSERT_TDP_RESULT(error, "Volume set");

	return NO_ERROR;
}

int32_t get_prog_info()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.get_prog_info = 1;
	pthread_mutex_unlock(&stream_params_mutex);
	
	return NO_ERROR;
}

int32_t get_prog_list()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.get_prog_list = 1;
	pthread_mutex_unlock(&stream_params_mutex);

	return NO_ERROR;
}
