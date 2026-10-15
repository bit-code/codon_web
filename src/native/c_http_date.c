#include <time.h>
#include <stddef.h>

// Generates RFC 1123 HTTP Date header formatted string into buf, e.g.:
// "Date: Mon, 21 Sep 2026 15:30:00 GMT\r\n"
// Returns the number of bytes written (usually 37 bytes)
int c_get_http_date_rfc1123(char *buf, int max_len) {
    if (!buf || max_len < 40) return 0;
    time_t now = time(NULL);
    struct tm tm_buf;
    if (!gmtime_r(&now, &tm_buf)) return 0;
    return (int)strftime(buf, (size_t)max_len, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &tm_buf);
}
