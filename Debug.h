//#######################################################################
// Module:     Debug.h
// Descrption: Varialble length debug output
// Creator:    markeby
// Date:       3/17/2024
//#######################################################################
#pragma once
#include <Arduino.h>

#define DEBUG_NO_INDEX      255

const String vFormat (const char *const zcFormat, ...);
const String vsFormat (const char *const zcFormat, va_list args);

void DebugMsg       (const char* label, uint8_t index, const char *const fmt, ...);
void DebugMsgOpen   (const char* label, uint8_t index, String name,  const char *const fmt, ...);
void DebugMsgClose  (void);
void DebugMsgDataF  (uint32_t data, uint8_t lead);

inline void DebugMsgData   (uint8_t data)                    {DebugMsgDataF(data,2);}
inline void DebugMsgData   (int8_t data)                     {DebugMsgDataF(data,2);}
inline void DebugMsgData   (uint16_t data)                   {DebugMsgDataF(data,4);}
inline void DebugMsgData   (int16_t data)                    {DebugMsgDataF(data,4);}
inline void DebugMsgData   (uint32_t data)                   {DebugMsgDataF(data,8);}
inline void DebugMsgData   (int data)                        {DebugMsgDataF(data,0);}
inline void DebugMsgDataN  (uint32_t data)                   {DebugMsgDataF(data,0);}


#define DbgD(d) {printf("==> %s:%d %s = %d\n",__FILE__,__LINE__, #d, d);}
#define DbgX(x) {printf("==> %s:%d %s = 0x%X\n",__FILE__,__LINE__, #x, x);}
#define DbgF(f) {printf("==> %s:%d %s = %f\n",__FILE__,__LINE__, #f, f);}
#define DbgS(s) {printf("==> %s:%d %s = %s\n",__FILE__,__LINE__, #s, s);}
#define DbgN    {printf("\n");}



