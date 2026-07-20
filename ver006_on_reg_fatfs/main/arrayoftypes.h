#ifndef ARRAY_OF_TYPES_H
#define ARRAY_OF_TYPES_H

#include "stdint.h"

typedef union
{
  uint16_t uv;
  int16_t  iv;
  uint8_t  m[2];
  struct
  {
    uint8_t v1,v2;
  }v;
}value2b_t;

typedef union
{
  struct {uint8_t u80, u81, u82, u83;};
  struct {int8_t i80, i81, i82, i83;};
  struct {uint16_t u160, u161;};
  struct {int16_t i160, i161;};
  uint32_t u32;
  int32_t i32;
  float flt;
}value4b_t;

typedef union
{
  uint8_t  m[8];
  struct {uint8_t u80, u81, u82, u83, u84, u85, u86, u87;};
  struct {int8_t i80, i81, i82, i83, i84, i85, i86, i87;};
  struct {uint16_t u160, u161, u162, u163;};
  struct {int16_t i160, i161, i162, i163;};
  struct {uint32_t u320, u321;};
  struct {int32_t i320, i321;};
  struct {float f0, f1;};
}value8b_t;

typedef union
{
  uint8_t  m[16];
  struct {uint8_t u80, u81, u82, u83, u84, u85, u86, u87, u88, u89, u810, u811, u812, u813, u814, u815;};
  struct {int8_t i80, i81, i82, i83, i84, i85, i86, i87, i88, i89, i810, i811, i812, i813, i814, i815;};
  struct {uint16_t u160, u161, u162, u163, u164, u165, u166, u167;};
  struct {int16_t i160, i161, i162, i163, i164, i165, i166, i167;};
  struct {uint32_t u320, u321, u322, u323;};
  struct {int32_t i320, i321, i322, i323;};
  struct {float f0, f1, f3, f4;};
}value16b_t;


typedef union
{
  struct {float x,y,z;};
  struct {float b,e,dal;};
}value3f_t;


#endif





