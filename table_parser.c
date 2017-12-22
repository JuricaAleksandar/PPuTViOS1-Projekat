#include "table_parser.h"
#include <stdlib.h>
#include <stdio.h>

int32_t parse_PAT(uint8_t* buffer, PAT_st* PAT)
{
	uint16_t i;
	uint8_t j = 0;
	uint8_t prog_count;
	PAT->section_length = (((uint16_t)(buffer[1] & 0x0F))<<8)+buffer[2];
	prog_count = (PAT->section_length-9)/4-1;
	PAT->pmt_count = prog_count;
	PAT->pmts = (PMT_st*)malloc(sizeof(PMT_st)*prog_count);
	for( i = 0; i < (uint16_t)prog_count + 1; i++ )
	{
		uint8_t* tmp_ptr = buffer + 8 + (i * 4);
		if( *(tmp_ptr) != 0 )
		{
			uint8_t first_byte = *tmp_ptr;
			uint8_t second_byte = *(tmp_ptr+1);
			uint8_t third_byte = *(tmp_ptr+2);
			uint8_t fourth_byte = *(tmp_ptr+3);
			PAT->pmts[j].pr_num = (uint16_t)((first_byte<<8) + second_byte);
			PAT->pmts[j].table_id = (uint16_t)((third_byte<<8) + fourth_byte) & 0x1FFF;
			printf("Program number: %u , Table id: %u\n",PAT->pmts[j].pr_num,PAT->pmts[j].table_id);
			j++;
		}
	}

	return 0;
}

int32_t parse_PMT(uint8_t* buffer, PMT_st* PMT)
{
	uint16_t sl;
	PMT->section_length = (((uint16_t)(buffer[1] & 0x0F))<<8) + buffer[2];
	PMT->video_pid = 65000;
	PMT->audio_pid = 65000;
	PMT->has_teletext = 0;
	sl = PMT->section_length - 13;
	uint8_t* tmp_ptr = buffer + 12;	
	while( sl != 0)
	{	
		if( (*tmp_ptr == 2 || *tmp_ptr == 1) && PMT->video_pid == 65000 )
		{
			PMT->video_pid = ((((uint16_t)tmp_ptr[1])<<8) + (uint16_t)tmp_ptr[2]) & 0x1FFF;
		}
		if ((*tmp_ptr == 3 || *tmp_ptr == 4) && PMT->audio_pid == 65000)
		{
			PMT->audio_pid = ((((uint16_t)tmp_ptr[1])<<8) + (uint16_t)*(tmp_ptr+2)) & 0x1FFF;	
		}
		if(*tmp_ptr == 6)
		{
			PMT->has_teletext = 1;
		}
		uint16_t ES_info_length = ((((uint16_t)tmp_ptr[3])<<8) + (uint16_t)tmp_ptr[4]) & 0x0FFF;
		sl = sl - ES_info_length - 5;
		tmp_ptr = tmp_ptr + ES_info_length + 5;
	}
}

int32_t parse_SDT(uint8_t* buffer, SDT_st* SDT, uint16_t count)
{
	uint16_t j = 0;
	uint16_t sl;
	SDT->section_length = (((uint16_t)(buffer[1] & 0x0F))<<8) + buffer[2];
	sl = SDT->section_length - 12;
	uint8_t* tmp_ptr = buffer + 11;	
	SDT->services = (service_info*)malloc(count*sizeof(service_info));
	while(sl != 0)
	{
		SDT->services[j].service_id = (((uint16_t)tmp_ptr[0])<<8) + tmp_ptr[1];
		SDT->services[j].descriptors_loop_length = ((((uint16_t)tmp_ptr[3])<<8) + tmp_ptr[4]) & 0x0FFF;
		SDT->services[j].service_type = tmp_ptr[7];

		uint8_t provider_len = tmp_ptr[8];
		uint8_t service_name_len = tmp_ptr[9+provider_len];			

		SDT->services[j].service_name = (char*)malloc((service_name_len+1)*sizeof(char));		
		uint16_t i;
		for( i = 0; i < service_name_len; i++)
		{
			SDT->services[j].service_name[i] = tmp_ptr[10+provider_len+i];
		}
		SDT->services[j].service_name[service_name_len] = 0;
		sl = sl - SDT->services[j].descriptors_loop_length - 5;
		tmp_ptr = tmp_ptr + SDT->services[j].descriptors_loop_length + 5;
		printf("Service name: %s, Service type: %u\n",SDT->services[j].service_name,SDT->services[j].service_type);
		j++;
	}
}
