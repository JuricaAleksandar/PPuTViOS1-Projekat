#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "tables.h"

int32_t parse_PAT(uint8_t*, PAT_st*);
int32_t parse_PMT(uint8_t*, PMT_st*);
int32_t parse_SDT(uint8_t*, SDT_st*, uint16_t);

#endif
