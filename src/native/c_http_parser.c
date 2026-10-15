#include "c_http_parser.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define METHOD_UNKNOWN 0
#define METHOD_GET 1
#define METHOD_POST 2
#define METHOD_PUT 3
#define METHOD_DELETE 4
#define METHOD_HEAD 5
#define METHOD_OPTIONS 6
#define METHOD_PATCH 7

static inline int fast_method_to_int(const char *m, size_t len) {
    if (len == 3) {
        if (m[0] == 'G' && m[1] == 'E' && m[2] == 'T') return METHOD_GET;
        if (m[0] == 'P' && m[1] == 'U' && m[2] == 'T') return METHOD_PUT;
    } else if (len == 4) {
        if (m[0] == 'P' && m[1] == 'O' && m[2] == 'S' && m[3] == 'T') return METHOD_POST;
        if (m[0] == 'H' && m[1] == 'E' && m[2] == 'A' && m[3] == 'D') return METHOD_HEAD;
    } else if (len == 6) {
        if (memcmp(m, "DELETE", 6) == 0) return METHOD_DELETE;
    } else if (len == 5) {
        if (memcmp(m, "PATCH", 5) == 0) return METHOD_PATCH;
    } else if (len == 7) {
        if (memcmp(m, "OPTIONS", 7) == 0) return METHOD_OPTIONS;
    }
    return METHOD_UNKNOWN;
}

c_http_parser_ctx *c_http_parser_new(void) {
    c_http_parser_ctx *ctx = (c_http_parser_ctx *)malloc(sizeof(c_http_parser_ctx));
    if (ctx) memset(ctx, 0, sizeof(c_http_parser_ctx));
    return ctx;
}

void c_http_parser_free(c_http_parser_ctx *ctx) {
    if (ctx) free(ctx);
}

int c_http_parser_execute(c_http_parser_ctx *ctx, const char *buf, int len, int last_len) {
    if (!ctx || !buf || len <= 0) return 2; // PARSE_ERROR

    struct c_http_request_result *out = &ctx->result;
    const char *method = NULL;
    size_t method_len = 0;
    const char *path = NULL;
    size_t path_len = 0;
    int minor_version = 0;
    struct phr_header phr_headers[MAX_PARSED_HEADERS];
    size_t num_headers = MAX_PARSED_HEADERS;

    int res = phr_parse_request(
        buf, (size_t)len,
        &method, &method_len,
        &path, &path_len,
        &minor_version,
        phr_headers, &num_headers,
        (size_t)last_len
    );

    if (res == -2) {
        out->res_code = 1; // PARSE_NEED_MORE
        return 1;
    } else if (res == -1) {
        out->res_code = 2; // PARSE_ERROR
        return 2;
    }

    out->res_code = 0; // PARSE_OK
    out->header_bytes_consumed = res;
    out->method = fast_method_to_int(method, method_len);
    out->minor_version = minor_version;
    out->keep_alive = (minor_version == 1) ? 1 : 0;
    out->content_length = 0;

    // Split target path into path and query
    const char *q = (const char *)memchr(path, '?', path_len);
    if (q != NULL) {
        out->path = path;
        out->path_len = (int)(q - path);
        out->query = q + 1;
        out->query_len = (int)(path_len - (q - path) - 1);
    } else {
        out->path = path;
        out->path_len = (int)path_len;
        out->query = NULL;
        out->query_len = 0;
    }

    int max_copy = (int)num_headers;
    if (max_copy > MAX_PARSED_HEADERS) max_copy = MAX_PARSED_HEADERS;
    out->num_headers = max_copy;

    for (int i = 0; i < max_copy; i++) {
        out->headers[i].name = phr_headers[i].name;
        out->headers[i].name_len = (int64_t)phr_headers[i].name_len;
        out->headers[i].value = phr_headers[i].value;
        out->headers[i].value_len = (int64_t)phr_headers[i].value_len;

        if (phr_headers[i].name_len == 14 && strncasecmp(phr_headers[i].name, "content-length", 14) == 0) {
            int64_t cl = 0;
            const char *vp = phr_headers[i].value;
            size_t vlen = phr_headers[i].value_len;
            for (size_t j = 0; j < vlen; j++) {
                char c = vp[j];
                if (c >= '0' && c <= '9') cl = cl * 10 + (c - '0');
            }
            out->content_length = cl;
        } else if (phr_headers[i].name_len == 10 && strncasecmp(phr_headers[i].name, "connection", 10) == 0) {
            if (phr_headers[i].value_len == 5 && strncasecmp(phr_headers[i].value, "close", 5) == 0) {
                out->keep_alive = 0;
            } else if (phr_headers[i].value_len == 10 && strncasecmp(phr_headers[i].value, "keep-alive", 10) == 0) {
                out->keep_alive = 1;
            }
        }
    }

    return 0;
}

int c_http_res_code(const c_http_parser_ctx *ctx) { return ctx->result.res_code; }
int c_http_method(const c_http_parser_ctx *ctx) { return ctx->result.method; }
const char *c_http_path(const c_http_parser_ctx *ctx) { return ctx->result.path; }
int c_http_path_len(const c_http_parser_ctx *ctx) { return ctx->result.path_len; }
const char *c_http_query(const c_http_parser_ctx *ctx) { return ctx->result.query; }
int c_http_query_len(const c_http_parser_ctx *ctx) { return ctx->result.query_len; }
int c_http_minor_version(const c_http_parser_ctx *ctx) { return ctx->result.minor_version; }
int64_t c_http_content_length(const c_http_parser_ctx *ctx) { return ctx->result.content_length; }
int c_http_keep_alive(const c_http_parser_ctx *ctx) { return ctx->result.keep_alive; }
int c_http_consumed(const c_http_parser_ctx *ctx) { return ctx->result.header_bytes_consumed; }
int c_http_num_headers(const c_http_parser_ctx *ctx) { return ctx->result.num_headers; }

const struct c_parsed_header *c_http_headers_ptr(const c_http_parser_ctx *ctx) {
    return ctx->result.headers;
}

const char *c_http_header_name(const c_http_parser_ctx *ctx, int idx) {
    if (idx < 0 || idx >= ctx->result.num_headers) return NULL;
    return ctx->result.headers[idx].name;
}
int c_http_header_name_len(const c_http_parser_ctx *ctx, int idx) {
    if (idx < 0 || idx >= ctx->result.num_headers) return 0;
    return (int)ctx->result.headers[idx].name_len;
}
const char *c_http_header_value(const c_http_parser_ctx *ctx, int idx) {
    if (idx < 0 || idx >= ctx->result.num_headers) return NULL;
    return ctx->result.headers[idx].value;
}
int c_http_header_value_len(const c_http_parser_ctx *ctx, int idx) {
    if (idx < 0 || idx >= ctx->result.num_headers) return 0;
    return (int)ctx->result.headers[idx].value_len;
}
