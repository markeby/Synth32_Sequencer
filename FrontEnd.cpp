//#######################################################################
// Module:     FrontEnd.cpp
// Descrption: Front end processing for sequencer
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include <Arduino.h>

#include "Debug.h"
#include "Files.h"
#include "TouchGT911.h"
#include "SerialMonitor.h"
#include "FrontEnd.h"

static const char* Label = "F";
#define DBG(args...) {if(DebugState){DebugMsg(Label,DEBUG_NO_INDEX,args);}}
static const char* Label1 = "I";
#define DBGI(args...) {if(DebugMidi){DebugMsg(Label1,DEBUG_NO_INDEX,args);}}

#define TITILE_TEXT &FreeSansBoldOblique24pt7b
#define FILE_TEXT   &FreeSansBoldOblique18pt7b
#define ITEM_TEXT   &FreeSansOblique12pt7b

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 480;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 6];
static TFT_eSPI tft = TFT_eSPI ();

//#######################################################################
static void touchPadRead (lv_indev_drv_t* indev_driver, lv_indev_data_t* data)
    {
    uint16_t touchX, touchY;

    bool touched = TouchPanel.Scan ();
    if ( !touched )
        data->state = LV_INDEV_STATE_REL;
    else
        {
        /*Set the coordinates*/
        data->point.x = TouchPanel.GetX ();
        data->point.y = TouchPanel.GetY ();
        data->state = LV_INDEV_STATE_PR;
        }
    }

//#######################################################################
static void displayFlush (lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
    {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite ();
    tft.setAddrWindow (area->x1, area->y1, w, h);
    tft.pushColors ((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite ();

    lv_disp_flush_ready (disp);
    }

//#######################################################################
static void eventSelectFile (lv_event_t* e)
    {
    lv_event_code_t code = lv_event_get_code (e);
    if ( code == LV_EVENT_VALUE_CHANGED )
        {
        lv_obj_t* obj = lv_event_get_target (e);

        char buf[120];
        lv_dropdown_get_selected_str (obj, buf, sizeof(buf));
        FrontEnd.OpenFile (buf);
        }
    }

//#######################################################################
static void eventPlayPause (lv_event_t * e)
    {
    lv_event_code_t code = lv_event_get_code (e);

    if ( code == LV_EVENT_VALUE_CHANGED )
        {
        lv_obj_t* obj = lv_event_get_target (e);
        FrontEnd.SetPlaying (lv_obj_get_state (obj) & LV_STATE_CHECKED);
        }
    }

//#######################################################################
void MidiCallback (midi_event *pev)
    {
    if ( (pev->data[0] >= 0x80) && (pev->data[0] < 0xA0) )
        {
        Serial1.write (pev->data[0]);
        Serial1.write (&pev->data[1], pev->size-1);
        DBGI ("For channel %d Sent %X %X %X", pev->channel, pev->data[0], pev->data[1], pev->data[2]);
        }
    }

//#######################################################################
void SysexCallback(sysex_event *pev)
    {
    if ( DebugMidi )
        {
        printf ("[I] {sysex} %d mS   Track: %d   Data:", millis(), pev->track);
        for ( int z = 0;  z < pev->size;  z++ )
           printf (" 0x%X", pev->data[z]);
        printf ("\n");
        }
    }

//#######################################################################
//#######################################################################
FONT_END_C::FONT_END_C ()
    {
    FileOpened = false;
    Playing    = false;
    }

//#######################################################################
void FONT_END_C::Begin ()
    {
    // Start MIDI output port
    Serial1.begin (31250, SERIAL_8N1, RXD1, TXD1, false);

    TouchPanel.Begin ();

    lv_init();

    tft.begin ();
    tft.setRotation (0);
    tft.fillScreen (TFT_RED);
    delay (500);
    tft.fillScreen (TFT_GREEN);
    delay (500);
    tft.fillScreen (TFT_BLUE);
    delay (500);
    tft.fillScreen (TFT_BLACK);

    lv_disp_draw_buf_init (&draw_buf, buf, NULL, screenWidth * screenHeight / 6);

    lv_disp_drv_init (&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = displayFlush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register (&disp_drv);

    lv_indev_drv_init (&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchPadRead;
    lv_indev_drv_register (&indev_drv);

    String str;
    if ( this->Files.Begin (str) )
        printf ("*** No Files available ***\n");
    else
        printf ("\t>>> File system ready\n");

    lv_obj_t* panel = lv_obj_create (lv_scr_act ());
    lv_obj_set_size (panel, screenWidth, screenHeight);

    // Dropdown font
    lv_style_init (&FontD);
    lv_style_set_text_font (&FontD, &lv_font_montserrat_18);

    // Button font
    lv_style_init (&FontButton);
    lv_style_set_text_font (&FontButton, &lv_font_montserrat_30);

    // Currently selected item font
    lv_style_init (&FontCurrent);
    lv_style_set_text_font (&FontCurrent, &lv_font_montserrat_18);
    lv_style_set_text_color (&FontCurrent, lv_palette_main (LV_PALETTE_DEEP_PURPLE));

    // Setup the dropdown file list
    pDropDown = lv_dropdown_create (panel);
    lv_obj_set_width        (pDropDown, 266);
    lv_obj_align            (pDropDown, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_add_style        (pDropDown, &FontD, 0);
    lv_obj_t *dplist = lv_dropdown_get_list (pDropDown);
    lv_obj_add_style        (dplist, &FontD, 0);
    lv_dropdown_set_options (pDropDown, str.c_str ());
    lv_obj_add_event_cb     (pDropDown, eventSelectFile, LV_EVENT_ALL, NULL);

    // Setup the play/pause button
    pPlayButton = lv_btn_create (panel);
    lv_obj_add_event_cb (pPlayButton, eventPlayPause, LV_EVENT_ALL, NULL);
    lv_obj_align        (pPlayButton, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag     (pPlayButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height   (pPlayButton, LV_SIZE_CONTENT);
    lv_obj_t* label = lv_label_create (pPlayButton);
    lv_label_set_text (label, "Play/Pause");
    lv_obj_center     (label);
    lv_obj_add_style  (label, &FontButton, 0);

    // Setup the currently selected file
    pCurrentFile = lv_label_create (panel);
    lv_obj_align      (pCurrentFile, LV_ALIGN_BOTTOM_MID, 0, -90);
    lv_obj_add_style  (pCurrentFile, &FontCurrent, 0);
    lv_label_set_text (pCurrentFile, "");            // clear the text

    SilenceMidi ();
    }

//#######################################################################
void FONT_END_C::SilenceMidi ()
    {
    midi_event ev;

    ev.data[0] = 0xb0;
    ev.data[1] = 120;
    ev.data[2] = 0;

    for ( ev.channel = 0;  ev.channel < 2;  ev.channel++ )
        {
        Serial1.write (ev.data[0] | ev.channel);
        Serial1.write (&ev.data[1], 2);
        DBGI ("Sent %X %X %X", ev.data[0], ev.data[1], ev.data[2]);
        }
    }

//#######################################################################
void FONT_END_C::SetPlaying (bool state)
    {
    if ( FileOpened )       // File is good so, play
        Playing = state;
    else                    // Not a valid file so reset the button
        lv_obj_clear_state (pPlayButton,  LV_STATE_CHECKED);
    }

//#######################################################################
void FONT_END_C::OpenFile (char* name)
    {
    OpenFileName = name;
    OpenFileName += ".mid";

    lv_obj_clear_state (pPlayButton,  LV_STATE_CHECKED);

    lv_label_set_text (pCurrentFile, "");            // clear the text
    if ( OpenFileName.isEmpty () )
        return;
    OpenFile ();
    lv_label_set_text_fmt (pCurrentFile, "%s\n  Tracks: %d", name, Files.GetTrackCount ());
    }

//#######################################################################
void FONT_END_C::OpenFile ()
    {
    DBG("Selected file: %s", OpenFileName.c_str());
    if ( !Files.OpenFile (OpenFileName) )
        FileOpened = true;
    }

//#######################################################################
void FONT_END_C::Process ()
    {
    lv_timer_handler ();

    if ( Playing )
        {
        PlayingN1 = true;
        if ( this->Files.Process () )
            {
            Playing = false;
            lv_obj_clear_state (pPlayButton,  LV_STATE_CHECKED);
            OpenFile ();            // reopen the same file
            }
        }
    else
        {
        if ( PlayingN1 )
            {
            PlayingN1 = false;
            SilenceMidi ();                         // stop any sound as we are pausing.
            }
        }
    }

//#######################################################################
FONT_END_C FrontEnd;

