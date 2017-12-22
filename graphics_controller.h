#ifndef GC_H
#define GC_H

#include <stdint.h>
#include "tables.h"

#define INFO_BANNER_HEIGHT 300
#define PR_NUM_SIZE 150
#define PR_LIST_HEIGHT 450

/* helper macro for error checking */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}

typedef struct _flags_and_params{
	uint8_t wait;
	uint16_t program_count;
	uint8_t radio_program;
	uint8_t volume;
	uint16_t order_prog_num;
	uint8_t has_teletext;
	uint16_t prog_num;
	uint16_t audio_pid;
	uint16_t video_pid;
	uint8_t print_volume;
	uint8_t print_prog_num;
	uint8_t print_info_banner;
	uint8_t print_prog_list;
	uint8_t volume_keypress;
	uint8_t prog_num_keypress;
	uint8_t info_banner_keypress;
}flags_and_params;

int32_t graphics_init();
int32_t graphics_deinit();
int32_t print_volume(uint32_t);
int32_t print_prog_num(uint16_t, uint8_t);
int32_t print_info_banner(uint16_t, uint16_t, uint16_t, uint8_t);
int32_t print_black_screen();
int32_t print_prog_list(service_info*, uint16_t);
int32_t remove_prog_list();
static void* render_loop(void*);
static void* volume_timer();
static void* info_timer();
static void* pr_num_timer();

#endif
