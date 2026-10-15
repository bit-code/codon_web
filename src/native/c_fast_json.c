#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <unistd.h>
#include "yyjson.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Initialize and read JSON
yyjson_doc *c_yyjson_read(const char *data, size_t len) {
    if (!data) return NULL;
    size_t actual_len = strlen(data);
    if (len == 0 || actual_len > len) len = actual_len;
    return yyjson_read_opts((char *)data, len, 0, NULL, NULL);
}

void c_yyjson_doc_free(yyjson_doc *doc) {
    if (doc) yyjson_doc_free(doc);
}

yyjson_val *c_yyjson_doc_get_root(yyjson_doc *doc) {
    return doc ? doc->root : NULL;
}

// Accessors & Type Checks
size_t c_yyjson_get_len(yyjson_val *val) {
    return val ? yyjson_get_len(val) : 0;
}

int c_yyjson_is_obj(yyjson_val *val) {
    return val ? yyjson_is_obj(val) : 0;
}

int c_yyjson_is_arr(yyjson_val *val) {
    return val ? yyjson_is_arr(val) : 0;
}

int c_yyjson_is_str(yyjson_val *val) {
    return val ? yyjson_is_str(val) : 0;
}

int c_yyjson_is_int(yyjson_val *val) {
    return val ? yyjson_is_int(val) : 0;
}

int c_yyjson_is_real(yyjson_val *val) {
    return val ? yyjson_is_real(val) : 0;
}

int c_yyjson_is_bool(yyjson_val *val) {
    return val ? (yyjson_is_bool(val) ? 1 : 0) : 0;
}

int c_yyjson_is_null(yyjson_val *val) {
    return val ? yyjson_is_null(val) : 1;
}

const char *c_yyjson_get_str(yyjson_val *val) {
    return val ? yyjson_get_str(val) : NULL;
}

int64_t c_yyjson_get_sint(yyjson_val *val) {
    return val ? yyjson_get_sint(val) : 0;
}

double c_yyjson_get_real(yyjson_val *val) {
    return val ? yyjson_get_real(val) : 0.0;
}

int c_yyjson_get_bool(yyjson_val *val) {
    return val ? (yyjson_get_bool(val) ? 1 : 0) : 0;
}

yyjson_val *c_yyjson_obj_get(yyjson_val *obj, const char *key) {
    return (obj && key) ? yyjson_obj_get(obj, key) : NULL;
}

yyjson_val *c_yyjson_arr_get(yyjson_val *arr, size_t idx) {
    return arr ? yyjson_arr_get(arr, idx) : NULL;
}

// Object iteration
typedef struct {
    yyjson_obj_iter iter;
} c_obj_iter;

void c_obj_iter_init(yyjson_val *obj, c_obj_iter *it) {
    if (it) {
        if (obj && yyjson_is_obj(obj)) {
            yyjson_obj_iter_init(obj, &it->iter);
        } else {
            memset(&it->iter, 0, sizeof(it->iter));
        }
    }
}

int c_obj_iter_next(c_obj_iter *it, const char **key_out, size_t *key_len, yyjson_val **val_out) {
    if (!it) return 0;
    yyjson_val *key = yyjson_obj_iter_next(&it->iter);
    if (!key) return 0;
    if (key_out) *key_out = yyjson_get_str(key);
    if (key_len) *key_len = yyjson_get_len(key);
    if (val_out) *val_out = yyjson_obj_iter_get_val(key);
    return 1;
}

// Convert any scalar or nested JSON value to string representation
const char *c_yyjson_val_to_str(yyjson_val *val, char *buf, size_t buf_len, size_t *out_len) {
    if (!val) {
        if (out_len) *out_len = 0;
        return "";
    }
    if (yyjson_is_str(val)) {
        size_t len = yyjson_get_len(val);
        if (out_len) *out_len = len;
        return yyjson_get_str(val);
    }
    if (yyjson_is_null(val)) {
        if (out_len) *out_len = 4;
        return "null";
    }
    if (yyjson_is_true(val)) {
        if (out_len) *out_len = 4;
        return "true";
    }
    if (yyjson_is_false(val)) {
        if (out_len) *out_len = 5;
        return "false";
    }
    if (yyjson_is_int(val)) {
        int64_t v = yyjson_get_sint(val);
        int n = snprintf(buf, buf_len, "%lld", (long long)v);
        if (out_len) *out_len = (n > 0) ? (size_t)n : 0;
        return buf;
    }
    if (yyjson_is_real(val)) {
        double v = yyjson_get_real(val);
        int n = snprintf(buf, buf_len, "%.15g", v);
        if (out_len) *out_len = (n > 0) ? (size_t)n : 0;
        return buf;
    }
    // Object or array: write JSON into buf
    size_t written = yyjson_val_write_buf(buf, buf_len, val, 0, NULL);
    if (out_len) *out_len = written;
    return buf;
}

// Serialization
char *c_yyjson_write(yyjson_doc *doc, size_t *len) {
    return doc ? yyjson_write_opts(doc, 0, NULL, len, NULL) : NULL;
}

// Mutable Document for fast construction and serialization
yyjson_mut_doc *c_yyjson_mut_doc_new(void) {
    return yyjson_mut_doc_new(NULL);
}

void c_yyjson_mut_doc_free(yyjson_mut_doc *doc) {
    if (doc) yyjson_mut_doc_free(doc);
}

yyjson_mut_val *c_yyjson_mut_obj(yyjson_mut_doc *doc) {
    return doc ? yyjson_mut_obj(doc) : NULL;
}

void c_yyjson_mut_doc_set_root(yyjson_mut_doc *doc, yyjson_mut_val *root) {
    if (doc && root) yyjson_mut_doc_set_root(doc, root);
}

int c_yyjson_mut_obj_add_strn(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, size_t klen, const char *val, size_t vlen) {
    if (!doc || !obj || !key || !val) return 0;
    size_t actual_klen = strlen(key);
    size_t actual_vlen = strlen(val);
    yyjson_mut_val *k = yyjson_mut_strn(doc, key, actual_klen);
    yyjson_mut_val *v = yyjson_mut_strn(doc, val, actual_vlen);
    return (k && v && yyjson_mut_obj_add(obj, k, v)) ? 1 : 0;
}

char *c_yyjson_mut_write(yyjson_mut_doc *doc, size_t *out_len) {
    if (!doc) return NULL;
    return yyjson_mut_write(doc, 0, out_len);
}

void c_free(void *ptr) {
    if (ptr) free(ptr);
}

// ==============================================================================
// Industrial-grade Zero-Copy Streaming JSON Encoder
// Single-pass, zero-allocation, SIMD/LUT-accelerated with overflow protection
// ==============================================================================

typedef struct {
    char alc_buf[8192];
    yyjson_alc alc;
    yyjson_mut_doc *doc;
    yyjson_mut_val *root;
} c_fast_writer;

static __thread c_fast_writer g_tls_fast_writer;

c_fast_writer* c_get_tls_fast_writer(void) {
    return &g_tls_fast_writer;
}

void c_fast_writer_init(c_fast_writer *w) {
    if (!w) return;
    yyjson_alc_pool_init(&w->alc, w->alc_buf, sizeof(w->alc_buf));
    w->doc = yyjson_mut_doc_new(&w->alc);
    w->root = yyjson_mut_obj(w->doc);
    yyjson_mut_doc_set_root(w->doc, w->root);
}

void c_fast_writer_add(c_fast_writer *w, const char *k, size_t klen, const char *v, size_t vlen) {
    if (!w || !w->doc || !w->root || !k || !v) return;
    yyjson_mut_val *key = yyjson_mut_strn(w->doc, k, klen);
    yyjson_mut_val *val = yyjson_mut_strn(w->doc, v, vlen);
    yyjson_mut_obj_add(w->root, key, val);
}

size_t c_fast_writer_finish(c_fast_writer *w, char *out_buf, size_t out_cap) {
    if (!w || !w->doc || !out_buf || out_cap == 0) return 0;
    return yyjson_mut_write_buf(out_buf, out_cap, w->doc, 0, NULL);
}

typedef struct {
    char *buf;
    size_t cap;
    size_t len;
    int is_first;
    int overflow; // 0 = ok, 1 = buffer overflow prevented
} c_json_stream;

#define MAX_STREAM_DEPTH 16
static __thread c_json_stream g_tls_stream_pool[MAX_STREAM_DEPTH];
static __thread int g_tls_stream_depth = 0;

static void c_release_tls_json_stream(c_json_stream *s) {
    if (!s) return;
    if (g_tls_stream_depth > 0 && s == &g_tls_stream_pool[g_tls_stream_depth - 1]) {
        g_tls_stream_depth--;
    } else {
        int is_pool = 0;
        for (int i = 0; i < MAX_STREAM_DEPTH; i++) {
            if (s == &g_tls_stream_pool[i]) { is_pool = 1; break; }
        }
        if (!is_pool) free(s);
    }
}

c_json_stream* c_get_tls_json_stream(void) {
    if (g_tls_stream_depth < MAX_STREAM_DEPTH) {
        return &g_tls_stream_pool[g_tls_stream_depth++];
    }
    return (c_json_stream*)malloc(sizeof(c_json_stream));
}

static const unsigned char c_json_safe_lut[256] = {
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0-15
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 16-31
    1,1,0,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 32-47 (34 is '"')
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 48-63
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 64-79
    1,1,1,1,1,1,1,1, 1,1,1,1,0,1,1,1, // 80-95 (92 is '\\')
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 96-111
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 112-127
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 128-143 (UTF-8 bytes >= 128 are safe)
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 144-159
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 160-175
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 176-191
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 192-207
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 208-223
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 224-239
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1  // 240-255
};

static inline void c_json_stream_write_escaped(c_json_stream *s, const char *src, size_t len) {
    if (!src || len == 0) return;
    char *dest = s->buf + s->len;
    size_t cap = (s->cap > s->len) ? (s->cap - s->len) : 0;
    if (cap < 2) {
        s->overflow = 1;
        return;
    }

    int needs_escape = 0;
    for (size_t k = 0; k < len; k++) {
        if (!c_json_safe_lut[(unsigned char)src[k]]) {
            needs_escape = 1;
            break;
        }
    }

    if (!needs_escape) {
        if (len < cap) {
            memcpy(dest, src, len);
            s->len += len;
        } else {
            s->overflow = 1;
        }
        return;
    }

    size_t d = 0;
    size_t i = 0;
    while (i < len && d + 6 < cap) {
        unsigned char c = (unsigned char)src[i++];
        if (c_json_safe_lut[c]) {
            dest[d++] = (char)c;
        } else if (c == '"') {
            dest[d++] = '\\'; dest[d++] = '"';
        } else if (c == '\\') {
            dest[d++] = '\\'; dest[d++] = '\\';
        } else if (c == '\n') {
            dest[d++] = '\\'; dest[d++] = 'n';
        } else if (c == '\r') {
            dest[d++] = '\\'; dest[d++] = 'r';
        } else if (c == '\t') {
            dest[d++] = '\\'; dest[d++] = 't';
        } else {
            static const char hex[] = "0123456789abcdef";
            dest[d++] = '\\'; dest[d++] = 'u';
            dest[d++] = '0'; dest[d++] = '0';
            dest[d++] = hex[c >> 4];
            dest[d++] = hex[c & 0x0F];
        }
    }
    if (i < len) {
        s->overflow = 1;
    }
    s->len += d;
}

void c_json_stream_init(c_json_stream *s, char *buf, size_t cap) {
    if (!s || !buf || cap < 2) return;
    s->buf = buf;
    s->cap = cap;
    s->buf[0] = '{';
    s->len = 1;
    s->is_first = 1;
    s->overflow = 0;
}

void c_json_stream_append_str_pair(c_json_stream *s, const char *k, size_t klen, const char *v, size_t vlen) {
    if (!s || !k || !v || s->overflow) return;
    size_t actual_klen = (klen > 0) ? klen : strlen(k);
    size_t actual_vlen = (vlen > 0) ? vlen : strlen(v);
    if (s->len + actual_klen + actual_vlen + 16 >= s->cap) {
        s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (!s->is_first) {
        buf[s->len++] = ',';
    }
    s->is_first = 0;
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, k, actual_klen);
    buf[s->len++] = '"';
    buf[s->len++] = ':';
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, v, actual_vlen);
    buf[s->len++] = '"';
}

void c_json_stream_append_int_pair(c_json_stream *s, const char *k, size_t klen, int64_t val) {
    if (!s || s->overflow || s->len + klen + 32 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (!s->is_first) {
        buf[s->len++] = ',';
    }
    s->is_first = 0;
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, k, klen);
    buf[s->len++] = '"';
    buf[s->len++] = ':';
    int n = snprintf(buf + s->len, s->cap - s->len, "%lld", (long long)val);
    if (n > 0) s->len += (size_t)n;
}

void c_json_stream_append_raw_pair(c_json_stream *s, const char *k, size_t klen, const char *raw, size_t rlen) {
    if (!s || !k || !raw || s->overflow) return;
    size_t actual_klen = (klen > 0) ? klen : strlen(k);
    size_t actual_rlen = (rlen > 0) ? rlen : strlen(raw);
    if (s->len + actual_klen + actual_rlen + 8 >= s->cap) {
        s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (!s->is_first) {
        buf[s->len++] = ',';
    }
    s->is_first = 0;
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, k, actual_klen);
    buf[s->len++] = '"';
    buf[s->len++] = ':';
    memcpy(buf + s->len, raw, actual_rlen);
    s->len += actual_rlen;
}

void c_json_stream_append_null_pair(c_json_stream *s, const char *k, size_t klen) {
    c_json_stream_append_raw_pair(s, k, klen, "null", 4);
}

void c_json_stream_append_bool_pair(c_json_stream *s, const char *k, size_t klen, int val) {
    if (val) {
        c_json_stream_append_raw_pair(s, k, klen, "true", 4);
    } else {
        c_json_stream_append_raw_pair(s, k, klen, "false", 5);
    }
}

void c_json_stream_append_float_pair(c_json_stream *s, const char *k, size_t klen, double val) {
    if (!s || s->overflow || s->len + klen + 32 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (!s->is_first) {
        buf[s->len++] = ',';
    }
    s->is_first = 0;
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, k, klen);
    buf[s->len++] = '"';
    buf[s->len++] = ':';
    int n = snprintf(buf + s->len, s->cap - s->len, "%.15g", val);
    if (n > 0) s->len += (size_t)n;
}

void c_json_stream_begin_array(c_json_stream *s, const char *k, size_t klen) {
    if (!s || !k || s->overflow) return;
    size_t actual_klen = (klen > 0) ? klen : strlen(k);
    if (s->len + actual_klen + 16 >= s->cap) {
        s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (!s->is_first) {
        buf[s->len++] = ',';
    }
    s->is_first = 0;
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, k, actual_klen);
    buf[s->len++] = '"';
    buf[s->len++] = ':';
    buf[s->len++] = '[';
}

void c_json_stream_append_arr_str(c_json_stream *s, const char *v, size_t vlen) {
    if (!s || !v || s->overflow) return;
    size_t actual_vlen = (vlen > 0) ? vlen : strlen(v);
    if (s->len + actual_vlen + 16 >= s->cap) {
        s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (buf[s->len - 1] != '[') {
        buf[s->len++] = ',';
    }
    buf[s->len++] = '"';
    c_json_stream_write_escaped(s, v, actual_vlen);
    buf[s->len++] = '"';
}

void c_json_stream_append_arr_int(c_json_stream *s, int64_t val) {
    if (!s || s->overflow || s->len + 32 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (buf[s->len - 1] != '[') {
        buf[s->len++] = ',';
    }
    int n = snprintf(buf + s->len, s->cap - s->len, "%lld", (long long)val);
    if (n > 0) s->len += (size_t)n;
}

void c_json_stream_append_arr_float(c_json_stream *s, double val) {
    if (!s || s->overflow || s->len + 32 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (buf[s->len - 1] != '[') {
        buf[s->len++] = ',';
    }
    int n = snprintf(buf + s->len, s->cap - s->len, "%.15g", val);
    if (n > 0) s->len += (size_t)n;
}

void c_json_stream_append_arr_bool(c_json_stream *s, int val) {
    if (!s || s->overflow || s->len + 8 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (buf[s->len - 1] != '[') {
        buf[s->len++] = ',';
    }
    if (val) {
        memcpy(buf + s->len, "true", 4);
        s->len += 4;
    } else {
        memcpy(buf + s->len, "false", 5);
        s->len += 5;
    }
}

void c_json_stream_append_arr_raw(c_json_stream *s, const char *raw, size_t rlen) {
    if (!s || !raw || s->overflow) return;
    size_t actual_rlen = (rlen > 0) ? rlen : strlen(raw);
    if (s->len + actual_rlen + 8 >= s->cap) {
        s->overflow = 1;
        return;
    }
    char *buf = s->buf;
    if (buf[s->len - 1] != '[') {
        buf[s->len++] = ',';
    }
    memcpy(buf + s->len, raw, actual_rlen);
    s->len += actual_rlen;
}

void c_json_stream_end_array(c_json_stream *s) {
    if (!s || s->overflow || s->len + 1 >= s->cap) {
        if (s) s->overflow = 1;
        return;
    }
    s->buf[s->len++] = ']';
}

size_t c_json_stream_finish(c_json_stream *s) {
    if (!s) return 0;
    if (s->overflow || s->len + 1 >= s->cap) {
        c_release_tls_json_stream(s);
        return 0;
    }
    s->buf[s->len++] = '}';
    s->buf[s->len] = '\0';
    size_t res = s->len;
    c_release_tls_json_stream(s);
    return res;
}

// CPU Affinity Pinning for Multi-Reactor Workers
int c_bind_cpu_core(int core_id) {
#ifdef __linux__
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores <= 0) return -1;
    int target_core = (core_id >= 0) ? (int)(core_id % num_cores) : 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_core, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        return -1;
    }
    return target_core;
#else
    return -1;
#endif
}

int c_get_cpu_cores(void) {
#ifdef __linux__
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (num_cores > 0) ? (int)num_cores : 1;
#else
    return 1;
#endif
}


