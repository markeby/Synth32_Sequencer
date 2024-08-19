#include <string.h>
#include <FS.h>
#include "FileMidiHelper.h"

//#######################################################################
// read fixed length parameter from input
uint32_t readMultiByte (File& fd, uint8_t len)
    {
    uint32_t  val = 0L;

    for ( int z = 0;  z < len;  z++ )
        val = (val << 8) + fd.read ();

    return (val);
    }

//#######################################################################
uint32_t readVarLen (File& fd)
    {
    uint32_t  val = 0;
    char      zc;

    do  {
        zc = fd.read ();
        val = (val << 7) + (zc & 0x7f);
        }
    while ( zc & 0x80 );

    return (val);
    }

//#######################################################################
#if DUMP_DATA
// Formatted dump of a buffer of data
void dumpBuffer (byte* pb, int len)
    {
    for ( int z = 0;  z < len;  z++, pb++ )
        {
        if ( (z != 0) && ((z& 0x0f) == 0) ) // every 16 characters
            DUMPS ("\n");

        DUMPS (" ");
        if ( *pb <= 0xf )
            DUMPX ("0", *pb)
        else
            DUMPX ("", *pb);
        }
    }
#endif
