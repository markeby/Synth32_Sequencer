
#pragma once
#include <FS.h>

// ------------- Configuration Section - START

#define DUMP_DATA           0
#define SHOW_UNUSED_META    0
#define MIDI_MAX_TRACKS     32
#define TRACK_PRIORITY      1
#define TRACK_SIZE          50

// ------------- Configuration Section - END

#define ARRAY_SIZE(a) (uint32_t)(sizeof(a)/sizeof((a)[0]))

#if DUMP_DATA
#define DUMPS(s)      printf(F(s))                    // Print a string
#define DUMP(s, v)  { printf(F(s)); printf(v); }      // Print a value (decimal)
#define DUMPX(s, x) { printf(F(s)); printf(x,HEX); }  // Print a value (hex)
#else
#define DUMPS(s)      // Print a string
#define DUMP(s, v)    // Print a value (decimal)
#define DUMPX(s, x)   // Print a value (hex)
#endif // DUMP_DATA

typedef struct
    {
    byte     track;    // the track this was on
    byte     channel;  // the midi channel
    byte     size;     // the number of data bytes
    byte     data[4];  // the data. Only 'size' bytes are valid
    } midi_event;

typedef struct
    {
    byte     track;     // the track this was on
    uint16_t size;      // the number of data bytes
    byte     data[TRACK_SIZE];  // the data. Only 'size' bytes are valid
    } sysex_event;

typedef struct
    {
    byte     track;     // the track this was on
    uint16_t size;      // the number of data bytes
    byte     type;      // meta event type
    union
        {
        byte data[TRACK_SIZE];  // byte data. Only 'size' bytes are valid
        char chars[TRACK_SIZE]; // string data. Only 'size' bytes are valid
        };
    } meta_event;

class FILE_MIDI_C;

class FILE_TRACK_C
    {
public:
          FILE_TRACK_C  (void);
         ~FILE_TRACK_C (void);
    bool      getEndOfTrack (void);
    uint32_t  getLength     (void);
    void      close         (void);
    bool      getNextEvent  (FILE_MIDI_C& mf, uint16_t tickCount);
    int       load          (uint8_t trackId, FILE_MIDI_C& mf);
    void      restart       (void);
    void      syncTime      (void);
    void      dump          (void);

protected:
    void      parseEvent    (FILE_MIDI_C& mf);
    void      reset         (void);

    byte        _trackId;       // the id for this track
    uint32_t    _length;        // length of track in bytes
    uint32_t    _startOffset;   // start of the track in bytes from start of file
    uint32_t    _currOffset;    // offset from start of the track for the next read of SD data
    bool        _endOfTrack;    // true when we have reached end of track or we have encountered an undefined event
    uint32_t    _elapsedTicks;  // the total number of elapsed ticks since last event
    midi_event  _mev;           // data for MIDI callback function - persists between calls for run-on messages
    };

class FILE_MIDI_C
    {
public:
    friend class FILE_TRACK_C;

    FILE_MIDI_C  (void);
    ~FILE_MIDI_C (void);
    inline uint32_t getTickTime             (void)      { return (_tickTime); }
    inline uint16_t getTempo                (void)      { return (_tempo); }
    inline int16_t  getTempoAdjust          (void)      { return (_tempoDelta); }
    inline uint16_t getTicksPerQuarterNote  (void)      { return (_ticksPerQuarterNote); }
    inline uint16_t getTimeSignature        (void)      { return ((_timeSignature[0] << 8) + _timeSignature[1]); }
    void setMicrosecondPerQuarterNote       (uint32_t m);
    void setTempo                           (uint16_t t);
    void setTempoAdjust                     (int16_t t);
    void setTicksPerQuarterNote             (uint16_t ticks);
    void setTimeSignature                   (uint8_t n, uint8_t d);

    //--------------------------------------------------------------
    void        close               (void);
    bool        isEOF               (void);
    int         load                (const char* fname);
    inline byte getFormat           (void)                  { return (_format); };
    inline byte getTrackCount       (void)                  { return (_trackCount); };

    //--------------------------------------------------------------
    void pause                      (bool bMode);
    void restart                    (void);
    bool getNextEvent               (void);
    void processEvents              (uint16_t ticks);
    inline void looping             (bool bMode)                            { _looping = bMode; }
    inline bool isLooping           (void)                                  { return (_looping); }
    inline bool isPaused            (void)                                  { return (_paused); }
    inline void setMidiHandler      (void (*mh) (midi_event* pev))          { _midiHandler = mh; };
    inline void setSysexHandler     (void (*sh) (sysex_event* pev))         { _sysexHandler = sh; };
    inline void setMetaHandler      (void (*mh) (const meta_event* mev))    { _metaHandler = mh; };

    //--------------------------------------------------------------
    void dump (void);

protected:
    void     calcTickTime   (void);                     // called internally to update the tick time when parameters change
    void     initialise     (void);                     // initialize class variables all in one place
    void     synchTracks    (void);                     // synchronize the start of all tracks
    uint16_t tickClock      (void);                     // work out the number of ticks since the last event check

    void (*_midiHandler)    (midi_event* pev);          // callback into user code to process MIDI stream
    void (*_sysexHandler)   (sysex_event* pev);         // callback into user code to process SYSEX stream
    void (*_metaHandler)    (const meta_event* pev);    // callback into user code to process META stream

    byte        _format;                // file format - 0: single track, 1: multiple track, 2: multiple song
    byte        _trackCount;            // number of tracks in file
    uint16_t    _ticksPerQuarterNote;   // time base of file
    uint32_t    _tickTime;              // calculated per tick based on other data for MIDI file
    uint16_t    _lastTickError;         // error brought forward from last tick check
    uint32_t    _lastTickCheckTime;     // the last time (microsec) an tick check was performed
    bool        _synchDone;             // sync up at the start of all tracks
    bool        _paused;                // if true we are currently paused
    bool        _looping;               // if true we are currently looping
    uint16_t    _tempo;                 // tempo for this file in beats per minute
    int16_t     _tempoDelta;            // tempo offset adjustment in beats per minute
    uint8_t     _timeSignature[2];      // time signature [0] = numerator, [1] = denominator

    // file handling
    File           _fd;                     // file descriptor
    FILE_TRACK_C   _track[MIDI_MAX_TRACKS]; // track data for this file
    };

