
#pragma once
#include <FS.h>

// Miscellaneous
#define MTHD_HDR      "MThd"    // SMF marker
#define MTHD_HDR_SIZE 4         // SMF marker length
#define MTRK_HDR      "MTrk"    // SMF track header marker
#define MTRK_HDR_SIZE 4         // SMF track header marker length

#define BUF_SIZE(x)   (sizeof(x)/sizeof(x[0]))  // Buffer size macro

// Function prototypes ----------------
uint32_t readMultiByte  (File& fd, uint8_t len);
uint32_t readVarLen     (File& fd);
void     dumpBuffer     (byte *p, int len);

