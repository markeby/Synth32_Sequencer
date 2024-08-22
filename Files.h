//#######################################################################
// Module:     Files.h
// Descrption: SIFFS file management
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#pragma once
#include <list>
#include "config.h"
#include "FS.h"
#include "SD.h"

//#######################################################################
class FILES_C
    {
private:
    bool                Ready;
    std::list<String>   FileList;
    int                 NumberFiles;

    bool FetchDirectory (void);

public:
         FILES_C        (void);
    bool Begin          (void);
    void ListDirectory  (void);
    };

//#######################################################################
extern FILES_C Files;

