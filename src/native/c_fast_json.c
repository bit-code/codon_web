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
    return val ? (yyjson_get_bool(val) ? 1 : 0) : 0;
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

