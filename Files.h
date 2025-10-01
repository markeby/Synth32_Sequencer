//#######################################################################
// Module:     Files.h
// Descrption: SIFFS file management
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#pragma once
#include <deque>
#include <FS.h>
#include <SD.h>

#include "config.h"
#include "FileMidi.h"

uint32_t readMultiByte  (File& fd, uint8_t len);
uint32_t readVarLen     (File& fd);

//#######################################################################
class FILES_C
    {
private:
    bool                Ready;
    std::deque<String>  FileList;
    int                 NumberFiles;
    File                FileDescripter;

    bool FetchDirectory (String& str);


public:
    FILE_MIDI_C     MidiF;

            FILES_C         (void);
    bool    Begin           (String& str);
    bool    OpenFile        (String& fname);
    String  FetchFileName   (int index);
    bool    Process         (void);

    byte    GetTrackCount   (void)          { return (MidiF.getTrackCount ()); }
    };


