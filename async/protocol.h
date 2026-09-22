#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAGIC_HEADER 0x44424c4b // "DBLK" magic visual validator

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;         // Validation sequence pattern 
    uint64_t block_index;   // Sequence sequence identity index
    uint64_t total_blocks;  // Tracker of items elements processed so far
    uint32_t payload_size;  // Exact length of the trailing arbitrary raw data
} DataBlockHeader;
#pragma pack(pop)

#endif
