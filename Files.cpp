//#######################################################################
// Module:     Files.cpp
// Descrption: SIFFS file management
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include "Files.h"

//#####################################
//    Located in FrontEnd.cpp
//#####################################
extern void MidiCallback  (midi_event *pev);
extern void SysexCallback (sysex_event *pev);

//#######################################################################
uint32_t ReadMultiByte (File& fd, uint8_t len)
    {
    uint32_t  val = 0;

    for ( int z = 0;  z < len;  z++ )
        val = (val << 8) + fd.read ();
    return (val);
    }

//#######################################################################
uint32_t ReadVarLen (File& fd)
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
FILES_C::FILES_C ()
    {
    FileList.clear ();
    NumberFiles = 0;
    }

//#######################################################################
bool FILES_C::Begin (String& str)
    {
    bool rc = false;

    MidiF.setMidiHandler  (MidiCallback);
    MidiF.setSysexHandler (SysexCallback);

    if ( SD.begin() )
        {
        if ( SD.cardType () != CARD_NONE )
            {
            if ( this->FetchDirectory (str) )
                rc = true;
            }
        else
            {
            printf ("\t**** No SD card found\n");
            rc = true;
            }
        }
    else
        {
        printf ("\t**** SD card failed to initialize\n");
        rc = true;
        }

    return (rc);
    }

//#######################################################################
bool FILES_C::FetchDirectory (String& str)
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
            FileList.push_back (st);
            }
        file = root.openNextFile ();
        }

    NumberFiles = FileList.size ();
    if ( NumberFiles == 0 )
        return (true);

    int sz = FileList.size ();
    for ( int z = 0;  z < sz;  z++ )
        {
        String strt = FileList.at (z);

        if ( strt.endsWith (".mid") )
            strt.remove (strt.length () - 4);
        str += strt;
        if ( z != (sz - 1) )
            str += "\n";
        }

    return (false);
    }

//#######################################################################
bool FILES_C::OpenFile (String& fname)
    {
    String st = "/";
    st += fname;

    MidiF.close ();

    FileDescripter = SD.open (st);

    if ( !FileDescripter )
        {
        printf ("*** ERROR:  File <%s> failed to open\n", st.c_str ());
        return (true);
        }
    if ( MidiF.Load (this->FileDescripter) != 0 )
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
    if ( !MidiF.isEOF () )
        {
        if ( MidiF.getNextEvent() )
            return (false);                 // this executes when the metronome ticks.
        }
    else
        return (true);      // flag the end of the run.
    return (false);         // not done so continue.
    }

