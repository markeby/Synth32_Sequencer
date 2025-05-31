//#######################################################################
// Module:     SerialMonitor.ino
// Descrption: Serial control for setup and debug
// Creator:    markeby
// Date:       9/4/2023
//#######################################################################
#include <Arduino.h>
#include "settings.h"
#include "SerialMonitor.h"
#include "Files.h"
#include "FileMidi.h"
#include "UpdateOTA.h"
using namespace SERIAL_MONITOR;

//#######################################################################
inline void DispRunTime (void)
    {
    float fsc = RunTime * 0.000001;
    float fmn = fsc / 60;
    float fhr = fmn / 60;
    int   hr  = fhr;
    int   mhr = hr * 60.0;
    int   mn  = fmn - mhr;
    float sc  = fsc - ((float)(mhr + mn) * 60.0);
    Serial << hr << ":" << mn << ":" << sc << endl;
    }

//#######################################################################
const char* StateDebug (bool d)
    {
    static const char* StateOn  = "  <ON>";
    static const char* StateOff = " <off>";

    if ( d )
        return (StateOn);
    return (StateOff);
    }

//#######################################################################
void MONITOR_C::DumpStats (void)
    {
    static const char* hh = " ## ";

    Serial << "==========================================" << endl;
    Serial << hh << ESP.getChipModel () << " rev " << ESP.getChipRevision () << ", ";
    Serial << hh << ESP.getChipCores () << " cores.  " << ESP.getCpuFreqMHz () << " MHz" << endl;
    Serial << hh << " SDK " << ESP.getSdkVersion () << endl;
    Serial << hh << "       Sketch size = " << ESP.getSketchSize () << endl;
    Serial << hh << "         Heap size = " << ESP.getHeapSize () << endl;
    Serial << hh << " Minimum heap size = " << ESP.getMinFreeHeap () << endl;
//    Serial << hh << "  Max alloced Heap = " << ESP.getMaxAllocHeap () << endl;
    Serial << hh << "         Free heap = " << ESP.getFreeHeap () << endl;
    Serial << "==========================================" << endl << endl;
    Serial << hh << " Free sketch space = " << ESP.getFreeSketchSpace () << endl;
    Serial << hh << "   Flash chip size = " << ESP.getFlashChipSize () << endl;
    Serial << hh << "  Flash chip speed = " << ESP.getFlashChipSpeed() << endl;
    Serial << hh << "   Flash chip mode = " << ESP.getFlashChipMode() << endl << endl;  Serial << "==========================================" << endl << endl;
    Serial << hh << "        Stack size = " << getArduinoLoopTaskStackSize() << endl;
    Serial << hh << "  Free stack space = " << uxTaskGetStackHighWaterMark(NULL) << endl << endl;
    Serial << "==========================================" << endl << endl;
    Serial << hh << "        Update URL = " << UpdateOTA.GetIP() << endl << endl;
    Serial << hh << "       Runing Time = "; DispRunTime ();
    Serial << hh << "     Last interval = " << DeltaTimeMilli << " mSec" << endl;
    Serial << hh << "  Average interval = " << DeltaTimeMilliAvg << " mSec" << endl;
    Serial << hh << "  Longest interval = " << LongestTimeMilli << " mSec" << endl;
    SkipDelta = 3;
    }

//#######################################################################
bool MONITOR_C::Save (SMODE m)
    {
    int z;
    switch ( m )
        {
        case INSSID:
            Settings.PutSSID (this->InputString);
            break;
        case INPWD:
            Settings.PutPasswd (this->InputString);
            break;
        default:
            break;
        }

    this->InputString.clear ();
    return (true);
    }

//#######################################################################
void MONITOR_C::InputPrompt (const char* text)
    {
    Serial << "\n\n" << text << " >" << this->InputString;
    }

//#######################################################################
void MONITOR_C::Tuning ()
    {
    this->Menu ();
    }

//#######################################################################
void MONITOR_C::Reset (const char* reason)
    {
    Serial << endl << "  ********** " << reason << " **********" << endl << endl;
    ESP.restart ();
    }

//#######################################################################
bool MONITOR_C::PromptZap (void)
    {
    Serial << " y/N >";
    char in_char = (char)(Serial.read () & 0xFF);
    Serial << in_char << "\n\n";
    switch ( in_char )
        {
        case 'y':
        case 'Y':
            switch ( this->InputMode )
                {
                case ZAP1:
                    Settings.ClearAllSynth();
                    Serial << 9 << endl << "\nCleared System settings." << endl;
                    this->Reset ("Reset after clearing settings");
                    break;
                case ZAP2:
                    this->Mode (MENU);
                    break;
                default:
                    break;
                }
            break;
        case 'n':
        case 'N':
        default:
            return (true);
            break;
        }
    return (false);
    }

//#######################################################################
void MONITOR_C::MenuSel (void)
    {
    char s = Serial.read ();

    if ( s == 27 )                  // detect escape 00x1B
        {
        int funct = 0;
        while ( Serial.available () )
            {
            s = Serial.read ();
            switch ( s )
                {
                case 91:            // 0x5B
                    if ( !funct )
                        funct++;
                    break;
                case 50:            // 0x32
                    if ( funct == 1 )
                        funct++;
                    break;
                case 52:            // 0x34
                    if ( funct == 2 )
                        funct++;
                    break;
                case 126:           // 0x7E
                    if ( funct == 3 )
                        {
                        this->Reset ("F12 reset");
                        }
                    break;
                case 0x41:          // arrow up
                    break;
                case 0x42:          // arrow down
                    break;
                case 0x43:          // arrow right
                    break;
                case 0x44:          // arrow left
                    break;
                default:
                    break;
                }
            }
        return;
        }

    switch ( this->InputMode )
        {
        case CMD:
            switch ( s )
                {
                case 's':
                    Serial << endl;
                    this->DumpStats ();
                    this->Mode (MENU);
                    break;
                case 'd':
                    Settings.SaveDebugFlags ();
                    Serial << "  Saving debug flags" << endl;
                    this->Mode (MENU);
                    break;
                case '1':
                    DebugState  = !DebugState;
                    Serial << "  MIDI debugging " << (( DebugState ) ? "Enabled" : "Disabled") << endl;
                    this->Mode (MENU);
                    break;
                case '2':
                    DebugMidiFile  = !DebugMidiFile;
                    Serial << "  MIDI file processing " << (( DebugMidiFile ) ? "Enabled" : "Disabled") << endl;
                    this->Mode (MENU);
                    break;
                case '3':
                    DebugMidi  = !DebugMidi;
                    Serial << "  MIDI Interface " << (( DebugMidi ) ? "Enabled" : "Disabled") << endl;
                    this->Mode (MENU);
                    break;

                case 'S':
                    this->InputString = Settings.GetSSID ();
                    this->InputPrompt ("  Enter SSID");
                    this->Mode (INSSID);
                    break;
                case 'P':
                    this->InputString = Settings.GetPasswd ();
                    this->InputPrompt ("  Enter PWD");
                    this->Mode (INPWD);
                    break;
                case 'C':
                    this->InputPrompt ("  Clearing Synth settings");
                    this->Mode (ZAP1);
                    break;
                case 'z':           // Test function #1
                    break;
                case 'x':           // Test function #2
                    break;
                case 'c':           // Test function #3
                    break;
                case 'v':           // Test function #4
                    break;
                case ' ':           // Just move the cursor down a couple of lines
                    Serial << "...\n\n";
                    break;
                default:
                    Serial << "       ??" << endl;
                    this->Mode (MENU);
                    break;
                }
            break;
        }
    }

//#######################################################################
void MONITOR_C::Menu (void)
    {
    Serial << endl;
    Serial << "\t######       Testing        ######" << endl;
    Serial << "\t######   Midi File system   ######" << endl;
    Serial << StateDebug (DebugState)    << "\t1   - Debug State operation" << endl;
    Serial << StateDebug (DebugMidiFile) << "\t2   - Debug MIDI file processing" << endl;
    Serial << StateDebug (DebugMidi)     << "\t3   - Debug MIDI interface" << endl;
    Serial << "\ts   - Dump process Stats" << endl;
    Serial << "\td   - Save debug flags" << endl;
    Serial << "\n";
    Serial << "\tz   - Test function #1" << endl;
    Serial << "\tx   - Test function #2" << endl;
    Serial << "\tc   - Test function #3" << endl;
    Serial << "\tv   - Test function #4" << endl;
    Serial << "\tS   - SSID" << endl;
    Serial << "\tP   - Password" << endl;
    Serial << "\tC   - Clear Synth settings" << endl;
    Serial << "\tF12 - Reset" << endl;
    Serial << endl;
    }

//#######################################################################
void MONITOR_C::TextIn (void)
    {
    char in_char = (char)(Serial.read () & 0xFF);
    switch ( in_char )
        {
        case '\r':              // return (enter)
            if ( this->Save (this->InputMode) )
                this->Mode (MENU);
            break;
        case (char)127:         // backspace
            if ( this->InputString.length () )
                {
                Serial << (char)8 << ' ' << (char)8;
                this->InputString.remove (this->InputString.length () - 1);
                }
            break;
        case (char)27:          // escape for exit with no change
            this->Mode (MENU);
            break;
        case '\t':              // Tab for special loops
            this->Save (this->InputMode);
            break;
        default:                // all other characters go into string
            this->InputString += in_char;
            Serial << in_char;
            break;
        }
    }

//#######################################################################
MONITOR_C::MONITOR_C (void)
    {
    this->InputString = "";
    this->InputMode   = MENU;
    }

//#######################################################################
MONITOR_C::~MONITOR_C (void)
    {
    }

//#######################################################################
void MONITOR_C::Begin (void)
    {
    Serial.begin (115200);
    this->DumpStats ();
    Serial << "\n\n\nHow the hell did I get here man?\n\n\n";
    }

//#######################################################################
bool MONITOR_C::Detect (void)
    {
    return (Serial.available ());
    }

//#######################################################################
void MONITOR_C::Loop (void)
    {
    if ( this->InputMode != MENU )
        {
        while ( Serial.available () )
            {
            switch ( this->InputMode )
                {
                case CMD:
                    this->MenuSel ();
                    break;
                case INSSID:
                case INPWD:
                    this->TextIn ();
                    break;
                case ZAP1:
                case ZAP2:
                    if ( this->PromptZap () )
                        this->Mode(MENU);
                    break;
                default:
                    break;
                }
            }
        }
    else
        {
        this->Menu ();
        this->Mode (CMD);
        }
    }

//#######################################################################
MONITOR_C Monitor;

