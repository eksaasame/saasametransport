// base32.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_BASE32_INCLUDE__
#define __MACHO_BASE32_INCLUDE__

#include "..\config\config.hpp"

namespace macho{

/*
    base32 encoding / decoding.
    Encode32 outputs at out bytes with values from 0 to 32 that can be mapped to 32 signs.
    Decode32 input is the output of Encode32. The out parameters should be unsigned char[] of
    length GetDecode32Length(inLen) and GetEncode32Length(inLen) respectively.
    To map the output of Encode32 to an alphabet of 32 characters use Map32.
    To unmap back the output of Map32 to an array understood by Decode32 use Unmap32.
    Both Map32 and Unmap32 do inplace modification of the inout32 array.
    The alpha32 array must be exactly 32 chars long.
*/

struct base32{
    static bool decode32(unsigned char* in, int inLen, unsigned char* out);
    static bool encode32(unsigned char* in, int inLen, unsigned char* out);
    static int  get_decode32_length(int bytes);
    static int  get_encode32_length(int bytes);
    static bool map32(unsigned char* inout32, int inout32Len, unsigned char* alpha32 = (unsigned char*)base32::alphabet );
    static bool unmap32(unsigned char* inout32, int inout32Len, unsigned char* alpha32 = (unsigned char*)base32::alphabet );
    static const unsigned char alphabet[33] ; 
};

#ifndef MACHO_HEADER_ONLY

const unsigned char base32::alphabet[33] = "123456789ABCDEFGHJKMNPQRSTUVWXYZ";

int base32::get_encode32_length(int bytes){
   int bits = bytes * 8;
   int length = bits / 5;
   if((bits % 5) > 0){
      length++;
   }
   return length;
}

int base32::get_decode32_length(int bytes){
   int bits = bytes * 5;
   int length = bits / 8;
   return length;
}

static bool Encode32Block(unsigned char* in5, unsigned char* out8){
      // pack 5 bytes
      unsigned __int64 buffer = 0;
      for(int i = 0; i < 5; i++){
          if(i != 0){
              buffer = (buffer << 8);
          }
          buffer = buffer | in5[i];
      }
      // output 8 bytes
      for(int j = 7; j >= 0; j--){
          buffer = buffer << (24 + (7 - j) * 5);
          buffer = buffer >> (24 + (7 - j) * 5);
          unsigned char c = (unsigned char)(buffer >> (j * 5));
          // self check
          if(c >= 32) return false;
          out8[7 - j] = c;
      }
      return true;
}

bool base32::encode32(unsigned char* in, int inLen, unsigned char* out){

   if((in == 0) || (inLen <= 0) || (out == 0)) return false;

   int d = inLen / 5;
   int r = inLen % 5;

   unsigned char outBuff[8];

   for(int j = 0; j < d; j++){
      if(!Encode32Block(&in[j * 5], &outBuff[0])) return false;
      memmove(&out[j * 8], &outBuff[0], sizeof(unsigned char) * 8);
   }

   unsigned char padd[5];
   memset(padd, 0, sizeof(unsigned char) * 5);
   for(int i = 0; i < r; i++){
      padd[i] = in[inLen - r + i];
   }
   if(!Encode32Block(&padd[0], &outBuff[0])) return false;
   memmove(&out[d * 8], &outBuff[0], sizeof(unsigned char) * get_encode32_length(r));

   return true;
}

static bool Decode32Block(unsigned char* in8, unsigned char* out5){
      // pack 8 bytes
      unsigned __int64 buffer = 0;
      for(int i = 0; i < 8; i++)
      {
          // input check
          if(in8[i] >= 32) return false;
          if(i != 0)
          {
              buffer = (buffer << 5);
          }
          buffer = buffer | in8[i];
      }
      // output 5 bytes
      for(int j = 4; j >= 0; j--)
      {
          out5[4 - j] = (unsigned char)(buffer >> (j * 8));
      }
      return true;
}

bool base32::decode32(unsigned char* in, int inLen, unsigned char* out){

   if((in == 0) || (inLen <= 0) || (out == 0)) return false;

   int d = inLen / 8;
   int r = inLen % 8;

   unsigned char outBuff[5];

   for(int j = 0; j < d; j++){
      if(!Decode32Block(&in[j * 8], &outBuff[0])) return false;
      memmove(&out[j * 5], &outBuff[0], sizeof(unsigned char) * 5);
   }

   unsigned char padd[8];
   memset(padd, 0, sizeof(unsigned char) * 8);
   for(int i = 0; i < r; i++){
      padd[i] = in[inLen - r + i];
   }
   if(!Decode32Block(&padd[0], &outBuff[0])) return false;
   memmove(&out[d * 5], &outBuff[0], sizeof(unsigned char) * get_decode32_length(r));

   return true;
}

bool base32::map32(unsigned char* inout32, int inout32Len, unsigned char* alpha32){
    if((inout32 == 0) || (inout32Len <= 0) || (alpha32 == 0)) return false;
    for(int i = 0; i < inout32Len; i++){
        if(inout32[i] >=32) return false;
        inout32[i] = alpha32[inout32[i]];
    }
    return true;
}

static void ReverseMap(unsigned char* inAlpha32, unsigned char* outMap){
    memset(outMap, 0, sizeof(unsigned char) * 256);
    for(int i = 0; i < 32; i++){
        outMap[(int)inAlpha32[i]] = i;
    }
}

bool base32::unmap32(unsigned char* inout32, int inout32Len, unsigned char* alpha32){
    if((inout32 == 0) || (inout32Len <= 0) || (alpha32 == 0)) return false;
    unsigned char rmap[256];
    ReverseMap(alpha32, rmap);
    for(int i = 0; i < inout32Len; i++){
        inout32[i] = rmap[(int)inout32[i]];
    }
    return true;
}

#endif

};//namespace macho

#endif