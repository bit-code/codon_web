#ifndef C_HTTP_PARSER_H
#define C_HTTP_PARSER_H

#include "picohttpparser.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_PARSED_HEADERS 64

struct c_parsed_header {
    const char *name;
    int64_t name_len;
    const char *value;
    int64_t value_len;
};

struct c_http_request_result {
    int res_code; // 0 = OK, 1 = NEED_MORE, 2 = ERROR
    int method;   // HttpMethod enum
    const char *path;
    int path_len;
    const char *query;
    int query_len;
    int minor_version;
    int64_t content_length;
    int keep_alive;
    int header_bytes_consumed;
    int num_headers;
    struct c_parsed_header headers[MAX_PARSED_HEADERS];
};

typedef struct c_http_parser_ctx {
    struct c_http_request_result result;
} c_http_parser_ctx;

c_http_parser_ctx *c_http_parser_new(void);
void c_http_parser_free(c_http_parser_ctx *ctx);

int c_http_parser_execute(c_http_parser_ctx *ctx, const char *buf, int len, int last_len);

int c_http_res_code(const c_http_parser_ctx *ctx);
int c_http_method(const c_http_parser_ctx *ctx);
const char *c_http_path(const c_http_parser_ctx *ctx);
int c_http_path_len(const c_http_parser_ctx *ctx);
const char *c_http_query(const c_http_parser_ctx *ctx);
int c_http_query_len(const c_http_parser_ctx *ctx);
int c_http_minor_version(const c_http_parser_ctx *ctx);
int64_t c_http_content_length(const c_http_parser_ctx *ctx);
int c_http_keep_alive(const c_http_parser_ctx *ctx);
int c_http_consumed(const c_http_parser_ctx *ctx);
int c_http_num_headers(const c_http_parser_ctx *ctx);

const struct c_parsed_header *c_http_headers_ptr(const c_http_parser_ctx *ctx);
const char *c_http_header_name(const c_http_parser_ctx *ctx, int idx);
int c_http_header_name_len(const c_http_parser_ctx *ctx, int idx);
const char *c_http_header_value(const c_http_parser_ctx *ctx, int idx);
int c_http_header_value_len(const c_http_parser_ctx *ctx, int idx);

#endif
