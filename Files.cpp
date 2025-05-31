//#######################################################################
// Module:     Files.cpp
// Descrption: SIFFS file management
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include "Files.h"

extern void MidiCallback  (midi_event *pev);
extern void SysexCallback (sysex_event *pev);

//#######################################################################
uint32_t readMultiByte (File& fd, uint8_t len)
    {
    uint32_t  val = 0;

    for ( int z = 0;  z < len;  z++ )
        val = (val << 8) + fd.read ();
    return (val);
    }

//#######################################################################
uint32_t readVarLen (File& fd)
    {
    uint32_t  val = 0;
    uint8_t   zc;

    do  {
        zc = fd.read ();
        val = (val << 7) + (zc & 0x7f);
        } while ( zc & 0x80 );
    return (val);
    }

//#######################################################################
//#######################################################################
FILES_C::FILES_C () : Ready(false)
    {
    this->Ready = false;
    FileList.clear ();
    NumberFiles = 0;
    }

//#######################################################################
bool FILES_C::Begin ()
    {
    this->MidiF.setMidiHandler  (MidiCallback);
    this->MidiF.setSysexHandler (SysexCallback);

    if ( SD.begin() )
        {
        if ( SD.cardType () != CARD_NONE )
            {
            if ( this->FetchDirectory () )
                return (true);
            }
        else
            {
            printf ("\t**** No SD card found\n");
            return (true);
            }
        }
    else
        {
        printf ("\t**** SD card failed to initialize\n");
        return (true);
        }
    Ready = true;

    return (false);
    }

//#######################################################################
bool FILES_C::FetchDirectory ()
    {
    File root = SD.open ("/");
    if ( !root )
        {
        printf ("\t**** failed to open directory");
        return (true);
        }

    File file = root.openNextFile ();
    while ( file )
        {
        if ( !file.isDirectory () )
            {
            String st = file.name ();
            this->FileList.push_back (st);
            }
        file = root.openNextFile ();
        }

    this->NumberFiles = this->FileList.size ();
    if ( this->NumberFiles == 0 )
        return (true);
    return (false);
    }

//#######################################################################
bool FILES_C::OpenFile (String& fname)
    {
    String st = "/";
    st += fname;

    this->FileDescripter = SD.open (st);

    if ( !this->FileDescripter )
        {
        printf ("*** ERROR:  File <%s> failed to open\n", st.c_str ());
        return (true);
        }
    if ( this->MidiF.Load (this->FileDescripter) != 0 )
        {
        printf ("\t**** failed to read midi data file\n");
        return (true);
        }

    return (false);
    }

//#######################################################################
String FILES_C::FetchFileName (int index)
    {
    String st;

    if ( index < FileList.size () )
        st = FileList.at (index);
    return (st);
    }

//#######################################################################
bool FILES_C::Process ()
    {
    if ( !this->MidiF.isEOF () )
        {
        if ( this->MidiF.getNextEvent() )
            return (false);                 // this executes when the metronome ticks.
        }
    else
        return (true);      // flag the end of the run.
    return (false);         // not done so continue.
    }

