#!/bin/bash
set -e
pkill -9 -f bench_mysql_server || true
sleep 1

cd /mnt/e/AiProject/Antigravity/codon_web
./bench_mysql_server 1 > server_output.log 2>&1 &
SERVER_PID=$!
echo "Started server with PID: $SERVER_PID"

sleep 2
echo "Starting wrk benchmark: 1 thread, 60 connections, 10s..."
wrk -t1 -c60 -d10s --latency http://localhost:18080/api/db-real-fast

echo "wrk finished! Checking server health..."
sleep 2

if kill -0 $SERVER_PID 2>/dev/null; then
    echo "SUCCESS: Server is ALIVE and STABLE after benchmark!"
    kill -9 $SERVER_PID 2>/dev/null || true
    exit 0
else
    echo "FAILURE: Server crashed!"
    cat server_output.log
    exit 1
fi
