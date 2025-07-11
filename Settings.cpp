//#######################################################################
// Module:     Settings.cpp
// Descrption: Load and save settings for system and synth
// Creator:    markeby
// Date:       5/17/2023
//#######################################################################
#include <Arduino.h>
#include <Preferences.h>
#include "Settings.h"
#include "Config.h"

//#######################################################################
Preferences Prefs;

//#######################################################################
// Preference keys
static const char* sysKeySpace    = "SysP";
static const char* sysKeySSID     = "SSID";
static const char* sysKeyPSWD     = "PASSD";
static const char* sysKeyDBG      = "DBG";

//#######################################################################
void SETTINGS_C::ClearAllSys (void)
    {
    Prefs.begin (sysKeySpace, false);
    Prefs.clear ();
    Prefs.end ();
    }

//#######################################################################
void SETTINGS_C::PutSSID (String& str)
    {
    Prefs.begin (sysKeySpace, false);
    Prefs.putString (sysKeySSID, str);
    s_SSID = str;
    Prefs.end ();
    }

//#######################################################################
void SETTINGS_C::PutPasswd (String& str)
    {
    Prefs.begin (sysKeySpace, false);
    Prefs.putString (sysKeyPSWD, str);
    s_PSWD = str;
    Prefs.end ();
    }

//#######################################################################
void SETTINGS_C::PutDebugSwitch (uint8_t num, bool state)
    {
    char buf[8];

    sprintf (buf, "%s%d", sysKeyDBG, num);
    Prefs.begin(sysKeySpace, false);
    Prefs.putBool ((const char *)buf, state);
    Prefs.end ();
    }

//#######################################################################
bool SETTINGS_C::GetDebugSwitch (uint8_t num)
    {
    char buf[8];

    sprintf (buf, "%s%d", sysKeyDBG, num);
    Prefs.begin (sysKeySpace, false);
    if (  !Prefs.isKey ((const char *)buf) )
        this->DebugFlags = false;
    bool zb = Prefs.getBool ((const char *)buf);
    Prefs.end ();
    return (zb);
    }

//#######################################################################
void SETTINGS_C::SaveDebugFlags ()
    {
    this->PutDebugSwitch (0, DebugMidiFile);
    this->PutDebugSwitch (1, DebugState);
    this->PutDebugSwitch (2, DebugMidi);
    }

//#######################################################################
void SETTINGS_C::RestoreDebugFlags ()
    {
    this->DebugFlags = true;
    DebugMidiFile    = this->GetDebugSwitch (0);
    DebugState       = this->GetDebugSwitch (1);
    DebugMidi        = this->GetDebugSwitch (2);
    if ( this->DebugFlags )
        printf ("\t>>> Debug setup.\n");
    else
        printf ("\t  **** Debug flags failed to load\n");
    }

//#######################################################################
SETTINGS_C::SETTINGS_C (void)
    {
    }

//#######################################################################
void SETTINGS_C::Begin (void)
    {
    Prefs.begin (sysKeySpace, false);
        {
        s_SSID = Prefs.getString (sysKeySSID);
        s_PSWD = Prefs.getString (sysKeyPSWD);
        }
    Prefs.end ();
    Settings.RestoreDebugFlags ();
    }

//#######################################################################
//#######################################################################
void SETTINGS_C::ClearAllSynth (void)
    {
    Prefs.begin (sysKeySpace, false);
    Prefs.clear ();
    Prefs.end ();
    }

//#######################################################################
bool SETTINGS_C::GetOscBank (uint8_t num, uint16_t* pbank)
    {
    }

//#######################################################################
void SETTINGS_C::PutOscBank (uint8_t num, uint16_t* pbank)
    {
    }

//#######################################################################
SETTINGS_C Settings;        // System settings

