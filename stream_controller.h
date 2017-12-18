#ifndef SC_H
#define SC_H

#include <stdint.h>
#include "tdp_api.h"

#define ASSERT_TDP_RESULT(x,y)  if(NO_ERROR == x) \
                                    printf("%s success\n", y); \
                                else{ \
                                    textColor(1,1,0); \
                                    printf("%s fail\n", y); \
                                    textColor(0,7,0); \
                                    return -1; \
                                }
#define ASSERT_TDP_THREAD_RESULT(x,y)  if(NO_ERROR == x) \
                                    printf("%s success\n", y); \
                                else{ \
                                    textColor(1,1,0); \
                                    printf("%s fail\n", y); \
                                    textColor(0,7,0); \
                                    return (void*)-1; \
                                }

typedef struct _stream_status{
	uint8_t audio_on;
	uint8_t video_on;
}stream_status;

typedef struct _stream_params{
	uint8_t next_program;
	uint8_t previous_program;
	uint8_t change_program;
	uint8_t get_prog_info;
	uint8_t set_volume;
}stream_params;

int32_t stream_init(int32_t (*callback1)(uint16_t,uint8_t),int32_t (*callback2)(),int32_t (*callback3)(uint16_t,uint16_t,uint16_t,uint8_t));
int32_t stream_deinit();
int32_t get_prog_info();
int32_t next_program();
int32_t previous_program();
int32_t change_program();
int32_t set_volume(uint32_t);
static int32_t filter_callback(uint8_t*);
static int32_t tuner_status_callback(t_LockStatus);
static void* stream_loop(void*);

#endif
