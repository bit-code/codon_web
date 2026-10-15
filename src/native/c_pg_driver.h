#ifndef C_PG_DRIVER_H
#define C_PG_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* PGConnPtr;
typedef void* PGResultPtr;

// Connection Management
PGConnPtr c_pg_connect(const char* conninfo);
int c_pg_status(PGConnPtr conn); // 0 = OK, 1 = BAD
const char* c_pg_error_message(PGConnPtr conn);
void c_pg_finish(PGConnPtr conn);

// Query Execution with Parameter Binding
PGResultPtr c_pg_exec_params(PGConnPtr conn, const char* sql, int num_params, const char* const* params);
int c_pg_result_status(PGResultPtr res); // 1 = COMMAND_OK, 2 = TUPLES_OK, -1 = ERROR
const char* c_pg_result_error(PGResultPtr res);
void c_pg_clear_result(PGResultPtr res);

// Result Metadata & Access
int c_pg_ntuples(PGResultPtr res);
int c_pg_nfields(PGResultPtr res);
const char* c_pg_fname(PGResultPtr res, int field_idx);
const char* c_pg_getvalue(PGResultPtr res, int tuple_idx, int field_idx);
int c_pg_getisnull(PGResultPtr res, int tuple_idx, int field_idx);
int c_pg_cmd_tuples(PGResultPtr res);
const char* c_pg_result_sqlstate(PGResultPtr res);
int c_pg_ftype(PGResultPtr res, int field_idx);

// Non-blocking & Asynchronous Operations
int c_pg_set_nonblocking(PGConnPtr conn, int arg);
int c_pg_is_nonblocking(PGConnPtr conn);
int c_pg_socket(PGConnPtr conn);
int c_pg_send_query_params(PGConnPtr conn, const char* sql, int num_params, const char* const* params);
int c_pg_flush(PGConnPtr conn);
int c_pg_consume_input(PGConnPtr conn);
int c_pg_is_busy(PGConnPtr conn);
PGResultPtr c_pg_get_result(PGConnPtr conn);

#ifdef __cplusplus
}
#endif

#endif // C_PG_DRIVER_H
