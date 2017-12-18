#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "errno.h"
#include "stream_controller.h"
#include "tables.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* Command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

static stream_status status;
static int32_t (*prog_info_callback)(uint16_t,uint16_t,uint16_t,uint8_t);
static int32_t (*ch_prog_callback)(uint16_t,uint8_t);
static int32_t (*bgd_callback)();
static stream_params str_prm;
static pthread_mutex_t stream_params_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stream_exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t stream_thread_running = 1;
static pthread_t stream_thread;
static t_Error error;
static uint32_t pr_counter = 0;
static uint32_t player_handle = 0;
static uint32_t source_handle = 0;
static uint32_t filter_handle = 0;
static uint32_t audio_stream_handle = 0;
static uint32_t video_stream_handle = 0;  
static pthread_cond_t status_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
static PAT_st PAT;
static SDT_st SDT;

static int32_t tuner_status_callback(t_LockStatus status)
{
    if(status == STATUS_LOCKED)
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
    return 0;
}

static int32_t filter_callback(uint8_t *buffer)
{	
	if( buffer[0] == (uint8_t)0x0)
	{
		uint16_t i;
		uint8_t j = 0;
		uint8_t prog_count;
		PAT.section_length = (((uint16_t)(buffer[1] & 0x0F))<<8)+buffer[2];
		prog_count = (PAT.section_length-9)/4-1;
		PAT.pmt_count = prog_count;
		PAT.pmts = (PMT_st*)malloc(sizeof(PMT_st)*prog_count);
		for( i = 0; i < (uint16_t)prog_count + 1; i++ )
		{
			uint8_t* tmp_ptr = buffer + 8 + (i * 4);
			if( *(tmp_ptr) != 0 )
			{
				uint8_t first_byte = *tmp_ptr;
				uint8_t second_byte = *(tmp_ptr+1);
				uint8_t third_byte = *(tmp_ptr+2);
				uint8_t fourth_byte = *(tmp_ptr+3);
				PAT.pmts[j].pr_num = (uint16_t)((first_byte<<8) + second_byte);
				PAT.pmts[j].table_id = (uint16_t)((third_byte<<8) + fourth_byte) & 0x1FFF;
				printf("Program number: %u , Table id: %u\n",PAT.pmts[j].pr_num,PAT.pmts[j].table_id);
				j++;
			}
		}
	}
	else if( buffer[0] == (uint8_t)0x2 )
	{
		uint16_t j = 0;
		uint16_t sl;
		uint16_t prn = (((uint16_t)buffer[3])<<8) + *(buffer+4);
		while( prn != PAT.pmts[j].pr_num )
			j++;
		PAT.pmts[j].section_length = (((uint16_t)(buffer[1] & 0x0F))<<8) + buffer[2];
		PAT.pmts[j].video_pid = 65000;
		PAT.pmts[j].audio_pid = 65000;
		PAT.pmts[j].has_teletext = 0;
		sl = PAT.pmts[j].section_length - 13;
		uint8_t* tmp_ptr = buffer + 12;	
		while( sl != 0)
		{	
			if( (*tmp_ptr == 2 || *tmp_ptr == 1) && PAT.pmts[j].video_pid == 65000 )
			{
				PAT.pmts[j].video_pid = ((((uint16_t)tmp_ptr[1])<<8) + (uint16_t)tmp_ptr[2]) & 0x1FFF;
			}
			if ((*tmp_ptr == 3 || *tmp_ptr == 4) && PAT.pmts[j].audio_pid == 65000)
			{
				PAT.pmts[j].audio_pid = ((((uint16_t)tmp_ptr[1])<<8) + (uint16_t)*(tmp_ptr+2)) & 0x1FFF;	
			}
			if(*tmp_ptr == 6)
			{
				PAT.pmts[j].has_teletext = 1;
			}
			uint16_t ES_info_length = ((((uint16_t)tmp_ptr[3])<<8) + (uint16_t)tmp_ptr[4]) & 0x0FFF;
			sl = sl - ES_info_length - 5;
			tmp_ptr = tmp_ptr + ES_info_length + 5;
		}
	}
	else if( buffer[0] == (uint8_t)0x42 )
	{
		uint16_t j = 0;
		uint16_t sl;
		SDT.section_length = (((uint16_t)(buffer[1] & 0x0F))<<8) + buffer[2];
		sl = SDT.section_length - 12;
		uint8_t* tmp_ptr = buffer + 11;	
		SDT.services = (service_info*)malloc(PAT.pmt_count*sizeof(service_info));
		while(sl != 0)
		{
			SDT.services[j].service_id = (((uint16_t)tmp_ptr[0])<<8) + tmp_ptr[1];
			SDT.services[j].descriptors_loop_length = ((((uint16_t)tmp_ptr[3])<<8) + tmp_ptr[4]) & 0x0FFF;
			SDT.services[j].service_type = tmp_ptr[7];

			uint8_t provider_len = tmp_ptr[8];
			uint8_t service_name_len = tmp_ptr[9+provider_len];			
	
			SDT.services[j].service_name = (char*)malloc((service_name_len+1)*sizeof(char));		
			uint16_t i;
			for( i = 0; i < service_name_len; i++)
			{
				SDT.services[j].service_name[i] = tmp_ptr[10+provider_len+i];
			}
			SDT.services[j].service_name[service_name_len] = 0;
			sl = sl - SDT.services[j].descriptors_loop_length - 5;
			tmp_ptr = tmp_ptr + SDT.services[j].descriptors_loop_length + 5;
			printf("Service name: %s, Service type: %u\n",SDT.services[j].service_name,SDT.services[j].service_type);
			j++;
		}
	}
	pthread_mutex_lock(&status_mutex);
	pthread_cond_signal(&status_condition);
	pthread_mutex_unlock(&status_mutex);
	return 0;
}

int32_t stream_init(int32_t (*callback1)(uint16_t,uint8_t),int32_t (*callback2)(),int32_t (*callback3)(uint16_t,uint16_t,uint16_t,uint8_t))
{
	str_prm.next_program = 0;
	str_prm.previous_program = 0;
	str_prm.change_program = 0;
	str_prm.get_prog_info = 0;
	str_prm.set_volume = 0;
	
	ch_prog_callback = callback1;
	bgd_callback = callback2;
	prog_info_callback = callback3;

    struct timespec lockStatusWaitTime;
    struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    /* Lock to frequency  (wait for tuner status notification)*/
	error = Tuner_Init();
    ASSERT_TDP_RESULT(error,"Tuner initiallization -");
	
	error = Tuner_Register_Status_Callback(tuner_status_callback);
	ASSERT_TDP_RESULT(error,"Callback registration -");

	error = Tuner_Lock_To_Frequency(754000000, 8, DVB_T);
	ASSERT_TDP_RESULT(error,"Locking -");
	
	pthread_mutex_lock(&status_mutex);
	if(ETIMEDOUT == pthread_cond_timedwait(&status_condition, &status_mutex, &lockStatusWaitTime))
    {
        printf("\n\nLock timeout exceeded!\n\n");
        return -1;
    }
    pthread_mutex_unlock(&status_mutex);

    /* Set filter to demux */
    
	error = Player_Init(&player_handle);
	ASSERT_TDP_RESULT(error,"Player init");
    
	error = Player_Source_Open(player_handle,&source_handle);
	ASSERT_TDP_RESULT(error,"Player source open");

	error = Demux_Set_Filter(player_handle, 0x0000, 0x00, &filter_handle);
    ASSERT_TDP_RESULT(error, "Demux set filter");
	
	error = Demux_Register_Section_Filter_Callback(filter_callback);
    ASSERT_TDP_RESULT(error, "Demux register section filter callback");
    
    pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);

	error = Demux_Free_Filter(player_handle, filter_handle);
	ASSERT_TDP_RESULT(error, "Demux free filter");
	
	error = Demux_Set_Filter(player_handle, 0x11, 0x42 , &filter_handle);
	ASSERT_TDP_RESULT(error, "Demux set filter");

	pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);

	error = Demux_Free_Filter(player_handle, filter_handle);
	ASSERT_TDP_RESULT(error, "Demux free filter");
	
	error = Demux_Set_Filter(player_handle, PAT.pmts[0].table_id, 0x02 , &filter_handle);
	ASSERT_TDP_RESULT(error, "Demux set filter");


	pthread_mutex_lock(&status_mutex);
    pthread_cond_wait(&status_condition, &status_mutex);   
	pthread_mutex_unlock(&status_mutex);

	error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, AUDIO_TYPE_MPEG_AUDIO, &audio_stream_handle);
	ASSERT_TDP_RESULT(error, "Audio stream created");

	error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, VIDEO_TYPE_MPEG2, &video_stream_handle);     
	ASSERT_TDP_RESULT(error, "Video stream created");

	status.audio_on = 1;
	status.video_on = 1;

	stream_thread_running = 1;
	pthread_create(&stream_thread, NULL, stream_loop, NULL);
}

int32_t stream_deinit()
{  
	pthread_mutex_lock(&stream_exit_mutex);
	stream_thread_running = 0;
	pthread_mutex_unlock(&stream_exit_mutex);
	pthread_join(stream_thread, NULL);

	if(status.audio_on == 1)
	{
		error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
		ASSERT_TDP_RESULT(error, "Audio stream removed");
		status.audio_on = 0;
	}
	if(status.video_on == 1)
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
     
    return 0;
}

static void* stream_loop(void* param)
{	
	pthread_mutex_lock(&stream_exit_mutex);
	while(stream_thread_running)
	{
		pthread_mutex_unlock(&stream_exit_mutex);

		pthread_mutex_lock(&stream_params_mutex);

		if(str_prm.next_program == 1)
		{
			if(pr_counter + 1 == PAT.pmt_count)
				pr_counter = 0;
			else
				pr_counter++;	
	
			bgd_callback();

			if(status.audio_on == 1)
			{
				error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream removed");
				status.audio_on = 0;
			}
			if(status.video_on == 1)
			{
				error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Video stream removed");
				status.video_on = 0;
			}
	
			error = Demux_Free_Filter(player_handle, filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux free filter");
	
			error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux set filter");

			pthread_mutex_lock(&status_mutex);
			pthread_cond_wait(&status_condition, &status_mutex);   
			pthread_mutex_unlock(&status_mutex);

			uint8_t radio = 0;
			if(PAT.pmts[pr_counter].video_pid == 65000)
				radio = 1;
			ch_prog_callback(pr_counter + 1, radio);

			if(PAT.pmts[pr_counter].audio_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, AUDIO_TYPE_MPEG_AUDIO, &audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream created");
				status.audio_on = 1;
			}
			if(PAT.pmts[pr_counter].video_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, VIDEO_TYPE_MPEG2, &video_stream_handle);     
				ASSERT_TDP_THREAD_RESULT(error, "Video stream created");
				status.video_on = 1;
			}

			str_prm.next_program = 0;
		}	
		if(str_prm.previous_program == 1)
		{
			if(pr_counter  == 0)
				pr_counter = PAT.pmt_count - 1;
			else
				pr_counter--;	
	
			bgd_callback();

			if(status.audio_on == 1)
			{
				error = Player_Stream_Remove(player_handle, source_handle, audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream removed");
				status.audio_on = 0;
			}
			if(status.video_on == 1)
			{
				error = Player_Stream_Remove(player_handle, source_handle, video_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Video stream removed");
				status.video_on = 0;
			}

			error = Demux_Free_Filter(player_handle, filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux free filter");
	
			error = Demux_Set_Filter(player_handle, PAT.pmts[pr_counter].table_id, 0x02 , &filter_handle);
			ASSERT_TDP_THREAD_RESULT(error, "Demux set filter");

			pthread_mutex_lock(&status_mutex);
			pthread_cond_wait(&status_condition, &status_mutex);   
			pthread_mutex_unlock(&status_mutex);

			uint8_t radio = 0;
			if(PAT.pmts[pr_counter].video_pid == 65000)
				radio = 1;
			ch_prog_callback(pr_counter + 1, radio);

			if(PAT.pmts[pr_counter].audio_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].audio_pid, AUDIO_TYPE_MPEG_AUDIO, &audio_stream_handle);
				ASSERT_TDP_THREAD_RESULT(error, "Audio stream created");
				status.audio_on = 1;
			}
			if(PAT.pmts[pr_counter].video_pid != 65000)
			{
				error = Player_Stream_Create(player_handle, source_handle, PAT.pmts[pr_counter].video_pid, VIDEO_TYPE_MPEG2, &video_stream_handle);     
				ASSERT_TDP_THREAD_RESULT(error, "Video stream created");
				status.video_on = 1;
			}
			
			str_prm.previous_program = 0;
		}	
		if(str_prm.get_prog_info == 1)
		{
			prog_info_callback(PAT.pmts[pr_counter].pr_num, PAT.pmts[pr_counter].audio_pid, PAT.pmts[pr_counter].video_pid, PAT.pmts[pr_counter].has_teletext);
			str_prm.get_prog_info = 0;
		}
		pthread_mutex_unlock(&stream_params_mutex);
	
		pthread_mutex_lock(&stream_exit_mutex);
	}
	pthread_mutex_unlock(&stream_exit_mutex);

	return (void*)0;
}

int32_t next_program()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.next_program = 1;
	pthread_mutex_unlock(&stream_params_mutex);

	return 0;
}

int32_t previous_program()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.previous_program = 1;
	pthread_mutex_unlock(&stream_params_mutex);

	return 0;
}

int32_t set_volume(uint32_t volume)
{
	error = Player_Volume_Set(player_handle, volume*160400000);	
	ASSERT_TDP_RESULT(error, "Volume set");

	return 0;
}

int32_t get_prog_info()
{
	pthread_mutex_lock(&stream_params_mutex);
	str_prm.get_prog_info = 1;
	pthread_mutex_unlock(&stream_params_mutex);
	
	return 0;
}