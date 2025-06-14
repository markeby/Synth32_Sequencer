// config.h
//
// Project settings here (defines, numbers, etc.)

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
//   Alarms and alerts
//#################################################
#define HEARTBEAT_PIN       38
#//define HEARTBEAT_PIN       2

//#################################################
//    Serial 1 MIDI echo out port
//#################################################
#define RXD1        35
#define TXD1        22

//#################################################
//    Display message parameters and I2C
//#################################################
#define MSG_ADRS                0x51
#define MSG_SCL                 25
#define MSG_SDA                 26
#define RESET_STROBE_IO         27
#define DISPLAY_SETTLE_TIME     200000

//#################################################
//  Synth I2C interface starting indexes
//#################################################
#define START_OSC_ANALOG        0
#define START_NOISE_ANALOG      56
#define START_NOISE_DIGITAL     95
#define D_A_COUNT               88

//#################################################
//   Global system variables
//#################################################
extern float    DeltaTimeMilli;
extern float    DeltaTimeMicro;
extern float    LongestTimeMilli;
extern float    DeltaTimeMilliAvg;
extern uint64_t RunTime;
extern int      SkipDelta;

//#################################################
//    Synth specific constants
//#################################################
#define MIDI_PORT           0        // sometime referred to as cable number
#define FULL_KEYS           128
#define DA_RANGE            4096
#define MAX_DA              (DA_RANGE - 1)
#define NOTES_PER_OCTAVE    12


