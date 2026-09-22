#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

/**client  
 * @brief Callback triggered after the host server indicates it is ready 
 *        to receive data blocks.
 * 
 * @param client_fd The socket file descriptor connected to the host server.
 * @param index     The index number of the current datablock object.
 * @param total     The cumulative count of datablock objects sent so far.
 * @param user_data Optional pointer to application-specific state or context.
 */
 
 
void onDataBlocktReceived(int client_fd, size_t index, size_t total, void *user_data) {
    // 1. Validate the socket descriptor
    if (client_fd < 0) {
        fprintf(stderr, "[Error] Invalid socket descriptor passed to handler.\n");
        return;
    }

    // 2. Log or track the state transmission progress
    printf("[Info] Server Ready. Processing Datablock Index: %zu | Total Sent So Far: %zu\n", 
           index, total);

    /* 
     * 3. Example Execution Logic:
     * Imagine fetching the actual payload data from your application context 
     * and sending it over the ready socket.
     */
    if (user_data != NULL) {
        // Example payload placeholder (e.g., pulling from a buffer array)
        // const char *datablock_payload = ((const char **)user_data)[index];
        // size_t payload_len = 4096; 
        //
        // ssize_t bytes_sent = send(client_fd, datablock_payload, payload_len, 0);
        // if (bytes_sent < 0) {
        //     perror("[Error] Failed to send datablock payload");
        // }
    }
}


#include <stdio.h>
#include <sys/socket.h>

/* Application context wrapping references to your data memory blocks */
typedef struct {
    const uint8_t *raw_buffer;
    size_t buffer_size;
    size_t block_size;
} AppContext;

/* Helper mapping logic to handle complete, robust blocking network writes */
static ssize_t send_all(int fd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const uint8_t *ptr = (const uint8_t *)buf;
    
    while (total_sent < len) {
        ssize_t sent = send(fd, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // Socket error or client disconnected
        }
        total_sent += sent;
    }
    return (ssize_t)total_sent;
}

/**
 * @brief Handles serializing and sending a structured data block protocol
 *        over a synchronous blocking network socket interface.
 */
void onDataBlockObjectReceived(int client_fd, size_t index, size_t total, void *user_data) {
    if (client_fd < 0 || !user_data) {
        fprintf(stderr, "[Error] Invalid inputs provided to transmission worker.\n");
        return;
    }

    AppContext *ctx = (AppContext *)user_data;
    
    // Calculate memory offset positions dynamically based on current index tracking
    size_t offset = index * ctx->block_size;
    if (offset >= ctx->buffer_size) {
        fprintf(stderr, "[Error] Requested index out of bounds of the memory region.\n");
        return;
    }

    // Determine the exact payload slice footprint window
    size_t current_payload_len = ctx->block_size;
    if (offset + current_payload_len > ctx->buffer_size) {
        current_payload_len = ctx->buffer_size - offset; // Handle the tail block residual chunk cleanly
    }

    /* 
     * 3. Serialization and Endian Architecture Normalization
     * Convert properties to Network Byte Order (Big Endian) via htobe64/htonl 
     * to keep serialization robust against CPU byte order issues.
     */
    DataBlockHeader header;
    header.magic = htonl(MAGIC_HEADER);
    header.block_index = htobe64((uint64_t)index);
    header.total_blocks = htobe64((uint64_t)total);
    header.payload_size = htonl((uint32_t)current_payload_len);

    /* 4. Sequential Stream Writing */
    printf("[Info] Serializing Frame Header -> Index: %zu | Payload Chunk: %zu bytes\n", index, current_payload_len);

    // First: Transmit fixed-size serialized meta frame structure over the wire
    if (send_all(client_fd, &header, sizeof(header)) < 0) {
        perror("[Error] Wire connection dropped writing frame metadata header");
        return;
    }

    // Second: Directly dump the raw associated sequential byte payload right after the header
    if (send_all(client_fd, ctx->raw_buffer + offset, current_payload_len) < 0) {
        perror("[Error] Wire connection dropped writing raw payload bytes");
        return;
    }

    printf("[Success] Datablock payload segment index %zu safely transmitted.\n", index);
}
