//#######################################################################
// Module:     Synth32_Sequencer.ino
// Creator:    markeby
// Date:       8/21/2024
//#######################################################################
#include <Arduino.h>

#include "Settings.h"
#include "SerialMonitor.h"
#include "Files.h"
#include "UpdateOTA.h"

//SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB


bool       SystemError          = false;
bool       SystemFail           = false;
bool       SynthActive          = false;
float      DeltaTimeMilli       = 0;             // Millisecond interval.
float      DeltaTimeMicro       = 0;             // Microsecond interval
float      DeltaTimeMilliAvg    = 0;
float      LongestTimeMilli     = 0;
uint64_t   RunTime              = 0;
bool       DebugMidi            = false;

//#######################################################################
inline void TimeDelta (void)
    {
    static uint64_t strt = 0;       // Starting time for next frame delta calculation

    RunTime = micros ();
    DeltaTimeMicro = (int)((uint64_t)RunTime - (uint64_t)strt);
    DeltaTimeMilli = MICRO_TO_MILLI (DeltaTimeMicro);
    if ( DeltaTimeMilliAvg == 0 )
        DeltaTimeMilliAvg = DeltaTimeMilli;
    else
        DeltaTimeMilliAvg = (DeltaTimeMilliAvg + DeltaTimeMilli) / 2;
    strt = RunTime;
    if ( DeltaTimeMilli > 210 )     // throw out long serial debug outputs
        return;
    if ( DeltaTimeMilli > LongestTimeMilli )
        LongestTimeMilli = DeltaTimeMilli;
    }

//#######################################################################
inline bool TickTime (void)
    {
    static uint64_t loop_cnt_100hz = 0;
    static uint64_t icount = 0;

    loop_cnt_100hz += DeltaTimeMicro;
    icount++;

    if ( loop_cnt_100hz >= MILLI_TO_MICRO (10)  )
        {
        loop_cnt_100hz = 0;
        icount = 0;
        return (true);
        }
    return (false);
    }

//#######################################################################
//#######################################################################
void setup (void)
    {
    bool fault = false;
    Serial.begin (115200);

    printf ("\t>>> Start Settings config...\n");
    Settings.Begin ();    // System settings

    printf ("\t>>> Startup OTA...\n");
    UpdateOTA.Setup (Settings.GetSSID (), Settings.GetPasswd ());

    printf ("\t>>> Starting file system...\n");
    delay (1000);
    if ( Files.Begin () )
        printf ("*** No Files available\n");
    else
       printf ("\t>>> File system ready\n");

    delay (1599);

    printf("\t>>> System ready.\n");
    }

//#######################################################################
void loop (void)
    {
    static bool first = true;

    TimeDelta ();

    // Wifi connection manager
    if ( !UpdateOTA.WiFiStatus () )
        UpdateOTA.WaitWiFi ();

    UpdateOTA.Loop ();
    Monitor.Loop ();
    }

