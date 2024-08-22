
#include <string.h>
#include <SPIFFS.h>

#include "FileMidi.h"
#include "FileMidiHelper.h"

//#######################################################################
  FILE_MIDI_C::FILE_MIDI_C ()
    {
    }

//#######################################################################
void FILE_MIDI_C::initialise (void)
    {
    _trackCount = 0;            // number of tracks in file
    _format = 0;
    _tickTime = _lastTickError = 0;
    _synchDone = false;
    _paused = _looping = false;

    setMidiHandler  (nullptr);
    setSysexHandler (nullptr);
    setMetaHandler  (nullptr);

    // Set MIDI specified standard defaults
    setTicksPerQuarterNote (48); // 48 ticks per quarter note
    setTempo (120);              // 120 beats per minute
    setTempoAdjust (0);          // 0 beats per minute adjustment
    setMicrosecondPerQuarterNote (500000);  // 500,000 microseconds per quarter note
    setTimeSignature (4, 4);     // 4/4 time
    }

//#######################################################################
void FILE_MIDI_C::synchTracks (void)
    {
    for (uint8_t i = 0; i < _trackCount; i++) _track[i].syncTime ();

    _lastTickCheckTime = micros ();
    _lastTickError = 0;
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
    for ( uint8_t z = 0;  z < _trackCount; z++ )
        _track[z].close ();
    _trackCount = 0;
    _synchDone = false;
    _paused = false;

    _fd.close ();
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
    calcTickTime();
    }

//#######################################################################
void FILE_MIDI_C::setTimeSignature (uint8_t n, uint8_t d)
    {
    _timeSignature[0] = n;
    _timeSignature[1] = d;
    calcTickTime();
    }

//#######################################################################
void FILE_MIDI_C::setTicksPerQuarterNote (uint16_t ticks)
    {
    _ticksPerQuarterNote = ticks;
    calcTickTime();
    }

//#######################################################################
// This is the value given in the META message setting tempo
void FILE_MIDI_C::setMicrosecondPerQuarterNote (uint32_t m)
    {
    // work out the tempo from the delta by reversing the calcs in
    // calctickTime - m is already per quarter note
    _tempo = (60 * 1000000L) / m;
    calcTickTime();
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
    if ( (_tempo + _tempoDelta != 0) && _ticksPerQuarterNote != 0 && _timeSignature[1] != 0 )
        {
        _tickTime = (60 * 1000000L) / (_tempo + _tempoDelta); // microseconds per beat
//    _tickTime = (_tickTime * 4) / (_timeSignature[1] * _ticksPerQuarterNote); // microseconds per tick
        _tickTime /= _ticksPerQuarterNote;
        }
    }

//#######################################################################
bool FILE_MIDI_C::isEOF (void)
    {
    bool bEof = true;

    // check if each track has finished
    for (uint8_t i = 0; i < _trackCount && bEof; i++)
        {
        bEof = (_track[i].getEndOfTrack () && bEof);  // breaks at first false
        }

    if ( bEof )
        DUMPS ("\n! EOF");

    // if looping and all tracks done, reset to the start
    if ( bEof && _looping )
        {
        restart ();
        bEof = false;
        }

    return (bEof);
    }

//#######################################################################
// Start pause when true and restart when false
void FILE_MIDI_C::pause (bool bMode)
    {
    _paused = bMode;

    if ( !_paused )         // restarting so adjust the time last checked to now
        _lastTickCheckTime = micros ();
    }

//#######################################################################
// Reset the file to the start of all tracks
void FILE_MIDI_C::restart (void)
    {
    // track 0 contains information that does not need to be reloaded every time,
    // so if we are looping, ignore restarting that track. The file may have one
    // track only and in this case always sync from track 0.
    for (uint8_t i = (_looping && _trackCount > 1 ? 1 : 0); i < _trackCount; i++) _track[i].restart ();

    _synchDone = false;   // force a time resych as well
    }

//#######################################################################
// check if enough time has passed for a MIDI tick and work out how many!
uint16_t FILE_MIDI_C::tickClock (void)
    {
    uint32_t  elapsedTime;
    uint16_t  ticks = 0;

    elapsedTime = _lastTickError + micros () - _lastTickCheckTime;
    if ( elapsedTime >= _tickTime )
        {
        ticks = elapsedTime / _tickTime;
        _lastTickError = elapsedTime - (_tickTime * ticks);
        _lastTickCheckTime = micros ();    // save for next round of checks
        }

    return (ticks);
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
        synchTracks();
        _synchDone = true;
        }

    // check if enough time has passed for a MIDI tick
    if ( (ticks = tickClock()) == 0 )
        return (false);

    processEvents(ticks);

    return (true);
    }

//#######################################################################
void FILE_MIDI_C::processEvents (uint16_t ticks)
    {
    uint8_t n;

    if ( _format != 0 )
        {
        DUMP ("\n-- [", ticks);
        DUMPS ("] TRK ");
        }

#if TRACK_PRIORITY
    // process all events from each track first - TRACK PRIORITY
    for (uint8_t i = 0; i < _trackCount; i++)
        {
        if ( _format != 0 )
            DUMPX ("", i);
        // Limit n to be a sensible number of events in the loop counter
        // When there are no more events, just break out
        // Other than the first event, the others have an elapsed time of 0 (ie occur simultaneously)
        for (n = 0; n < 100; n++)
            {
            if ( !_track[i].getNextEvent (*this, (n == 0 ? ticks : 0)) )
                break;
            }

        if ( (n > 0) && (_format != 0) )
            DUMPS ("\n-- TRK ");
        }
#else // EVENT_PRIORITY
    // process one event from each track round-robin style - EVENT PRIORITY
    bool doneEvents;

    // Limit n to be a sensible number of events in the loop counter
    for (n = 0; (n < 100) && (!doneEvents); n++)
        {
        doneEvents = false;

        for (uint8_t i = 0; i < _trackCount; i++) // cycle through all
            {
            bool b;

            if ( _format != 0 )
                DUMPX ("", i);

            // Other than the first event, the other have an elapsed time of 0 (ie occur simultaneously)
            b = _track[i].getNextEvent(this, (n == 0 ? ticks : 0));
            if ( b && (_format != 0) )
                DUMPS ("\n-- TRK ");
            doneEvents = (doneEvents || b);
            }

        // When there are no more events, just break out
        if ( !doneEvents )
            break;
        }
#endif // EVENT/TRACK_PRIORITY
    }

//#######################################################################
int FILE_MIDI_C::load (const char* fname)
// Load the MIDI file into memory ready for processing
// Return one of the E_* error codes
    {
    uint32_t d32;
    uint16_t d16;

    // open the file for reading
    File _fd = SPIFFS.open (fname);
    if ( !_fd )
        return (true);

    // Read the MIDI header
    // header chunk = "MThd" + <header_length:4> + <format:2> + <num_tracks:2> + <time_division:2>
        {
        char h[MTHD_HDR_SIZE + 1]; // Header characters + nul

        _fd.read ((byte*)h, MTHD_HDR_SIZE + 1);
        h[MTHD_HDR_SIZE] = '\0';

        if ( strcmp (h, MTHD_HDR) != 0 )
            {
            _fd.close ();
            return (true);
            }
        }

    // read header size
    d32 = readMultiByte (_fd, 4);
    if ( d32 != 6 )   // must be 6 for this header
        {
        _fd.close ();
        return (true);
        }

    // read file type
    d16 = readMultiByte (_fd, 2);
    if ( (d16 != 0) && (d16 != 1) )
        {
        _fd.close ();
        return (true);
        }
    _format = d16;

    // read number of tracks
    d16 = readMultiByte (_fd, 2);
    if ( (_format == 0) && (d16 != 1) )
        {
        _fd.close ();
        return (true);
        }
    if ( d16 > MIDI_MAX_TRACKS )
        {
        _fd.close ();
        return (true);
        }
    _trackCount = d16;

    // read ticks per quarter note
    d16 = readMultiByte (_fd, 2);
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
                _fd.close ();
                return (true);
            }
        d16 = framespersecond * resolution;
        }
    _ticksPerQuarterNote = d16;
    calcTickTime();  // we may have changed from default, so recalculate

    // load all tracks
    for (byte z = 0;  z < _trackCount;  z++ )
        {
        int err;

        if ( (err = _track[z].load (z, *this)) != -1 )
            {
            _fd.close ();
            return ((10 * (z + 1)) + err);
            }
        }

    return (false);
    }

//#######################################################################
#if DUMP_DATA
void FILE_MIDI_C::dump (void)
    {
    DUMP ("\nFile format:\t", getFormat ());
    DUMP ("\nTracks:\t\t", getTrackCount ());
    DUMP ("\nTime division:\t", getTicksPerQuarterNote ());
    DUMP ("\nTempo:\t\t", getTempo ());
    DUMP ("\nMicrosec/tick:\t", getTickTime ());
    DUMP ("\nTime Signature:\t", getTimeSignature () >> 8);
    DUMP ("/", getTimeSignature () & 0xf);
    DUMPS ("\n");

    for ( byte z = 0;  z < _trackCount;  z++ )
        {
        _track[i].dump ();
        DUMPS ("\n");
        }
    }
#endif // DUMP_DATA
