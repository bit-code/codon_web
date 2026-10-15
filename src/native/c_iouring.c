#define _GNU_SOURCE
#include "c_iouring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/io_uring.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>

#ifndef IORING_SETUP_COOP_TASKRUN
#define IORING_SETUP_COOP_TASKRUN (1U << 8)
#endif

#ifndef IORING_SETUP_SINGLE_ISSUER
#define IORING_SETUP_SINGLE_ISSUER (1U << 12)
#endif

#ifndef IORING_SETUP_DEFER_TASKRUN
#define IORING_SETUP_DEFER_TASKRUN (1U << 13)
#endif

#ifndef IOSQE_BUFFER_SELECT
#define IOSQE_BUFFER_SELECT (1U << 5)
#endif

#ifndef IOSQE_CQE_SKIP_SUCCESS
#define IOSQE_CQE_SKIP_SUCCESS (1U << 6)
#endif

#ifndef IORING_OP_RECV_MULTISHOT
#define IORING_OP_RECV_MULTISHOT 47
#endif

#ifndef IORING_ENTER_EXT_ARG
#define IORING_ENTER_EXT_ARG (1U << 3)
#endif

#ifndef IORING_FEAT_EXT_ARG
#define IORING_FEAT_EXT_ARG (1U << 8)
#endif

struct __kernel_timespec_local {
    int64_t tv_sec;
    int64_t tv_nsec;
};

struct io_uring_getevents_arg_local {
    uint64_t sigmask;
    uint32_t sigmask_sz;
    uint32_t pad;
    uint64_t ts;
};

struct c_iouring {
    int ring_fd;
    unsigned sq_entries;
    unsigned cq_entries;
    
    // SQ Ring mapping
    void *sq_ring_ptr;
    size_t sq_ring_sz;
    uint32_t *sq_khead;
    uint32_t *sq_ktail;
    uint32_t *sq_kring_mask;
    uint32_t *sq_array;
    struct io_uring_sqe *sqes;
    size_t sqes_sz;
    
    // CQ Ring mapping
    void *cq_ring_ptr;
    size_t cq_ring_sz;
    uint32_t *cq_khead;
    uint32_t *cq_ktail;
    uint32_t *cq_kring_mask;
    struct io_uring_cqe *cqes;
    
    // Submission tracking
    uint32_t sq_tail;
    uint32_t features;
    uint32_t setup_flags;
    
    // Timeout structure
    struct __kernel_timespec_local timeout_ts;
};

int c_iouring_is_supported(void) {
#ifdef __NR_io_uring_setup
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    int fd = (int)syscall(__NR_io_uring_setup, 2, &p);
    if (fd >= 0) {
        close(fd);
        return 1;
    }
    return 0;
#else
    return 0;
#endif
}

c_iouring_t *c_iouring_create(uint32_t entries) {
#ifdef __NR_io_uring_setup
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));
    
    int ring_fd = -1;
    uint32_t active_flags = 0;

    // Tier 1 Probe: Try modern Linux 6.1+ Single-Issuer + Defer-Taskrun + Coop-Taskrun
    p.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_COOP_TASKRUN | IORING_SETUP_DEFER_TASKRUN;
    ring_fd = (int)syscall(__NR_io_uring_setup, entries, &p);
    if (ring_fd >= 0) {
        active_flags = p.flags;
    } else {
        // Tier 2 Probe: Try Linux 5.19+ Single-Issuer + Coop-Taskrun
        memset(&p, 0, sizeof(p));
        p.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_COOP_TASKRUN;
        ring_fd = (int)syscall(__NR_io_uring_setup, entries, &p);
        if (ring_fd >= 0) {
            active_flags = p.flags;
        } else {
            // Tier 3 Probe: Fallback to baseline io_uring setup
            memset(&p, 0, sizeof(p));
            ring_fd = (int)syscall(__NR_io_uring_setup, entries, &p);
            if (ring_fd >= 0) {
                active_flags = p.flags;
            } else {
                return NULL;
            }
        }
    }
    
    c_iouring_t *ring = (c_iouring_t *)calloc(1, sizeof(c_iouring_t));
    if (!ring) {
        close(ring_fd);
        return NULL;
    }
    
    ring->ring_fd = ring_fd;
    ring->features = p.features;
    ring->setup_flags = active_flags;
    ring->sq_entries = p.sq_entries;
    ring->cq_entries = p.cq_entries;
    
    // 1. Map SQ Ring
    ring->sq_ring_sz = p.sq_off.array + p.sq_entries * sizeof(uint32_t);
    ring->sq_ring_ptr = mmap(0, ring->sq_ring_sz, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_SQ_RING);
    if (ring->sq_ring_ptr == MAP_FAILED) {
        close(ring_fd);
        free(ring);
        return NULL;
    }
    
    // 2. Map SQEs Array
    ring->sqes_sz = p.sq_entries * sizeof(struct io_uring_sqe);
    ring->sqes = (struct io_uring_sqe *)mmap(0, ring->sqes_sz, PROT_READ | PROT_WRITE,
                                            MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_SQES);
    if (ring->sqes == MAP_FAILED) {
        munmap(ring->sq_ring_ptr, ring->sq_ring_sz);
        close(ring_fd);
        free(ring);
        return NULL;
    }
    
    // 3. Map CQ Ring
    ring->cq_ring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);
    ring->cq_ring_ptr = mmap(0, ring->cq_ring_sz, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_CQ_RING);
    if (ring->cq_ring_ptr == MAP_FAILED) {
        munmap(ring->sqes, ring->sqes_sz);
        munmap(ring->sq_ring_ptr, ring->sq_ring_sz);
        close(ring_fd);
        free(ring);
        return NULL;
    }
    
    // Bind SQ Ring pointer offsets
    ring->sq_khead = (uint32_t *)((char *)ring->sq_ring_ptr + p.sq_off.head);
    ring->sq_ktail = (uint32_t *)((char *)ring->sq_ring_ptr + p.sq_off.tail);
    ring->sq_kring_mask = (uint32_t *)((char *)ring->sq_ring_ptr + p.sq_off.ring_mask);
    ring->sq_array = (uint32_t *)((char *)ring->sq_ring_ptr + p.sq_off.array);
    
    // Bind CQ Ring pointer offsets
    ring->cq_khead = (uint32_t *)((char *)ring->cq_ring_ptr + p.cq_off.head);
    ring->cq_ktail = (uint32_t *)((char *)ring->cq_ring_ptr + p.cq_off.tail);
    ring->cq_kring_mask = (uint32_t *)((char *)ring->cq_ring_ptr + p.cq_off.ring_mask);
    ring->cqes = (struct io_uring_cqe *)((char *)ring->cq_ring_ptr + p.cq_off.cqes);
    
    ring->sq_tail = *ring->sq_ktail;
    return ring;
#else
    return NULL;
#endif
}

void c_iouring_destroy(c_iouring_t *ring) {
    if (!ring) return;
    if (ring->cq_ring_ptr && ring->cq_ring_ptr != MAP_FAILED) {
        munmap(ring->cq_ring_ptr, ring->cq_ring_sz);
    }
    if (ring->sqes && ring->sqes != MAP_FAILED) {
        munmap(ring->sqes, ring->sqes_sz);
    }
    if (ring->sq_ring_ptr && ring->sq_ring_ptr != MAP_FAILED) {
        munmap(ring->sq_ring_ptr, ring->sq_ring_sz);
    }
    if (ring->ring_fd >= 0) {
        close(ring->ring_fd);
    }
    free(ring);
}

int c_iouring_get_fd(const c_iouring_t *ring) {
    return ring ? ring->ring_fd : -1;
}

uint32_t c_iouring_pending_submits(const c_iouring_t *ring) {
    if (!ring) return 0;
    uint32_t head = __atomic_load_n(ring->sq_khead, __ATOMIC_ACQUIRE);
    return (ring->sq_tail >= head) ? (ring->sq_tail - head) : 0;
}

int c_iouring_is_defer_taskrun(const c_iouring_t *ring) {
    if (!ring) return 0;
    return (ring->setup_flags & IORING_SETUP_DEFER_TASKRUN) ? 1 : 0;
}

// Low-overhead SQE acquisition with acquire semantics (0 mfence)
static inline struct io_uring_sqe *c_iouring_get_sqe(c_iouring_t *ring) {
    uint32_t head = __atomic_load_n(ring->sq_khead, __ATOMIC_ACQUIRE);
    uint32_t tail = ring->sq_tail;
    if (tail - head >= ring->sq_entries) {
        return NULL; // SQ is full
    }
    uint32_t index = tail & *ring->sq_kring_mask;
    struct io_uring_sqe *sqe = &ring->sqes[index];
    memset(sqe, 0, sizeof(*sqe));
    ring->sq_array[index] = index;
    ring->sq_tail = tail + 1;
    return sqe;
}

int c_iouring_prep_accept(c_iouring_t *ring, int fd, struct sockaddr *addr, socklen_t *addrlen, int flags, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_ACCEPT;
    sqe->fd = fd;
    sqe->addr = (uint64_t)addr;
    sqe->addr2 = (uint64_t)addrlen;
    sqe->accept_flags = flags;
    sqe->user_data = user_data;
    return 0;
}

int c_iouring_prep_recv(c_iouring_t *ring, int fd, void *buf, unsigned len, int flags, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_RECV;
    sqe->fd = fd;
    sqe->addr = (uint64_t)buf;
    sqe->len = len;
    sqe->msg_flags = flags;
    sqe->user_data = user_data;
    return 0;
}

int c_iouring_prep_send(c_iouring_t *ring, int fd, const void *buf, unsigned len, int flags, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_SEND;
    sqe->fd = fd;
    sqe->addr = (uint64_t)buf;
    sqe->len = len;
    sqe->msg_flags = flags;
    sqe->user_data = user_data;
    return 0;
}

int c_iouring_prep_send_fast(c_iouring_t *ring, int fd, const void *buf, unsigned len, int msg_flags, uint8_t sqe_flags, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_SEND;
    sqe->flags = sqe_flags;
    sqe->fd = fd;
    sqe->addr = (uint64_t)buf;
    sqe->len = len;
    sqe->msg_flags = msg_flags;
    sqe->user_data = user_data;
    return 0;
}

int c_iouring_prep_poll_add(c_iouring_t *ring, int fd, uint32_t poll_mask, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_POLL_ADD;
    sqe->fd = fd;
    sqe->poll32_events = poll_mask;
    sqe->user_data = user_data;
    return 0;
}

int c_iouring_prep_cancel(c_iouring_t *ring, uint64_t user_data) {
    if (!ring) return -1;
    struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
    if (!sqe) return -1;
    sqe->opcode = IORING_OP_ASYNC_CANCEL;
    sqe->addr = user_data;
    sqe->user_data = 0;
    return 0;
}

int c_iouring_enter(c_iouring_t *ring, unsigned min_complete, int timeout_ms) {
    if (!ring) return -1;
    
    // Check if completions already exist in CQ without kernel trip
    uint32_t cq_head = __atomic_load_n(ring->cq_khead, __ATOMIC_ACQUIRE);
    uint32_t cq_tail = __atomic_load_n(ring->cq_ktail, __ATOMIC_ACQUIRE);
    if (cq_head != cq_tail) {
        min_complete = 0;
    }
    
    // Flush sq_tail to kernel using release store semantics
    __atomic_store_n(ring->sq_ktail, ring->sq_tail, __ATOMIC_RELEASE);
    
    uint32_t head = __atomic_load_n(ring->sq_khead, __ATOMIC_ACQUIRE);
    unsigned to_submit = (ring->sq_tail >= head) ? (ring->sq_tail - head) : 0;
    
    // Fast path: nothing to submit and no need to wait -> 0 syscalls!
    if (to_submit == 0 && min_complete == 0) {
        return 0;
    }
    
    unsigned flags = min_complete > 0 ? IORING_ENTER_GETEVENTS : 0;
    
    if (min_complete > 0 && timeout_ms > 0) {
        ring->timeout_ts.tv_sec = timeout_ms / 1000;
        ring->timeout_ts.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
        
        if (ring->features & IORING_FEAT_EXT_ARG) {
            struct io_uring_getevents_arg_local arg;
            memset(&arg, 0, sizeof(arg));
            arg.ts = (uint64_t)&ring->timeout_ts;
            flags |= IORING_ENTER_EXT_ARG;
            return (int)syscall(__NR_io_uring_enter, ring->ring_fd, to_submit, min_complete, flags, &arg, sizeof(arg));
        } else {
            struct io_uring_sqe *sqe = c_iouring_get_sqe(ring);
            if (sqe) {
                sqe->opcode = IORING_OP_TIMEOUT;
                sqe->addr = (uint64_t)&ring->timeout_ts;
                sqe->len = 1;
                sqe->off = 0;
                sqe->user_data = UINT64_MAX; // Internal timeout marker
                __atomic_store_n(ring->sq_ktail, ring->sq_tail, __ATOMIC_RELEASE);
                to_submit++;
            }
        }
    }
    
    return (int)syscall(__NR_io_uring_enter, ring->ring_fd, to_submit, min_complete, flags, NULL, 0);
}

int c_iouring_reap_cqes(c_iouring_t *ring, c_iouring_cqe_t *out_cqes, unsigned max_cqes) {
    if (!ring || !out_cqes || max_cqes == 0) return 0;
    
    uint32_t head = *ring->cq_khead;
    uint32_t tail = __atomic_load_n(ring->cq_ktail, __ATOMIC_ACQUIRE);
    
    unsigned count = 0;
    while (head != tail && count < max_cqes) {
        uint32_t index = head & *ring->cq_kring_mask;
        struct io_uring_cqe *cqe = &ring->cqes[index];
        head++;
        
        // Internal timeout marker: consume silently
        if (cqe->user_data == UINT64_MAX) {
            continue;
        }
        
        out_cqes[count].user_data = cqe->user_data;
        out_cqes[count].res = cqe->res;
        out_cqes[count].flags = cqe->flags;
        count++;
    }
    
    if (head != *ring->cq_khead) {
        __atomic_store_n(ring->cq_khead, head, __ATOMIC_RELEASE);
    }
    return (int)count;
}
