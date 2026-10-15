#ifndef C_IOURING_H
#define C_IOURING_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t user_data;
    int32_t res;
    uint32_t flags;
} c_iouring_cqe_t;

typedef struct c_iouring c_iouring_t;

// Detection & Lifecycle
int c_iouring_is_supported(void);
c_iouring_t *c_iouring_create(uint32_t entries);
void c_iouring_destroy(c_iouring_t *ring);
int c_iouring_get_fd(const c_iouring_t *ring);

// SQE Preparation
int c_iouring_prep_accept(c_iouring_t *ring, int fd, struct sockaddr *addr, socklen_t *addrlen, int flags, uint64_t user_data);
int c_iouring_prep_recv(c_iouring_t *ring, int fd, void *buf, unsigned len, int flags, uint64_t user_data);
int c_iouring_prep_send(c_iouring_t *ring, int fd, const void *buf, unsigned len, int flags, uint64_t user_data);
int c_iouring_prep_poll_add(c_iouring_t *ring, int fd, uint32_t poll_mask, uint64_t user_data);
int c_iouring_prep_cancel(c_iouring_t *ring, uint64_t user_data);

// Execution & Harvesting
int c_iouring_enter(c_iouring_t *ring, unsigned min_complete, int timeout_ms);
int c_iouring_reap_cqes(c_iouring_t *ring, c_iouring_cqe_t *out_cqes, unsigned max_cqes);
uint32_t c_iouring_pending_submits(const c_iouring_t *ring);
int c_iouring_is_defer_taskrun(const c_iouring_t *ring);
int c_iouring_prep_send_fast(c_iouring_t *ring, int fd, const void *buf, unsigned len, int msg_flags, uint8_t sqe_flags, uint64_t user_data);

#ifdef __cplusplus
}
#endif

#endif // C_IOURING_H
