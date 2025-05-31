//#######################################################################
// Module:     Debug.cpp
// Descrption: Varialble length debug output
// Creator:    markeby
// Date:       3/17/2023
//#######################################################################
//#include <Arduino.h>
#include <Streaming.h>
#include <vector>
#include "Debug.h"
using namespace std;

String DebugStr;            // Storage for current debug frame

//#######################################################################
const String vFormat (const char *const zcFormat, ...)
    {
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start (vaArgs, zcFormat);

    // reliably acquire the size  from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy (vaArgsCopy, vaArgs);
    const int iLen = vsnprintf (NULL, 0, zcFormat, vaArgsCopy);
    va_end (vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    vector<char> zc (iLen + 1);
    vsnprintf (zc.data (), zc.size (), zcFormat, vaArgs);
    va_end (vaArgs);
    return (String (zc.data (), iLen));
    }

//#######################################################################
const String vsFormat (const char *const zcFormat, va_list args)
    {
    // reliably acquire the size  from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy (vaArgsCopy, args);
    const int iLen = vsnprintf (NULL, 0, zcFormat, vaArgsCopy);
    va_end (vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    vector<char> zc (iLen + 1);
    vsnprintf (zc.data (), zc.size (), zcFormat, args);
    va_end (args);
    return (String (zc.data (), iLen));
    }

//#######################################################################
void DebugMsg (const char* label, uint8_t index, const char *const fmt, ...)
    {
    va_list ap;
    va_start (ap, fmt);
    String str;

    if ( index == DEBUG_NO_INDEX )
        str = "[" + String(label) + "] ";
    else
        str = "[" + String(label) + "-" + String(index) + "] ";
    str += vsFormat (fmt, ap);
    Serial << str << endl;
    }

//#######################################################################
void DebugMsgStart (const char* label, uint8_t index, String name,  const char *const fmt, ...)
    {
    va_list ap;
    va_start (ap, fmt);

    if ( index == DEBUG_NO_INDEX )
        DebugStr = "[" + String(label) + "] ";
    else
        DebugStr = "[" + String(label) + "-" + String(index) + "] ";
    DebugStr += vsFormat (fmt, ap);
    }

//#######################################################################
void DebugMsgDataF (uint32_t data, uint8_t lead)
    {
    char fmt[12];

    sprintf (fmt, "%%#0%dX ", lead);
    DebugStr += vFormat (fmt, data);
    }

//#######################################################################
void DebugMsgClose ()
    {
    Serial << DebugStr << endl;
    DebugStr.clear ();
    }

