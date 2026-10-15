#include "c_pg_driver.h"
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <string.h>

PGConnPtr c_pg_connect(const char* conninfo) {
    if (!conninfo) return NULL;
    PGconn *conn = PQconnectdb(conninfo);
    return (PGConnPtr)conn;
}

int c_pg_status(PGConnPtr conn) {
    if (!conn) return 1;
    return (PQstatus((PGconn*)conn) == CONNECTION_OK) ? 0 : 1;
}

const char* c_pg_error_message(PGConnPtr conn) {
    if (!conn) return "NULL PostgreSQL connection";
    return PQerrorMessage((PGconn*)conn);
}

void c_pg_finish(PGConnPtr conn) {
    if (conn) {
        PQfinish((PGconn*)conn);
    }
}

PGResultPtr c_pg_exec_params(PGConnPtr conn, const char* sql, int num_params, const char* const* params) {
    if (!conn || !sql) return NULL;
    PGresult *res = PQexecParams(
        (PGconn*)conn,
        sql,
        num_params,
        NULL, // Infer param types
        params,
        NULL, // All text formats
        NULL, // Text formats
        0     // Return text format
    );
    return (PGResultPtr)res;
}

int c_pg_result_status(PGResultPtr res) {
    if (!res) return -1;
    ExecStatusType st = PQresultStatus((PGresult*)res);
    if (st == PGRES_COMMAND_OK) return 1;
    if (st == PGRES_TUPLES_OK) return 2;
    return -1;
}

const char* c_pg_result_error(PGResultPtr res) {
    if (!res) return "NULL PGresult";
    return PQresultErrorMessage((PGresult*)res);
}

void c_pg_clear_result(PGResultPtr res) {
    if (res) {
        PQclear((PGresult*)res);
    }
}

int c_pg_ntuples(PGResultPtr res) {
    if (!res) return 0;
    return PQntuples((PGresult*)res);
}

int c_pg_nfields(PGResultPtr res) {
    if (!res) return 0;
    return PQnfields((PGresult*)res);
}

const char* c_pg_fname(PGResultPtr res, int field_idx) {
    if (!res) return "";
    char *name = PQfname((PGresult*)res, field_idx);
    return name ? name : "";
}

const char* c_pg_getvalue(PGResultPtr res, int tuple_idx, int field_idx) {
    if (!res) return "";
    return PQgetvalue((PGresult*)res, tuple_idx, field_idx);
}

int c_pg_getisnull(PGResultPtr res, int tuple_idx, int field_idx) {
    if (!res) return 1;
    return PQgetisnull((PGresult*)res, tuple_idx, field_idx);
}

int c_pg_cmd_tuples(PGResultPtr res) {
    if (!res) return 0;
    const char *str = PQcmdTuples((PGresult*)res);
    if (!str || !*str) return 0;
    return atoi(str);
}

const char* c_pg_result_sqlstate(PGResultPtr res) {
    if (!res) return NULL;
    return PQresultErrorField((PGresult*)res, PG_DIAG_SQLSTATE);
}

int c_pg_ftype(PGResultPtr res, int field_idx) {
    if (!res) return 0;
    return (int)PQftype((PGresult*)res, field_idx);
}

int c_pg_set_nonblocking(PGConnPtr conn, int arg) {
    if (!conn) return -1;
    return PQsetnonblocking((PGconn*)conn, arg);
}

int c_pg_is_nonblocking(PGConnPtr conn) {
    if (!conn) return 0;
    return PQisnonblocking((PGconn*)conn);
}

int c_pg_socket(PGConnPtr conn) {
    if (!conn) return -1;
    return PQsocket((PGconn*)conn);
}

int c_pg_send_query_params(PGConnPtr conn, const char* sql, int num_params, const char* const* params) {
    if (!conn || !sql) return 0;
    return PQsendQueryParams(
        (PGconn*)conn,
        sql,
        num_params,
        NULL, // Infer param types
        params,
        NULL, // Text formats
        NULL, // Text formats
        0     // Return text format
    );
}

int c_pg_flush(PGConnPtr conn) {
    if (!conn) return -1;
    return PQflush((PGconn*)conn);
}

int c_pg_consume_input(PGConnPtr conn) {
    if (!conn) return 0;
    return PQconsumeInput((PGconn*)conn);
}

int c_pg_is_busy(PGConnPtr conn) {
    if (!conn) return 0;
    return PQisBusy((PGconn*)conn);
}

PGResultPtr c_pg_get_result(PGConnPtr conn) {
    if (!conn) return NULL;
    PGresult *res = PQgetResult((PGconn*)conn);
    return (PGResultPtr)res;
}
