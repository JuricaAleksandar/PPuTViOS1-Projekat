#include <stdio.h>
#include <directfb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "graphics_controller.h"

/* volume timer */
static timer_t volume_timer_id;
static struct sigevent volume_signal_event;
static struct itimerspec volume_timer_spec;
static struct itimerspec volume_timer_spec_old;

/* program number timer */
static timer_t pr_num_timer_id;
static struct sigevent pr_num_signal_event;
static struct itimerspec pr_num_timer_spec;
static struct itimerspec pr_num_timer_spec_old;

/* info timer */
static timer_t info_timer_id;
static struct sigevent info_signal_event;
static struct itimerspec info_timer_spec;
static struct itimerspec info_timer_spec_old;

static int32_t timer_flags = 0;

static uint8_t wait = 0;
static flags_and_params render_fnp;
static pthread_mutex_t render_fnp_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t render_exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t render_thread_running = 1;
static pthread_t render_thread;
static IDirectFBSurface *primary_surface = NULL;
static IDirectFB *dfb_interface = NULL;
static int screen_width = 0;
static int screen_height = 0;
static DFBSurfaceDescription surface_desc;

int32_t graphics_init()
{
	volume_signal_event.sigev_notify = SIGEV_THREAD;
	volume_signal_event.sigev_notify_function = (void*)volume_timer;
	volume_signal_event.sigev_value.sival_ptr = NULL;
	volume_signal_event.sigev_notify_attributes = NULL;	
	timer_create(/*sistemski sat za merenje vremena*/ CLOCK_REALTIME,                
             /*podešavanja timer-a*/ &volume_signal_event,                      
            /*mesto gde će se smestiti ID novog timer-a*/ &volume_timer_id);
	
	info_signal_event.sigev_notify = SIGEV_THREAD;
	info_signal_event.sigev_notify_function = (void*)info_timer;
	info_signal_event.sigev_value.sival_ptr = NULL;
	info_signal_event.sigev_notify_attributes = NULL;	
	timer_create(/*sistemski sat za merenje vremena*/ CLOCK_REALTIME,                
             /*podešavanja timer-a*/ &info_signal_event,                      
            /*mesto gde će se smestiti ID novog timer-a*/ &info_timer_id);		

	pr_num_signal_event.sigev_notify = SIGEV_THREAD;
	pr_num_signal_event.sigev_notify_function = (void*)pr_num_timer;
	pr_num_signal_event.sigev_value.sival_ptr = NULL;
	pr_num_signal_event.sigev_notify_attributes = NULL;	
	timer_create(/*sistemski sat za merenje vremena*/ CLOCK_REALTIME,                
             /*podešavanja timer-a*/ &pr_num_signal_event,                      
            /*mesto gde će se smestiti ID novog timer-a*/ &pr_num_timer_id);	

    /* initialize DirectFB */   
	DFBCHECK(DirectFBInit(NULL,NULL));
    /* fetch the DirectFB interface */
	DFBCHECK(DirectFBCreate(&dfb_interface));
    /* tell the DirectFB to take the full screen for this application */
	DFBCHECK(dfb_interface->SetCooperativeLevel(dfb_interface, DFSCL_FULLSCREEN));
	
    
    /* create primary surface with double buffering enabled */  
	surface_desc.flags = DSDESC_CAPS;
	surface_desc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfb_interface->CreateSurface(dfb_interface, &surface_desc, &primary_surface));

    /* fetch the screen size */
    DFBCHECK (primary_surface->GetSize(primary_surface, &screen_width, &screen_height));

	render_fnp.radio_program = 0;
	render_fnp.volume = 0;
	render_fnp.order_prog_num = 0;
	render_fnp.prog_num = 0;
	render_fnp.print_volume = 0;
	render_fnp.print_prog_num = 0;
	render_fnp.print_info_banner = 0;
	render_fnp.volume_keypress = 0;
	render_fnp.prog_num_keypress = 0;
	render_fnp.info_banner_keypress = 0;
	
	render_thread_running = 1;
	pthread_create(&render_thread, NULL, render_loop, NULL);

    return 0;
}

int32_t graphics_deinit()
{
	pthread_mutex_lock(&render_exit_mutex);
	render_thread_running = 0;
	pthread_mutex_unlock(&render_exit_mutex);
	pthread_join(render_thread, NULL);
	primary_surface->Release(primary_surface);
	dfb_interface->Release(dfb_interface);

	return 0;
}

static void* render_loop(void* param)
{
	pthread_mutex_lock(&render_exit_mutex);
	while(render_thread_running)
	{
		pthread_mutex_unlock(&render_exit_mutex);
		
		DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
		                   /*red*/ 0x00,
		                   /*green*/ 0x00,
		                   /*blue*/ 0x00,
		                   /*alpha*/ 0x00));

		DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
	                        /*upper left x coordinate*/ 0,
	                        /*upper left y coordinate*/ 0,
	                        /*rectangle width*/ screen_width,
	                        /*rectangle height*/ screen_height));		

		pthread_mutex_lock(&render_fnp_mutex);
		if(render_fnp.radio_program == 1)
		{
			pthread_mutex_unlock(&render_fnp_mutex);
			IDirectFBImageProvider *provider;
			IDirectFBSurface *logo_surface = NULL;
			int32_t logo_height, logo_width;
			
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0x00,
	                           /*green*/ 0x00,
	                           /*blue*/ 0x00,
	                           /*alpha*/ 0xff));
			DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
	                                /*upper left x coordinate*/ 0,
	                                /*upper left y coordinate*/ 0,
	                                /*rectangle width*/ screen_width,
	                                /*rectangle height*/ screen_height));
		
			/* create the image provider for the specified file */
			DFBCHECK(dfb_interface->CreateImageProvider(dfb_interface, "radio.png", &provider));
			/* get surface descriptor for the surface where the image will be rendered */
			DFBCHECK(provider->GetSurfaceDescription(provider, &surface_desc));
			/* create the surface for the image */
			DFBCHECK(dfb_interface->CreateSurface(dfb_interface, &surface_desc, &logo_surface));
			/* render the image to the surface */
			DFBCHECK(provider->RenderTo(provider, logo_surface, NULL));

			/* cleanup the provider after rendering the image to the surface */
			provider->Release(provider);

			/* fetch the logo size and add (blit) it to the screen */
			DFBCHECK(logo_surface->GetSize(logo_surface, &logo_width, &logo_height));
			DFBCHECK(primary_surface->Blit(primary_surface,
					               /*source surface*/ logo_surface,
					               /*source region, NULL to blit the whole surface*/ NULL,
					               /*destination x coordinate of the upper left corner of the image*/(screen_width - logo_width)/2,
					               /*destination y coordinate of the upper left corner of the image*/(screen_height - logo_height)/2));
			pthread_mutex_lock(&render_fnp_mutex);
		}
		if(render_fnp.print_prog_num == 1)
		{
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0x00,
	                           /*green*/ 0xa6,
	                           /*blue*/ 0x54,
	                           /*alpha*/ 0xff));
			DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
	                                /*upper left x coordinate*/ 50,
	                                /*upper left y coordinate*/ 50,
	                                /*rectangle width*/ PR_NUM_SIZE,
	                                /*rectangle height*/ PR_NUM_SIZE));
		
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0xff,
	                           /*green*/ 0xff,
	                           /*blue*/ 0xff,
	                           /*alpha*/ 0xff));			

			DFBCHECK(primary_surface->FillRectangle(primary_surface,
								60, 
								60, 
								PR_NUM_SIZE - 20, 
								PR_NUM_SIZE - 20));

			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0x00,
	                           /*green*/ 0xa6,
	                           /*blue*/ 0x54,
	                           /*alpha*/ 0xff));			

			DFBCHECK(primary_surface->FillRectangle(primary_surface,
								65, 
								65, 
								PR_NUM_SIZE - 30, 
								PR_NUM_SIZE - 30));

			/* draw text */

			IDirectFBFont *font_interface = NULL;
			DFBFontDescription font_desc;
	
			/* specify the height of the font by raising the appropriate flag and setting the height value */
			font_desc.flags = DFDESC_HEIGHT;
			font_desc.height = 60;
	
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0xff,
	                           /*green*/ 0xff,
	                           /*blue*/ 0xff,
	                           /*alpha*/ 0xff));			

			/* create the font and set the created font for primary surface text drawing */
			DFBCHECK(dfb_interface->CreateFont(dfb_interface, "/home/galois/fonts/DejaVuSans.ttf", &font_desc, &font_interface));
			DFBCHECK(primary_surface->SetFont(primary_surface, font_interface));
		
			char number[4];
			sprintf(number,"%u",render_fnp.order_prog_num);
			/* draw the text */
			DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ number,
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ 105,
				                         /*y coordinate of the lower left corner of the resulting text*/ 145,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));

			if(render_fnp.prog_num_keypress == 1)
			{
				memset(&info_timer_spec,0,sizeof(info_timer_spec));
				timer_settime(pr_num_timer_id,0,&pr_num_timer_spec,&pr_num_timer_spec_old);
				pr_num_timer_spec.it_value.tv_sec = 3;
				pr_num_timer_spec.it_value.tv_nsec = 0;
				timer_settime(pr_num_timer_id,timer_flags,&pr_num_timer_spec,&pr_num_timer_spec_old);
			}
			render_fnp.prog_num_keypress = 0;

		}
		if(render_fnp.print_volume == 1)
		{
			IDirectFBImageProvider *provider;
			IDirectFBSurface *logoSurface = NULL;
			int32_t logo_height, logo_width;
			char file_name[20];
			char number[3];
			sprintf(number,"%u",render_fnp.volume);
			strcpy(file_name,"volume/volume_");
			strcat(file_name,number);
			strcat(file_name,".png");

			/* create the image provider for the specified file */
			DFBCHECK(dfb_interface->CreateImageProvider(dfb_interface, file_name, &provider));
			/* get surface descriptor for the surface where the image will be rendered */
			DFBCHECK(provider->GetSurfaceDescription(provider, &surface_desc));
			/* create the surface for the image */
			DFBCHECK(dfb_interface->CreateSurface(dfb_interface, &surface_desc, &logoSurface));
			/* render the image to the surface */
			DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));

			/* cleanup the provider after rendering the image to the surface */
			provider->Release(provider);

			/* fetch the logo size and add (blit) it to the screen */
			DFBCHECK(logoSurface->GetSize(logoSurface, &logo_width, &logo_height));
			DFBCHECK(primary_surface->Blit(primary_surface,
					               /*source surface*/ logoSurface,
					               /*source region, NULL to blit the whole surface*/ NULL,
					               /*destination x coordinate of the upper left corner of the image*/screen_width - logo_width - 50,
					               /*destination y coordinate of the upper left corner of the image*/50));

			if(render_fnp.volume_keypress == 1)
			{
				memset(&volume_timer_spec,0,sizeof(volume_timer_spec));
				timer_settime(volume_timer_id,0,&volume_timer_spec,&volume_timer_spec_old);
				volume_timer_spec.it_value.tv_sec = 3;
				volume_timer_spec.it_value.tv_nsec = 0;
				timer_settime(volume_timer_id,timer_flags,&volume_timer_spec,&volume_timer_spec_old);
			}
			render_fnp.volume_keypress = 0;
		}
		if(render_fnp.print_info_banner == 1)
		{
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0x00,
	                           /*green*/ 0xa6,
	                           /*blue*/ 0x54,
	                           /*alpha*/ 0xff));
			DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
	                                /*upper left x coordinate*/ 50,
	                                /*upper left y coordinate*/ screen_height - INFO_BANNER_HEIGHT - 50,
	                                /*rectangle width*/ screen_width - 100,
	                                /*rectangle height*/ INFO_BANNER_HEIGHT));
		
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0xff,
	                           /*green*/ 0xff,
	                           /*blue*/ 0xff,
	                           /*alpha*/ 0xff));			

			DFBCHECK(primary_surface->FillRectangle(primary_surface,
								60, 
								screen_height - INFO_BANNER_HEIGHT - 40, 
								screen_width - 120, 
								INFO_BANNER_HEIGHT - 20));

			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0x00,
	                           /*green*/ 0xa6,
	                           /*blue*/ 0x54,
	                           /*alpha*/ 0xff));			

			DFBCHECK(primary_surface->FillRectangle(primary_surface,
								65, 
								screen_height - INFO_BANNER_HEIGHT - 35, 
								screen_width - 130, 
								INFO_BANNER_HEIGHT - 30));
				
			/* draw text */

			IDirectFBFont *font_interface = NULL;
			DFBFontDescription font_desc;
	
			/* specify the height of the font by raising the appropriate flag and setting the height value */
			font_desc.flags = DFDESC_HEIGHT;
			font_desc.height = 60;
	
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
	                           /*red*/ 0xff,
	                           /*green*/ 0xff,
	                           /*blue*/ 0xff,
	                           /*alpha*/ 0xff));			

			/* create the font and set the created font for primary surface text drawing */
			DFBCHECK(dfb_interface->CreateFont(dfb_interface, "/home/galois/fonts/DejaVuSans.ttf", &font_desc, &font_interface));
			DFBCHECK(primary_surface->SetFont(primary_surface, font_interface));
		
			char pr_number[20];
			char audio_pid[20];
			char video_pid[20];
			
			sprintf(audio_pid,"Audio PID: %u",render_fnp.audio_pid);
			sprintf(video_pid,"Video PID: %u",render_fnp.video_pid);
			sprintf(pr_number,"Program number: %u",render_fnp.prog_num);
			/* draw the text */
			DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ pr_number,
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ 80,
				                         /*y coordinate of the lower left corner of the resulting text*/ screen_height - INFO_BANNER_HEIGHT + 30,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));

			/* draw the text */
			DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ audio_pid,
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ 80,
				                         /*y coordinate of the lower left corner of the resulting text*/ screen_height - INFO_BANNER_HEIGHT + 100,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));

			/* draw the text */
			DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ video_pid,
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ 80,
				                         /*y coordinate of the lower left corner of the resulting text*/ screen_height - INFO_BANNER_HEIGHT + 170,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));

			if(render_fnp.has_teletext == 1)
			{
				DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ "Teletext: Available",
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ screen_width/2,
				                         /*y coordinate of the lower left corner of the resulting text*/ screen_height - INFO_BANNER_HEIGHT + 30,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
			}
			else
			{
				DFBCHECK(primary_surface->DrawString(primary_surface,
				                         /*text to be drawn*/ "Teletext: Unavailable",
				                         /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
				                         /*x coordinate of the lower left corner of the resulting text*/ screen_width/2,
				                         /*y coordinate of the lower left corner of the resulting text*/ screen_height - INFO_BANNER_HEIGHT + 30,
				                         /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
			}

			if(render_fnp.info_banner_keypress == 1)
			{
				memset(&info_timer_spec,0,sizeof(info_timer_spec));
				timer_settime(info_timer_id,0,&info_timer_spec,&info_timer_spec_old);
				info_timer_spec.it_value.tv_sec = 3;
				info_timer_spec.it_value.tv_nsec = 0;
				timer_settime(info_timer_id,timer_flags,&info_timer_spec,&info_timer_spec_old);
			}
			render_fnp.info_banner_keypress = 0;
		}
		
		
		if(wait)
		{
			pthread_mutex_unlock(&render_fnp_mutex);	
			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
		                   /*red*/ 0x00,
		                   /*green*/ 0x00,
		                   /*blue*/ 0x00,
		                   /*alpha*/ 0xff));

			DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
		                        /*upper left x coordinate*/ 0,
		                        /*upper left y coordinate*/ 0,
		                        /*rectangle width*/ screen_width,
		                        /*rectangle height*/ screen_height));
			
			/* switch between the displayed and the work buffer (update the display) */
			DFBCHECK(primary_surface->Flip(primary_surface,
			                   /*region to be updated, NULL for the whole surface*/NULL,
			                   /*flip flags*/0));
			
			sleep(5);

			DFBCHECK(primary_surface->SetColor(/*surface to draw on*/ primary_surface,
		                   /*red*/ 0x00,
		                   /*green*/ 0x00,
		                   /*blue*/ 0x00,
		                   /*alpha*/ 0x00));

			DFBCHECK(primary_surface->FillRectangle(/*surface to draw on*/ primary_surface,
		                        /*upper left x coordinate*/ 0,
		                        /*upper left y coordinate*/ 0,
		                        /*rectangle width*/ screen_width,
		                        /*rectangle height*/ screen_height));

			pthread_mutex_lock(&render_fnp_mutex);	
			wait = 0;
		}
		pthread_mutex_unlock(&render_fnp_mutex);	
		
		/* switch between the displayed and the work buffer (update the display) */
		DFBCHECK(primary_surface->Flip(primary_surface,
			                   /*region to be updated, NULL for the whole surface*/NULL,
			                   /*flip flags*/0));		

		pthread_mutex_lock(&render_exit_mutex);
	}
	pthread_mutex_unlock(&render_exit_mutex);

	return (void*)0;
}

int32_t print_volume(uint32_t volume)
{
	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.volume = volume;
	render_fnp.print_volume = 1;
	render_fnp.volume_keypress = 1;
	pthread_mutex_unlock(&render_fnp_mutex);

	return 0;
}

int32_t print_prog_num(uint16_t prog_num, uint8_t radio)
{
	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.radio_program = radio;
	render_fnp.order_prog_num = prog_num;
	render_fnp.print_prog_num = 1;
	render_fnp.prog_num_keypress = 1;
	pthread_mutex_unlock(&render_fnp_mutex);

	return 0;
}

int32_t print_info_banner(uint16_t prog_num, uint16_t audio_pid, uint16_t video_pid, uint8_t ttx)
{
	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.has_teletext = ttx;
	render_fnp.prog_num = prog_num;
	render_fnp.audio_pid = audio_pid;
	render_fnp.video_pid = video_pid;
	render_fnp.print_info_banner = 1;
	render_fnp.info_banner_keypress = 1;
	pthread_mutex_unlock(&render_fnp_mutex);

	return 0;
}

int32_t print_black_screen()
{
	pthread_mutex_lock(&render_fnp_mutex);
	wait = 1;
	pthread_mutex_unlock(&render_fnp_mutex);
	
	return 0;
}

static void* volume_timer()
{
	memset(&volume_timer_spec,0,sizeof(volume_timer_spec));
	timer_settime(volume_timer_id,0,&volume_timer_spec,&volume_timer_spec_old);

	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.print_volume = 0;
	pthread_mutex_unlock(&render_fnp_mutex);

	return (void*)0;
}

static void* info_timer()
{
	memset(&info_timer_spec,0,sizeof(info_timer_spec));
	timer_settime(info_timer_id,0,&info_timer_spec,&info_timer_spec_old);

	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.print_info_banner = 0;
	pthread_mutex_unlock(&render_fnp_mutex);

	return (void*)0;
}

static void* pr_num_timer()
{
	memset(&pr_num_timer_spec,0,sizeof(pr_num_timer_spec));
	timer_settime(pr_num_timer_id,0,&pr_num_timer_spec,&pr_num_timer_spec_old);

	pthread_mutex_lock(&render_fnp_mutex);
	render_fnp.print_prog_num = 0;
	pthread_mutex_unlock(&render_fnp_mutex);

	return (void*)0;
}
