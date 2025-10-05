//#######################################################################
// Module:     Config.h
// Descrption: Project settings here (defines, numbers, etc.)
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################

#pragma once
#include <Streaming.h>

//#####################################
// Usefull constants
//#####################################
#define MICRO_TO_MILLI(x) ((x) * 0.001)
#define MILLI_TO_MICRO(x) ((x) * 1000)

//#####################################
// In case I get confused with Python
//#####################################
#define False false
#define True  true

//#####################################
//   debug controls
//#####################################
extern bool DebugMidiFile;
extern bool DebugState;
extern bool DebugMidi;

//#################################################
//   Global system variables
//#################################################
extern float    DeltaTimeMilli;
extern float    DeltaTimeMicro;
extern float    LongestTimeMilli;
extern float    DeltaTimeMilliAvg;
extern uint64_t RunTime;
extern int      SkipDelta;

//#####################################
//    Located in FrontEnd.cpp
//#####################################
extern void MidiSilence   (void);

//#################################################
//    Serial 1 MIDI echo out port
//#################################################
#define RXD1        35
#define TXD1        22

