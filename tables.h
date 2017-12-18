#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

typedef struct _service_info{
	uint16_t service_id;
	uint8_t service_type;
	uint16_t descriptors_loop_length;
	char* service_name;
}service_info;

typedef struct SDT{
	uint16_t section_length;
	service_info* services;
}SDT_st;

typedef struct PMT{
	uint16_t table_id;
	uint16_t pr_num;
	uint16_t section_length;
	uint16_t audio_pid;
	uint16_t video_pid;
	uint8_t has_teletext;
}PMT_st;

typedef struct PAT{
	uint16_t section_length;
	uint16_t pmt_count;
	PMT_st* pmts;	
}PAT_st;

#endif
