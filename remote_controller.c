#include "remote_controller.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#define NUM_EVENTS  5

#define NON_STOP    1

/* error codes */
#define NO_ERROR 		0
#define ERROR			1

static int8_t callback_thread_running = 1;
static int32_t thread_exit_status;
static int32_t input_file_desc;
static int32_t (*callback)(uint16_t, uint16_t, uint32_t) = NULL;
static struct input_event* event_buf;
static pthread_t callback_thread;
//static pthread_cond_t exit_condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;

int32_t remote_init()
{
    const char* dev = "/dev/input/event0";
    char device_name[20];

    input_file_desc = open(dev, O_RDWR);
    if(input_file_desc == -1)
    {
        printf("Error while opening device (%s) !", strerror(errno));
	    return ERROR;
    }
    
    ioctl(input_file_desc, EVIOCGNAME(sizeof(device_name)), device_name);
	printf("RC device opened succesfully [%s]\n", device_name);
    
    event_buf = malloc(NUM_EVENTS * sizeof(struct input_event));
    if(!event_buf)
    {
        printf("Error allocating memory !");
        return ERROR;
    }
	
	return NO_ERROR;
}

int32_t remote_deinit()
{
	free(event_buf);

	return NO_ERROR;
}

int32_t remote_callback_register(int32_t (*function_pointer)(uint16_t, uint16_t, uint32_t))
{
	if(callback != NULL)
	{
		printf("Callback is already registered!\n");
		return ERROR;
	}
	callback = function_pointer;
	callback_thread_running = (uint8_t)1;
	pthread_create(&callback_thread, NULL, remote_loop, (void*)(&thread_exit_status));

	return NO_ERROR;
}

int32_t remote_callback_unregister()
{
	pthread_mutex_lock(&exit_mutex);
	callback_thread_running = (uint8_t)0;
    pthread_mutex_unlock(&exit_mutex);
	
	pthread_join(callback_thread, NULL);
	callback = NULL;

	return NO_ERROR;
}

static void* remote_loop(void* param)
{
	uint32_t event_cnt;
    uint32_t i;
	
	pthread_mutex_lock(&exit_mutex);
    while(callback_thread_running)
    {
		pthread_mutex_unlock(&exit_mutex);

        /* read input events */
        if(get_keys(NUM_EVENTS, (uint8_t*)event_buf, &event_cnt))
        {
			printf("Error while reading input events !");
			goto error_exit;
		}
		
        for(i = 0; i < event_cnt; i++)
        {
            /*printf("Event time: %d sec, %d usec\n",(int)event_buf[i].time.tv_sec,(int)event_buf[i].time.tv_usec);
            printf("Event type: %hu\n",event_buf[i].type);
            printf("Event code: %hu\n",event_buf[i].code);
            printf("Event value: %d\n",event_buf[i].value);
            printf("\n");*/
			callback(event_buf[i].code, event_buf[i].type, event_buf[i].value);       
		}
		pthread_mutex_lock(&exit_mutex);
    }
	pthread_mutex_unlock(&exit_mutex);

	*((uint32_t*)param) = NO_ERROR;
	return;

	error_exit:
	*((uint32_t*)param) = ERROR;
	return;
}

static int32_t get_keys(int32_t count, uint8_t* buf, int32_t* events_read)
{
    int32_t ret = 0;
    
    /* read input events and put them in buffer */
    ret = read(input_file_desc, buf, (size_t)(count * (int)sizeof(struct input_event)));
    if(ret <= 0)
    {
        printf("Error code %d", ret);
        return ERROR;
    }
    /* calculate number of read events */
    *events_read = ret / (int)sizeof(struct input_event);
    
    return NO_ERROR;
}
