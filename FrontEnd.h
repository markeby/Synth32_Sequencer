//#######################################################################
// Module:     FrontEnd.h
// Descrption:  Front end processing for sequencer
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#pragma once
#include "Files.h"

//#######################################################################
enum class STATE_C: uint8_t
        {
        GO_MENU = 0,
        GO_PLAY,
        MENU,
        TRACK,
        PLAY,
        PAUSE,
        STOP,
        END,
        };


//#######################################################################
class FONT_END_C
    {
private:
    int         Selected;
    STATE_C     State;
    short       TouchX;
    short       TouchY;
    byte        TouchError;
    String      OpenFileName;
    float       IdleWait;
    FILES_C     Files;

public:
         FONT_END_C         (void);
    void Begin              (void);
    void Silence            (void);
    bool TouchIt            (void);
    void OpenSelected       (void);
    void PlayingSelect      (void);
    void TrackSelect        (void);
    void Directory          (void);
    void Process            (void);
    };

//#######################################################################
extern FONT_END_C FrontEnd;

