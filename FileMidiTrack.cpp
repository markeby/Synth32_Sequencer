#include <string.h>
#include "config.h"
#include "FileMidi.h"
#include "Files.h"
#include "Debug.h"

static const char* Label = "T";
#define DBG(args...) {if(DebugMidiFile){ DebugMsg(Label,this->_trackId,args);}}

//#######################################################################
void FILE_TRACK_C::reset (void)
    {
    this->_length = 0;        // length of track in bytes
    this->_startOffset = 0;   // start of the track in bytes from start of file
    this->restart();
    this->_trackId = 255;
    }

//#######################################################################
FILE_TRACK_C::FILE_TRACK_C (File& fd, FILE_MIDI_C& mf) : _fd(fd), _mf(mf)
    {
    reset ();
    }

//#######################################################################
FILE_TRACK_C::~FILE_TRACK_C ()
    {
    }

//#######################################################################
// size of track in bytes
uint32_t FILE_TRACK_C::getLength (void)
    {
    return _length;
    }

//#######################################################################
// true if end of track has been reached
bool FILE_TRACK_C::getEndOfTrack (void)
    {
    return _endOfTrack;
    }

//#######################################################################
void FILE_TRACK_C::syncTime (void)
    {
    _elapsedTicks = 0;
    }

//#######################################################################
// Start playing the track from the beginning again
void FILE_TRACK_C::restart (void)
    {
    this->_currOffset = 0;
    this->_endOfTrack = false;
    this->_elapsedTicks = 0;
    }

//#######################################################################
// track_event = <time:v> + [<midi_event> | <meta_event> | <sysex_event>]
bool FILE_TRACK_C::getNextEvent (uint16_t tickCount)
    {
    uint32_t deltaT;

    // is there anything to process?
    if ( this->_endOfTrack )
        return (false);

    // move the file pointer to where we left off
    _fd.seek (_startOffset + _currOffset);

    // Work out new total elapsed ticks - include the overshoot from
    // last event.
    _elapsedTicks += tickCount;

    // Get the DeltaT from the file in order to see if enough ticks have
    // passed for the event to be active.
    deltaT = readVarLen (_fd);

    // If not enough ticks, just return without saving the file pointer and
    // we will go back to the same spot next time.
    if ( _elapsedTicks < deltaT )
        return (false);

    // Adjust the total elapsed time to the error against actual DeltaT to avoid
    // accumulation of errors, as we only check for _elapsedTicks being >= ticks,
    // giving positive biased errors every time.
    _elapsedTicks -= deltaT;

    parseEvent ();

    // remember the offset for next time
    _currOffset = _fd.position () - _startOffset;

    // catch end of track when there is no META event
    _endOfTrack = _endOfTrack || (_currOffset >= _length);
    if ( _endOfTrack )
        {
        DBG ("End track");
        }
    return (true);
    }

//#######################################################################
// process the event from the physical file
void FILE_TRACK_C::parseEvent ()
    {
    uint32_t mLen;

    uint8_t etype = _fd.read();
    switch ( etype )
        {
        // ---------------------------- MIDI
        // midi_event = any MIDI channel message, including running status
        // Midi events (status bytes 0x8n - 0xEn) The standard Channel MIDI messages, where 'n' is the MIDI channel (0 - 15).
        // This status byte will be followed by 1 or 2 data bytes, as is usual for the particular MIDI message.
        // Any valid Channel MIDI message can be included in a MIDI file.
        case 0x80 ... 0xBf: // MIDI message with 2 parameters
        case 0xe0 ... 0xef:
            _mev.size = 3;
            _mev.channel = etype & 0x0f;    // mask off the channel
            _mev.data[0] = etype;
            _mev.data[1] = _fd.read ();
            _mev.data[2] = _fd.read ();
            DBG ("Event %X %X %X", etype, _mev.data[1], _mev.data[2]);
            if ( _mf._midiHandler != nullptr )
                (_mf._midiHandler)(&_mev);

            break;

        case 0xc0 ... 0xdf: // MIDI message with 1 parameter
            _mev.size = 2;
            _mev.channel = etype & 0x0f;    // mask off the channel
            _mev.data[0] = etype & 0xf0;    // just the command byte
            _mev.data[1] = _fd.read ();
            if ( _mev.data[0] == 0xC0 )
                {
                DBG ("Program change %X %X", etype, _mev.data[1]);
                }
            else
                {
                DBG ("After touch %X %X", etype, _mev.data[1]);
                }
            if ( _mf._midiHandler != nullptr )
                (_mf._midiHandler)(&_mev);
            break;

        case 0x00 ... 0x7f: // MIDI run on message
            {
            // If the first (status) byte is less than 128 (0x80), this implies that MIDI
            // running status is in effect, and that this byte is actually the first data byte
            // (the status carrying over from the previous MIDI event).
            // This can only be the case if the immediately previous event was also a MIDI event,
            // ie SysEx and Meta events clear running status. This means that the _mev structure
            // should contain the info from the previous message in the structure's channel member
            // and data[0] (for the MIDI command).
            // Hence start saving the data at byte data[1] with the byte we have just read (eType)
            // and use the size member to determine how large the message is (ie, same as before).
            DBG ("Run on mesg %X size d", etype, _mev.size);
            _mev.data[1] = etype;
            for ( byte z = 2;  z < _mev.size;  z++ )
                _mev.data[z] = _fd.read ();  // next byte

            if ( _mf._midiHandler != nullptr )
                (_mf._midiHandler)(&_mev);
            }
            break;

// ---------------------------- SYSEX
        case 0xf0:  // sysex_event = 0xF0 + <len:1> + <data_bytes> + 0xF7
        case 0xf7:  // sysex_event = 0xF7 + <len:1> + <data_bytes> + 0xF7
            {
            sysex_event sev;
            uint16_t index = 0;

            // collect all the bytes until the 0xf7 - boundaries are included in the message
            sev.track = _trackId;
            mLen = readVarLen (_fd);
            sev.size = mLen;
            if ( etype == 0xF0 )       // add space for 0xF0
                {
                sev.data[index++] = etype;
                sev.size++;
                }
            uint16_t minLen = min((uint32_t)sev.size, ARRAY_SIZE(sev.data));
            // The length parameter includes the 0xF7 but not the start boundary.
            // However, it may be bigger than our buffer will allow us to store.
            for ( byte z = index;  z < minLen;  ++z )
                sev.data[z] = _fd.read ();
            if ( sev.size > minLen )
                _fd.seek (sev.size - minLen);
            DBG ("sysex event %X length %d", etype, minLen);
            if ( _mf._sysexHandler != nullptr )
                (_mf._sysexHandler)(&sev);
            }
            break;

// Meta stuff
        case 0xff:  // meta_event = 0xFF + <meta_type:1> + <length:v> + <event_data_bytes>
            {
             meta_event mev;

             etype = _fd.read ();
             mLen =  readVarLen (_fd);

             mev.track = _trackId;
             mev.size = mLen;
             mev.type = etype;
             switch ( etype )
                 {
                 case 0x2f:  // End of track
                     DBG ("End of track");
                     _endOfTrack = true;
                     break;

                 case 0x51:  // set Tempo - really the microseconds per tick
                     {
                     uint32_t val = readMultiByte (_fd, 3);

                     _mf.setMicrosecondPerQuarterNote (val);

                     mev.data[0] = (val >> 16) & 0xFF;
                     mev.data[1] = (val >> 8) & 0xFF;
                     mev.data[2] = val & 0xFF;
                     DBG ("Set tempo %d mSec per tick", val);
                     }
                     break;

                 case 0x58:  // time signature
                     {
                     uint8_t n = _fd.read ();
                     uint8_t d = _fd.read ();

                     _mf.setTimeSignature (n, 1 << d);  // denominator is 2^n
                     _fd.seek (mLen - 2);
                     DBG("Time signature %d/%d", n, 1 << d);
                     mev.data[0] = n;
                     mev.data[1] = d;
                     mev.data[2] = 0;
                     mev.data[3] = 0;
                     }
                     break;

                 case 0x59:  // Key Signature
                     {
                     int8_t sf  = _fd.read ();
                     uint8_t mi = _fd.read ();
                     const char* aaa[] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#" };

                     if ( sf >= -7 && sf <= 7 )
                         {
                         switch ( mi )
                             {
                             case 0:
                                 strcpy (mev.chars, aaa[sf + 7]);
                                 strcat (mev.chars, "M");
                                 break;
                             case 1:
                                 strcpy (mev.chars, aaa[sf + 10]);
                                 strcat (mev.chars, "m");
                                 break;
                             default:
                                 strcpy (mev.chars, "Err"); // error mi
                             }
                         }
                     else
                         strcpy (mev.chars, "Err"); // error sf

                     mev.size = strlen (mev.chars); // change META length
                     DBG ("Key signature <%s>", mev.chars);
                     }
                     break;

                 case 0x00:  // Sequence Number
                     {
                     uint16_t x = readMultiByte (_fd, 2);

                     mev.data[0] = (x >> 8) & 0xFF;
                     mev.data[1] = x & 0xFF;
                     DBG ("Sequence number %d", x);
                     }
                     break;

                 case 0x20:  // Channel Prefix
                     mev.data[0] = readMultiByte (_fd, 1);
                     DBG ("Channel prefix %d", mev.data[0]);
                     break;

                 case 0x21:  // Port Prefix
                     mev.data[0] = readMultiByte (_fd, 1);
                     DBG ("Port prefix %d", mev.data[0]);
                     break;

                 default:
                     {
                     uint8_t minLen = min (ARRAY_SIZE (mev.data), (uint32_t)mLen);

                     for ( byte i = 0;  i < minLen;  ++i )
                         mev.data[i] = _fd.read (); // read next

                     mev.chars[minLen] = '\0'; // in case it is a string
                     if ( mLen > ARRAY_SIZE (mev.data) )
                         _fd.seek (mLen - ARRAY_SIZE (mev.data));
                     DBG ("Array <%s>", mev.chars);
                     }
                     break;
                 }
             if ( _mf._metaHandler != nullptr )
                 (_mf._metaHandler)(&mev);
            }
            break;

        default:        // Unknown so stop playing this track
            DBG ("Uknown MIDI doing end of track");
            _endOfTrack = true;
            break;
        }
    }

//#######################################################################
// return -1 if success, 0 if malformed header, 1 if next track past end of file
int FILE_TRACK_C::load (uint8_t trackId)
    {
    uint32_t  dat32;

    // save the trackid for use later
    _trackId = _mev.track = trackId;
    char  h[MTRK_HDR_SIZE + 1]; // Header characters + nul

    _fd.read ((byte*)h, MTRK_HDR_SIZE);
    h[MTRK_HDR_SIZE] = '\0';
    if ( strcmp (h, MTRK_HDR) != 0 )
        return (0);

    // Row read track chunk size and in bytes. This is not really necessary
    // since the track MUST end with an end of track meta event.
    dat32 = readMultiByte(_fd, 4);
    _length = dat32;

    // save where we are in the file as this is the start of offset for this track
    _startOffset = _fd.position ();
    _currOffset = 0;

    DBG ("  Length %d", _length);

    // Advance the file pointer to the start of the next track;
    if ( !_fd.seek (_startOffset + _length) )
        return (1);

    return (-1);
    }

