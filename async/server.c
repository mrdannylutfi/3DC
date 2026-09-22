/*
Deserialized Server Receipt Logic
This consumer module processes incoming streams using non-blocking calls. 
It cleanly resolves fragmentation issues by buffering stream runtime metrics until a packet can be validated and unpacked.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include "protocol.h"

typedef enum {
    STATE_READING_HEADER,
    STATE_READING_PAYLOAD
} RecvState;

typedef struct {
    int fd;
    RecvState state;
    size_t bytes_read;
    DataBlockHeader header;
    uint8_t *payload_buffer;
    size_t payload_bytes_read;
} ReceiverSession;

/**
 * @brief Processes incoming stream chunks, enforces validation protocols, 
 *        and deserializes objects out of network-ordered layouts.
 */
int handle_deserialization_read(ReceiverSession *session) {
    while (1) {
        // Step 1: Accumulate raw stream data to reconstitute the struct header framework
        if (session->state == STATE_READING_HEADER) {
            uint8_t *header_ptr = (uint8_t *)&session->header;
            size_t target_len = sizeof(DataBlockHeader);
            
            ssize_t bytes = recv(session->fd, header_ptr + session->bytes_read, target_len - session->bytes_read, 0);
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; // Stream dry, wait for next cycle
                return -1; // Transport failure
            }
            if (bytes == 0) return -2; // Remote counterpart closed execution connection cleanly

            session->bytes_read += bytes;

            if (session->bytes_read == target_len) {
                // Deserialize variables using host execution architecture patterns
                uint32_t magic = ntohl(session->header.magic);
                if (magic != MAGIC_HEADER) {
                    fprintf(stderr, "[Fatal Protocol Violation] Magic identifier invalid: 0x%X\n", magic);
                    return -3;
                }

                uint64_t index = be64toh(session->header.block_index);
                uint64_t total = be64toh(session->header.total_blocks);
                uint32_t payload_size = ntohl(session->header.payload_size);

                printf("[Server Received Header] Object Index: %lu | Total Chain: %lu | Expecting Data Size: %u bytes\n",
                       index, total, payload_size);

                // Prepare variable-length container structures dynamically
                session->payload_buffer = (uint8_t *)malloc(payload_size);
                if (!session->payload_buffer && payload_size > 0) {
                    return -4; // Out of memory allocation safeguards
                }
                
                session->state = STATE_READING_PAYLOAD;
                session->payload_bytes_read = 0;
            }
        }

        // Step 2: Extract data payloads mapping smoothly back to the struct allocations
        if (session->state == STATE_READING_PAYLOAD) {
            uint32_t target_payload_len = ntohl(session->header.payload_size);
            
            if (target_payload_len == 0) {
                // Edge case: valid container structure featuring empty payloads
                printf("[Server Processing Notification] Block object extracted cleanly without payload structures.\n");
                return 1; 
            }

            ssize_t bytes = recv(session->fd, session->payload_buffer + session->payload_bytes_read, 
                                 target_payload_len - session->payload_bytes_read, 0);
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
                return -1;
            }
            if (bytes == 0) return -2;

            session->payload_bytes_read += bytes;

            if (session->payload_bytes_read == target_payload_len) {
                printf("[Server Processed Object] Block object %lu parsed successfully without drop leakage.\n", 
                       be64toh(session->header.block_index));
                
                /*
                 * Process data payload objects here:
                 * execute_business_logic(session->payload_buffer, target_payload_len);
                 */

                // Free payload session allocations safely after processing completes
                free(session->payload_buffer);
                session->payload_buffer = NULL;
                
                // Reset state machine parameters to listen for subsequent frames on this persistent connection
                session->state = STATE_READING_HEADER;
                session->bytes_read = 0;
                return 1; 
            }
        }
    }
}
