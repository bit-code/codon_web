#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

CODON_BIN="$PROJECT_ROOT/bench_mysql_server"
ACTIX_BIN="$PROJECT_ROOT/benchmarks/actix_mysql_server/target/release/actix_mysql_server"

CODON_PORT=18080
ACTIX_PORT=18081
WORKERS=4
THREADS=4
CONCURRENCY=100
DURATION="10s"

echo "=========================================================="
echo "      CodonWeb vs Actix-Web Automated Benchmark Suite     "
echo "=========================================================="
echo "Concurrency: $CONCURRENCY | Threads: $THREADS | Duration: $DURATION"
echo ""

# 提高系统句柄限制
ulimit -n 65535 2>/dev/null || true

# 检查可执行文件是否存在
if [ ! -f "$CODON_BIN" ]; then
    echo "[!] CodonWeb binary not found at $CODON_BIN. Compiling now..."
    CODON_PATH=. codon build -release --mcpu=native --inline-threshold=1000 --linker-flags 'src/native/libcfastjson.a -lpq' -o bench_mysql_server examples/bench_mysql_server.codon
fi

if [ ! -f "$ACTIX_BIN" ]; then
    echo "[!] Actix-Web binary not found at $ACTIX_BIN. Compiling now..."
    (cd "$PROJECT_ROOT/benchmarks/actix_mysql_server" && cargo build --release)
fi

CODON_STARTED_BY_SCRIPT=0
ACTIX_STARTED_BY_SCRIPT=0

cleanup() {
    echo ""
    echo ">> Cleaning up benchmark processes..."
    killall -9 bench_mysql_server 2>/dev/null || true
    killall -9 actix_mysql_server 2>/dev/null || true
    fuser -k ${CODON_PORT}/tcp 2>/dev/null || true
    fuser -k ${ACTIX_PORT}/tcp 2>/dev/null || true
    echo ">> Done."
}

trap cleanup EXIT INT TERM

# 确保无残留旧实例
killall -9 bench_mysql_server 2>/dev/null || true
killall -9 actix_mysql_server 2>/dev/null || true
fuser -k ${CODON_PORT}/tcp 2>/dev/null || true
fuser -k ${ACTIX_PORT}/tcp 2>/dev/null || true
sleep 0.5

# 1. 启动 CodonWeb
echo "[*] Starting CodonWeb (workers=$WORKERS, port=$CODON_PORT)..."
"$CODON_BIN" --workers=$WORKERS --port=$CODON_PORT >/dev/null 2>&1 &
CODON_PID=$!
CODON_STARTED_BY_SCRIPT=1

# 等待服务就绪
RETRY=0
while ! curl -s "http://127.0.0.1:$CODON_PORT/api/plaintext" >/dev/null 2>&1; do
    sleep 0.2
    RETRY=$((RETRY + 1))
    if [ $RETRY -gt 30 ]; then
        echo "[-] Error: Failed to start CodonWeb server."
        exit 1
    fi
done
echo "[+] CodonWeb is ready on port $CODON_PORT (PID: $CODON_PID)"

# 2. 启动 Actix-Web
echo "[*] Starting Actix-Web (workers=$WORKERS, port=$ACTIX_PORT)..."
"$ACTIX_BIN" --workers=$WORKERS --port=$ACTIX_PORT >/dev/null 2>&1 &
ACTIX_PID=$!
ACTIX_STARTED_BY_SCRIPT=1

# 等待服务就绪
RETRY=0
while ! curl -s "http://127.0.0.1:$ACTIX_PORT/api/plaintext" >/dev/null 2>&1; do
    sleep 0.2
    RETRY=$((RETRY + 1))
    if [ $RETRY -gt 30 ]; then
        echo "[-] Error: Failed to start Actix-Web server."
        exit 1
    fi
done
echo "[+] Actix-Web is ready on port $ACTIX_PORT (PID: $ACTIX_PID)"

echo ""
echo "=========================================================="
echo "          Both Benchmark Servers Ready. Starting wrk!     "
echo "=========================================================="
echo ""

run_benchmark() {
    local title=$1
    local endpoint=$2

    echo "##########################################################"
    echo ">> [Benchmark] $title ($endpoint)"
    echo "##########################################################"

    echo "--- 1. Testing CodonWeb ---"
    wrk -t$THREADS -c$CONCURRENCY -d$DURATION --latency "http://127.0.0.1:$CODON_PORT$endpoint"
    echo ""

    echo "--- 2. Testing Actix-Web ---"
    wrk -t$THREADS -c$CONCURRENCY -d$DURATION --latency "http://127.0.0.1:$ACTIX_PORT$endpoint"
    echo ""
}

run_benchmark "1. Plaintext Throughput" "/api/plaintext"
run_benchmark "2. Standard JSON Serialization" "/api/json-std"
run_benchmark "3. Real MySQL Async Query (mass_data)" "/api/db-real"

echo "=========================================================="
echo "            All Benchmark Tests Completed Cleanly!        "
echo "=========================================================="
