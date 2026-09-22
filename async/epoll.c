
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "protocol.h"


/*
Asynchronous Event-Driven Sender State Machine (epoll)This code implements non-blocking writes. 
It saves partial execution states inside a session structure so epoll can cleanly resume processing when the socket is ready to write again (EPOLLOUT).
*/


typedef enum {
    STATE_SENDING_HEADER,
    STATE_SENDING_PAYLOAD,
    STATE_FINISHED
} SendState;

/* Persistent connection session tracker for non-blocking execution memory */
typedef struct {
    int fd;
    SendState state;
    DataBlockHeader header;
    const uint8_t *payload_ptr;
    size_t header_bytes_sent;
    size_t payload_bytes_sent;
    size_t total_payload_len;
} AsyncSession;

/* Utility function to explicitly flag a socket descriptor as non-blocking */
int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Initialization trigger acting as the onDataBlockObjectReceived handler.
 *        Sets up the session structures and links them with the epoll framework instance.
 */
AsyncSession* onDataBlockObjectReceivedAsync(int epoll_fd, int client_fd, size_t index, size_t total, const uint8_t *payload, size_t payload_len) {
    if (make_socket_non_blocking(client_fd) < 0) {
        perror("Failed setting non-blocking state configuration");
        return NULL;
    }

    AsyncSession *session = (AsyncSession *)calloc(1, sizeof(AsyncSession));
    session->fd = client_fd;
    session->state = STATE_SENDING_HEADER;
    
    // Normalize data layout directly to Big-Endian Network standard formatting alignments
    session->header.magic = htonl(MAGIC_HEADER);
    session->header.block_index = htobe64((uint64_t)index);
    session->header.total_blocks = htobe64((uint64_t)total);
    session->header.payload_size = htonl((uint32_t)payload_len);
    
    session->payload_ptr = payload;
    session->total_payload_len = payload_len;

    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET | EPOLLRDHUP; // Edge-triggered output polling
    ev.data.ptr = session;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
        perror("epoll_ctl instantiation phase failed mapping data paths");
        free(session);
        return NULL;
    }
    return session;
}

/**
 * @brief The execution core of the asynchronous writing cycle.
 *        Called whenever epoll flags an EPOLLOUT event for the session.
 */
int handle_async_write_event(int epoll_fd, AsyncSession *session) {
    while (1) {
        if (session->state == STATE_SENDING_HEADER) {
            uint8_t *raw_header = (uint8_t *)&session->header;
            size_t remaining = sizeof(DataBlockHeader) - session->header_bytes_sent;
            
            ssize_t sent = send(session->fd, raw_header + session->header_bytes_sent, remaining, 0);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; // Kernel buffer saturated, yield loop
                return -1; // Catastrophic socket connection breakdown error
            }
            session->header_bytes_sent += sent;
            if (session->header_bytes_sent == sizeof(DataBlockHeader)) {
                session->state = STATE_SENDING_PAYLOAD;
            }
        }

        if (session->state == STATE_SENDING_PAYLOAD) {
            size_t remaining = session->total_payload_len - session->payload_bytes_sent;
            if (remaining == 0) {
                session->state = STATE_FINISHED;
                break;
            }

            ssize_t sent = send(session->fd, session->payload_ptr + session->payload_bytes_sent, remaining, 0);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; 
                return -1;
            }
            session->payload_bytes_sent += sent;
            if (session->payload_bytes_sent == session->total_payload_len) {
                session->state = STATE_FINISHED;
                break;
            }
        }
    }

    if (session->state == STATE_FINISHED) {
        printf("[Async Success] Transmitted index context blocks completely.\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, session->fd, NULL);
        close(session->fd);
        free(session);
    }
    return 1;
}
