//#######################################################################
// Module:     Files.cpp
// Descrption: SIFFS file management
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include <Arduino.h>
#include "Files.h"

using namespace std;

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

    File file;
    while ( (file = root.openNextFile ()) )
        {
        if ( !file.isDirectory () )
            {
            String st = file.name ();
            if ( st.equals("WPSettings.dat") )
                continue;
            if ( st.equals("IndexerVolumeGuid") )
                continue;
            this->FileList.push_back (st);
            }
        }

    this->NumberFiles = this->FileList.size ();
    if ( this->NumberFiles == 0 )
        return (true);
    return (false);
    }

//#######################################################################
void FILES_C::ListDirectory ()
    {
    int z = 0;

    if ( this-Ready )
        {
        printf ("\n\t===============================\n");
        printf ("\t=========== FILES =============\n");
        for ( auto const& zs : FileList )
            {
            String st = zs;
            printf ("\t%-2d  %s\n", ++z, st.c_str ());
            }
        }
    printf ("\n");
    }

//#######################################################################
FILES_C Files;

