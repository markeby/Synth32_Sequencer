//#######################################################################
// Module:     FrontEnd.h
// Descrption: Front end processing for sequencer
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#pragma once
#include <lvgl.h>
#include <TFT_eSPI.h>

#include "Config.h"
#include "Files.h"

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

    void    Restart     (void);

public:
    bool            MetaDataDone;

            FONT_END_C  (void);
    void    Begin       (void);
    void    SetPlaying  (bool state);
    void    OpenFile    (char* name);
    void    Process     (void);
    String  DumpMeta    (void)  { return (Files.MidiF.metaDataString ()); }
    };

//#######################################################################
extern FONT_END_C FrontEnd;

