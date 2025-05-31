//#######################################################################
// Module:     FrontEnd.cpp
// Descrption: Front end processing for sequencer
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include <Arduino.h>
#include <TAMC_GT911.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "Debug.h"
#include "Files.h"
#include "FrontEnd.h"
#include "SerialMonitor.h"

static const char* Label = "F";
#define DBG(args...) {if(DebugState){DebugMsg(Label,DEBUG_NO_INDEX,args);}}
static const char* Label1 = "I";
#define DBGI(args...) {if(DebugMidi){DebugMsg(Label1,DEBUG_NO_INDEX,args);}}

#define TITILE_TEXT &FreeSansBoldOblique24pt7b
#define FILE_TEXT   &FreeSansBoldOblique18pt7b
#define ITEM_TEXT   &FreeSansOblique12pt7b

#define TOUCH_SDA    33
#define TOUCH_SCL    32
#define TOUCH_INT    21
#define TOUCH_RST    25
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 320

static TFT_eSPI tFt  = TFT_eSPI   ();     // Invoke library, pins defined in User_Setup.h
static TAMC_GT911 touchPanel = TAMC_GT911 (TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

//#######################################################################
void MidiCallback(midi_event *pev)
    {
    if ( (pev->data[0] >= 0x80) && (pev->data[0] < 0xA0) )
        {
//        Serial1.write (pev->data[0] | pev->channel);
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
    this->Selected = 0;
    this->State = STATE_C::MENU;
    this->IdleWait = 0;
    this->TouchError = 4;
    }

//#######################################################################
void FONT_END_C::Begin ()
    {
    Serial1.begin (31250, SERIAL_8N1, RXD1, TXD1, false);

    tFt.init ();
    tFt.setRotation (0);

    touchPanel.setRotation (ROTATION_INVERTED);
    touchPanel.begin ();

    if ( this->Files.Begin () )
        printf ("*** No Files available\n");
    else
        printf ("\t>>> File system ready\n");

    this->Silence ();
    this->State = STATE_C::GO_MENU;
    }

//#######################################################################
void FONT_END_C::Silence ()
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
bool FONT_END_C::TouchIt ()
    {
    if ( this->TouchError == 0 )
        Monitor.Reset ("Touch error reset");

    touchPanel.read ();

    if ( this->IdleWait )
        {
        this->IdleWait -= DeltaTimeMilli;
        if ( this->IdleWait < 0 )
            this->IdleWait = 0;
        return (false);
        }

    if ( touchPanel.isTouched )
        {
        int z = touchPanel.touches - 1;
        this->TouchX = touchPanel.points[z].x;
        this->TouchY = touchPanel.points[z].y;
        if ( (this->TouchX == -1) && (this->TouchY == -1) )
            {
            this->TouchError--;
            return (false);
            }
        DBG ("Touch display state %d   X = %d   Y = %d", (byte)this->State, this->TouchX, this->TouchY);
        return (true);
        }
    return (false);
    }

//#######################################################################
void FONT_END_C::Directory ()
    {
    DBG ("Display directory");
    tFt.fillScreen (tFt.color565(0x10, 0x10, 0x10));
    tFt.setTextColor (TFT_WHITE);
    tFt.setCursor (12, 5);
    tFt.setFreeFont (TITILE_TEXT);
    tFt.drawString ("FILES", 90, 8);
    tFt.drawString ("----------", 78, 28);

    tFt.setFreeFont (ITEM_TEXT);
    for (int z = 0;  z < 8;  z++ )
        {
        String st = this->Files.FetchFileName (z);
        tFt.drawString (st.c_str (), 18, 70 + (z * 32));
        }
    }

//#######################################################################
void FONT_END_C::TrackSelect ()
    {
    tFt.fillScreen (tFt.color565 (0x10, 0x10, 0x10));
    tFt.setTextColor (TFT_WHITE);
    tFt.setCursor (12, 5);
    tFt.setFreeFont (FILE_TEXT);
    tFt.drawString (this->OpenFileName, 18, 1);
    tFt.setFreeFont (ITEM_TEXT);
    byte cnt = this->Files.MidiF.getTrackCount ();
    for (byte z = 1;  z < cnt;  z++ )
        {
        String st = "Track " + String (z, DEC);
        tFt.drawString (st.c_str (), 38, 16 + (z * 32));
        }
    }

//#######################################################################
void FONT_END_C::PlayingSelect ()
    {
    DBG ("Play menu display");
    tFt.fillScreen (tFt.color565(0x10, 0x10, 0x10));
    tFt.setTextColor (TFT_WHITE);
    tFt.setCursor (12, 5);
    tFt.setFreeFont (TITILE_TEXT);
    tFt.drawString ("PLAYING", 68, 8);
    tFt.drawString ("-------------", 58, 28);

    tFt.setFreeFont (FILE_TEXT);
    tFt.drawString (this->OpenFileName, 18, 70);
    }

//#######################################################################
void FONT_END_C::SetTrackSelected ()
    {
    static const char *marker = "#";

    if ( (this->Selected < 1) || (this->Selected > this->Files.MidiF.getTrackCount ()) )
        return;

    int y = 16 + (this->Selected * 32);
    if ( this->Files.MidiF._track[this->Selected]->IsSelected () )
        {
        DBG ("Deselect track %d", this->Selected);
        this->Files.MidiF._track[this->Selected]->SetSelected (false);
        tFt.setTextColor (tFt.color565 (0x10, 0x10, 0x10));
        tFt.drawString(marker, 1, y);
        }
    else
        {
        DBG ("Select track %d", this->Selected);
        this->Files.MidiF._track[this->Selected]->SetSelected (true);
        tFt.setTextColor (TFT_WHITE);
        tFt.drawString(marker, 1, y);
        }
    }

//#######################################################################
void FONT_END_C::OpenSelected ()
    {
    this->OpenFileName = this->Files.FetchFileName (this->Selected);
    if ( this->OpenFileName.isEmpty () )
        return;
    DBG("Selected file: %s", this->OpenFileName.c_str());
    if ( this->Files.OpenFile (this->OpenFileName) )
        this->State = STATE_C::GO_MENU;
    else
        this->State = STATE_C::GO_TRACK;
    }

//#######################################################################
void FONT_END_C::Process ()
    {
    switch ( this->State )
        {
        case STATE_C::GO_MENU:
            DBG ("Display menu of files");
            this->Directory ();
            this->State = STATE_C::MENU;
            break;

        case STATE_C::GO_TRACK:
            DBG ("Display track select");
            this->TrackSelect ();
            this->State = STATE_C::TRACK;
            break;

        case STATE_C::GO_PLAY:
            DBG ("Display play control");
            this->PlayingSelect ();
            this->State = STATE_C::PLAY;
            break;

        case STATE_C::TRACK:
            if ( TouchIt () )
                {
                if ( this->TouchY > 47  )
                    {
                    this->Selected = ((this->TouchY - 48) / 30) + 1;
                    DBG ("Track menu select = %d", this->Selected);
                    this->SetTrackSelected ();
                    this->IdleWait = 700;
                    }
                else if ( this->TouchY < 21 )
                    this->State = STATE_C::GO_PLAY;
                }
            break;

        case STATE_C::MENU:
            if ( TouchIt () )
                {
                if ( this->TouchY > 71  )
                    {
                    this->Selected = (this->TouchY - 72) / 30;
                    DBG ("File menu select = %d", this->Selected);
                    this->OpenSelected ();
                    this->IdleWait = 700;
                    }
                }
            break;

        case STATE_C::PLAY:
            if ( this->Files.Process () )
                this->State = STATE_C::GO_MENU;
            break;

        default:
            break;

        }
    }

//#######################################################################
FONT_END_C FrontEnd;

