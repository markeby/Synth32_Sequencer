//#######################################################################
// Module:     FrontEnd.h
// Descrption:  Front end processing for sequencer
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#pragma once
#include <lvgl.h>
#include <TFT_eSPI.h>

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
    // Fixed values for graphics and file list
    lv_disp_drv_t   disp_drv;
    lv_indev_drv_t  indev_drv;
    lv_obj_t*       pDropDown;
    lv_obj_t*       pPlayButton;
    lv_style_t      FontD;
    lv_style_t      FontButton;
    lv_style_t      FontCurrent;
    lv_obj_t*       pCurrentFile;
    FILES_C         Files;

    // operating state
    String          OpenFileName;
    bool            FileOpened;
    bool            Playing;
    bool            PlayingN1;

public:
         FONT_END_C         (void);
    void Begin              (void);
    void SilenceMidi        (void);
    void SetPlaying         (bool state);
    void OpenFile           (char* name);
    void OpenFile           (void);
    void Process            (void);
    };

//#######################################################################
extern FONT_END_C FrontEnd;

