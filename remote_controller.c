#include "remote_controller.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#define ERROR -1
#define NO_ERROR 0

/* remote controller thread flag: 1-running 0-stopped */
static int8_t rc_thread_running = 0;
static int32_t input_file_desc;
static int32_t (*callback)(uint16_t, uint16_t, uint32_t) = NULL;
static struct input_event* event_buf;
static pthread_t callback_thread;

static void* remote_loop()
{
    while(rc_thread_running)
    {
	    /* read input events */
	    if(get_keys((uint8_t*)event_buf))
	    {
			printf("\nError while reading input events!\n");
			return (void*)ERROR;
		}
	
		/* callback is used here if registered */
		if(callback != NULL)
		{
			callback(/* input event code */event_buf->code,
					/* input event type */event_buf->type,
					/* input event value */event_buf->value);       
		}
	}

	return (void*)NO_ERROR;
}

int32_t get_keys(uint8_t* buf)
{
    int32_t ret = 0;
    /* read next input event and put it in buffer */
    ret = read(input_file_desc, buf, (size_t)(sizeof(struct input_event)));

    if(ret <= 0)
    {
        printf("Error code %d", ret);
        return ERROR;
    }
    
    return NO_ERROR;
}

int32_t remote_init()
{
    const char* dev = "/dev/input/event0";
    char device_name[20];

    input_file_desc = open(dev, O_RDWR);
    if(input_file_desc == -1)
    {
        printf("\nError while opening device (%s)!\n", strerror(errno));
	    return ERROR;
    }
    
    ioctl(input_file_desc, EVIOCGNAME(sizeof(device_name)), device_name);
	printf("\nRC device opened succesfully [%s]\n", device_name);
    
    event_buf = (input_event*)malloc(sizeof(struct input_event));
    if(!event_buf)
    {
        printf("\nError allocating memory!\n");
        return ERROR;
    }
	
	rc_thread_running = (uint8_t)1;
	if (pthread_create(&callback_thread, NULL, &remote_loop, NULL) != 0)
	{
		rc_thread_running = (uint8_t)0;
		printf("\nFailed to create remote controller thread!\n");
		return ERROR;
	}

	printf("\nRemote controller initialized!\n");

	return NO_ERROR;
}

int32_t remote_deinit()
{	
	if(rc_thread_running == 1)
	{
		rc_thread_running = (uint8_t)0;
	
		void* retVal;

		if(pthread_join(callback_thread,&retVal))
		{
			printf("\nFailed to join remote controller thread!\n");
			return ERROR;
		}

		if((int32_t)retVal == ERROR)
		{
			printf("\nError in remote controller thread!\n");
			return ERROR;
		}
	}
	
	free(event_buf);
	
	printf("\nRemote controller deinitialized!\n");

	return NO_ERROR;
}

int32_t remote_callback_register(int32_t (*function_pointer)(uint16_t, uint16_t, uint32_t))
{
	if(callback != NULL)
	{
		printf("\nCallback is already registered!\n");
		return ERROR;
	}
	callback = function_pointer;

	printf("\nRemote controller callback successfully registered!\n");

	return NO_ERROR;
}

int32_t remote_callback_unregister()
{
	if(callback == NULL)
	{
		printf("\nNo callback registered!\n");
		return ERROR;
	}

	callback = NULL;

	printf("\nRemote controller callback successfully unregistered!\n");

	return NO_ERROR;
}
