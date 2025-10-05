//#######################################################################
// Module:     FileMidi.cpp
// Descrption: MIDI file top level processing
// Creator:    markeby
// Date:       8/16/2024
//#######################################################################
#include <string.h>

#include "config.h"
#include "Debug.h"
#include "FileMidi.h"
#include "Files.h"

static const char* Label = "M";
#define DBG(args...) {if(DebugMidiFile){DebugMsg(Label,DEBUG_NO_INDEX,args);}}

//#######################################################################
  FILE_MIDI_C::FILE_MIDI_C ()
    {
    setMidiHandler  (nullptr);
    setSysexHandler (nullptr);
    setMetaHandler  (nullptr);
    }

//#######################################################################
void FILE_MIDI_C::Startup (void)
    {
    _trackCount = 0;
    _format        = 0;
    _lastTickError = 0;
    _tickTime      = 0;

    for ( int z = 0;  z < MIDI_MAX_TRACKS; z++ )
        _track[z] = nullptr;

    close ();   // this will initialize the remaining items

    // Set MIDI specified standard defaults
    setTicksPerQuarterNote          (48);       // 48 ticks per quarter note
    setTempo                        (120);      // 120 beats per minute
    setTempoAdjust                  (0);        // 0 beats per minute adjustment
    setMicrosecondPerQuarterNote    (500000);   // 500,000 microseconds per quarter note
    setTimeSignature                (4, 4);     // 4/4 time
    }

//#######################################################################
void FILE_MIDI_C::synchTracks (void)
    {
    for ( uint8_t z = 0;  z < _trackCount;  z++ )
        _track[z]->SyncTime ();
    }

//#######################################################################
FILE_MIDI_C::~FILE_MIDI_C ()
    {
    this->close ();
    }

//#######################################################################
// Close out - should be ready for the next file
void FILE_MIDI_C::close ()
    {
    for ( int z = 0;  z < _trackCount; z++ )
        {
        if ( _track[z] != nullptr )
            {
            delete (_track[z]);
            _track[z] = nullptr;
            }
        }
    _trackCount = 0;
    _synchDone = false;
    _paused = false;
    _lastTickError = 0;
    }

//#######################################################################
void FILE_MIDI_C::setTempoAdjust (int16_t t)
    {
    if ( (t + _tempo) > 0 )
        _tempoDelta = t;
    calcTickTime ();
    }

//#######################################################################
void FILE_MIDI_C::setTempo (uint16_t t)
    {
    if ( (_tempoDelta + t) > 0 )
        _tempo = t;
    calcTickTime ();
    }

//#######################################################################
void FILE_MIDI_C::setTimeSignature (uint8_t n, uint8_t d)
    {
    _timeSignature[0] = n;
    _timeSignature[1] = d;
    calcTickTime ();
    }

//#######################################################################
void FILE_MIDI_C::setTicksPerQuarterNote (uint16_t ticks)
    {
    _ticksPerQuarterNote = ticks;
    calcTickTime ();
    }

//#######################################################################
// This is the value given in the META message setting tempo
void FILE_MIDI_C::setMicrosecondPerQuarterNote (uint32_t m)
    {
    // work out the tempo from the delta by reversing the calcs in
    // calctickTime - m is already per quarter note
    _tempo = (60 * 1000000L) / m;
    calcTickTime ();
    }

//#######################################################################
void FILE_MIDI_C::calcTickTime (void)
// 1 tick = microseconds per beat / ticks per Q note
// The variable "microseconds per beat" is specified by a MIDI event carrying
// the set tempo meta message. If it is not specified then it is 500,000 microseconds
// by default, which is equivalent to 120 beats per minute.
// If the MIDI time division is 60 ticks per beat and if the microseconds per beat
// is 500,000, then 1 tick = 500,000 / 60 = 8333.33 microseconds.
    {
    if ( (_tempo + _tempoDelta != 0) && (_ticksPerQuarterNote != 0) )
        {
        _tickTime = (60 * 1000000L) / (_tempo + _tempoDelta); // microseconds per beat
//    _tickTime = (_tickTime * 4) / (_timeSignature[1] * _ticksPerQuarterNote); // microseconds per tick
        _tickTime /= _ticksPerQuarterNote;
        }
    }

//#######################################################################
bool FILE_MIDI_C::isEOF (void)
    {
    for ( int z = 0;  z < _trackCount;  z++ )
        {
        if ( ! _track[z]->getEndOfTrack () )
            return (false);
        }
    return (true);
    }

//#######################################################################
// Start pause when true and restart when false
void FILE_MIDI_C::pause (bool state)
    {
    DBG ("Setting pause state = %d", state);
    _paused = state;
    if ( state )
        MidiSilence();                         // stop any sound as we are pausing.
    else
        StartClocking ();
    }

//#######################################################################
// Reset the file to the start of all tracks
void FILE_MIDI_C::Restart (void)
    {
    for ( int z = 0;  z < _trackCount;  z++ )
        _track[z]->Restart ();

    _synchDone = false;                     // force a time resych as well
    _lastTickCheckTime = micros ();         // reset the time interval baseline
    }

//#######################################################################
// check if enough time has passed for a MIDI tick and work out how many!
uint16_t FILE_MIDI_C::tickClock (void)
    {
    uint32_t  elapsedTime;
    uint16_t  ticks = 0;

    elapsedTime = _lastTickError + micros() - _lastTickCheckTime;
    if (elapsedTime >= _tickTime)
        {
        ticks = elapsedTime/_tickTime;
        _lastTickError = elapsedTime - (_tickTime * ticks);
        _lastTickCheckTime = micros();    // save for next round of checks
        }

    return(ticks);
    }

//#######################################################################
void FILE_MIDI_C::StartClocking ()
    {
    _lastTickCheckTime = micros ();         // reset the time interval baseline
    }

//#######################################################################
bool FILE_MIDI_C::getNextEvent (void)
    {
    uint16_t  ticks;
    // if we are paused we are paused!
    if ( _paused )
        return (false);

    // sync start all the tracks if we need to
    if ( !_synchDone )
        {
        synchTracks ();
        _synchDone = true;
        }

    // check if enough time has passed for a MIDI tick
    ticks = tickClock ();
    if ( ticks == 0 )
        return (false);

    processEvents (ticks);

    return (true);
    }

//#######################################################################
void FILE_MIDI_C::processEvents (uint16_t ticks)
    {
#if TRACK_PRIORITY
    // process all events from each track first - TRACK PRIORITY
    for ( int z = 0;  z < _trackCount;  z++ )
        {
        // Limit n to be a sensible number of events in the loop counter
        // When there are no more events, just break out
        // Other than the first event, the others have an elapsed time of 0 (ie occur simultaneously)
        for ( int n = 0;  n < 100;  n++ )
            {
            if ( !_track[z]->getNextEvent ((n == 0 ? ticks : 0)) )
                break;
            }
        }
#else // EVENT_PRIORITY
    // process one event from each track round-robin style - EVENT PRIORITY
    bool doneEvents;

    // Limit n to be a sensible number of events in the loop counter
    for ( int n = 0;  (n < 100) && (!doneEvents);  n++ )
        {
        doneEvents = false;

        for ( int z  = 0;  z < _trackCount;  z++ ) // cycle through all
            {
            bool b;

            // Other than the first event, the other have an elapsed time of 0 (ie occur simultaneously)
            b = _track[z]->getNextEvent ((n == 0 ? ticks : 0));
            doneEvents = (doneEvents || b);
            }

        // When there are no more events, just break out
        if ( !doneEvents )
            break;
        }
#endif // EVENT/TRACK_PRIORITY
    }

//#######################################################################
void FILE_MIDI_C::LoadMeta ()
    {

    for ( int z = 0;  z < _trackCount;  z++ )
        {
        for ( int n = 0;  n < 20;  n++ )
            {
            if (_track[z]->getNextEvent () )
                break;
            }
        }
    }

//#######################################################################
int FILE_MIDI_C::Load (File& fd)
    {
    uint32_t d32;
    uint16_t d16;

    this->Startup ();

    char h[MTHD_HDR_SIZE + 1]; // Header characters + nul
    fd.read ((byte*)h, MTHD_HDR_SIZE);
    h[MTHD_HDR_SIZE] = '\0';
    if ( strcmp (h, MTHD_HDR) != 0 )
        return (1);

    // read header size
    d32 = ReadMultiByte (fd, 4);
    if ( d32 != 6 )   // must be 6 for this header
        return (1);

    // read file type
    d16 = ReadMultiByte (fd, 2);
    if ( (d16 != 0) && (d16 != 1) )
        return (1);
    _format = d16;

    // read number of tracks
    d16 = ReadMultiByte (fd, 2);
    if ( (_format == 0) && (d16 != 1) )
        return (1);
    if ( d16 > MIDI_MAX_TRACKS )
        return (1);
    _trackCount = d16;

    // read ticks per quarter note
    d16 = ReadMultiByte (fd, 2);
    if ( d16 & 0x8000 ) // top bit set is SMTE format
        {
        int framespersecond = (d16 >> 8) & 0x00ff;
        int resolution      = d16 & 0x00ff;

        switch ( framespersecond )
            {
            case 232:
                framespersecond = 24; break;
            case 231:
                framespersecond = 25; break;
            case 227:
                framespersecond = 29; break;
            case 226:
                framespersecond = 30; break;
            default:
                return (1);
            }
        d16 = framespersecond * resolution;
        }
    _ticksPerQuarterNote = d16;
    calcTickTime ();  // we may have changed from default, so recalculate

    // load all tracks
    for (byte z = 0;  z < _trackCount;  z++ )
        {
        int err;
        _track[z] = new FILE_TRACK_C (fd, *this);
        DBG ("Creating track #%d", z);
        if ( (err = _track[z]->load (z)) != -1 )
            return ((10 * (z + 1)) + err);
        }

    LoadMeta ();
    Restart  ();
    pause (true);
    return   (0);
    }

//#######################################################################
String FILE_MIDI_C::metaDataString ()
    {
    String str;

    for ( int z = 0;  z < _trackCount;  z++ )
       str +=  _track[z]->metaDataString ();
    return (str);
    }

