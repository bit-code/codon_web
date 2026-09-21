"""Python JSON Benchmark Counterpart.
Tests Python stdlib json and fast orjson on identical workloads (including 1.4MB+).
"""

import json
import time

try:
    import orjson
    HAS_ORJSON = True
except ImportError:
    HAS_ORJSON = False

# Payload A: 1KB Web Object
WEB_JSON_STR = '{"status":"success","code":200,"message":"User fetched successfully","data":{"user_id":1048576,"username":"antigravity_engineer","email":"engineer@deepmind.internal","is_active":true,"score":98.75,"roles":["admin","developer","maintainer"],"metadata":{"created_at":"2026-01-15T08:30:00Z","last_login":"2026-09-20T22:15:30Z","login_count":1420,"preferences":{"theme":"dark","notifications":true,"locale":"zh-CN"}}}}'
web_obj = json.loads(WEB_JSON_STR)

# Payload B: 1000 items List (~92KB)
list_objs = [
    {
        "id": i,
        "name": f"item_{i}",
        "price": round(10.5 + (i % 100) * 1.5, 2),
        "in_stock": (i % 2 == 0),
        "tags": ["tagA", "tagB"]
    }
    for i in range(1000)
]
LARGE_LIST_JSON = json.dumps(list_objs)

# Payload C: 15000 items List (> 1.4MB)
huge_list_objs = [
    {
        "id": i,
        "name": f"item_{i}",
        "price": round(10.5 + (i % 100) * 1.5, 2),
        "in_stock": (i % 2 == 0),
        "tags": ["tagA", "tagB"]
    }
    for i in range(15000)
]
HUGE_LIST_JSON = json.dumps(huge_list_objs)

def bench(fn, iterations):
    for _ in range(min(5, iterations)):
        fn()
    t0 = time.perf_counter()
    for _ in range(iterations):
        fn()
    el = time.perf_counter() - t0
    ops = iterations / el
    ms = (el / iterations) * 1000.0
    return ms, ops

def print_metric(label, ms, ops, byte_size=0):
    mb_s = (byte_size * ops) / (1024.0 * 1024.0) if byte_size > 0 else 0.0
    mb_str = f" | {mb_s:7.1f} MB/s" if byte_size > 0 else ""
    print(f"  {label:<35}: {ms:8.5f} ms/op | {int(ops):9d} ops/s{mb_str}")

def main():
    sz_web = len(WEB_JSON_STR)
    sz_list = len(LARGE_LIST_JSON)
    sz_huge = len(HUGE_LIST_JSON)

    print("=" * 78)
    print("  Python JSON Benchmark (CPython 3.10 stdlib vs orjson)")
    print(f"  载荷 A: {sz_web} 字节 | 载荷 B: {sz_list} 字节 | 载荷 C: {sz_huge} 字节 ({sz_huge/(1024*1024):.2f} MB)")
    print("=" * 78)

    # 1. Web Loads
    ms, ops = bench(lambda: json.loads(WEB_JSON_STR), 100000)
    print_metric("Python stdlib (loads 1KB)", ms, ops, sz_web)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.loads(WEB_JSON_STR), 100000)
        print_metric("Python orjson (loads 1KB)", ms, ops, sz_web)

    # 2. Web Dumps
    ms, ops = bench(lambda: json.dumps(web_obj), 100000)
    print_metric("Python stdlib (dumps 1KB)", ms, ops, sz_web)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.dumps(web_obj), 100000)
        print_metric("Python orjson (dumps 1KB)", ms, ops, sz_web)

    # 3. List Loads
    ms, ops = bench(lambda: json.loads(LARGE_LIST_JSON), 2000)
    print_metric("Python stdlib (loads 92KB)", ms, ops, sz_list)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.loads(LARGE_LIST_JSON), 2000)
        print_metric("Python orjson (loads 92KB)", ms, ops, sz_list)

    # 4. List Dumps
    ms, ops = bench(lambda: json.dumps(list_objs), 2000)
    print_metric("Python stdlib (dumps 92KB)", ms, ops, sz_list)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.dumps(list_objs), 2000)
        print_metric("Python orjson (dumps 92KB)", ms, ops, sz_list)

    # 5. Huge Loads (1.4MB+)
    ms, ops = bench(lambda: json.loads(HUGE_LIST_JSON), 150)
    print_metric("Python stdlib (loads 1.4MB)", ms, ops, sz_huge)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.loads(HUGE_LIST_JSON), 150)
        print_metric("Python orjson (loads 1.4MB)", ms, ops, sz_huge)

    # 6. Huge Dumps (1.4MB+)
    ms, ops = bench(lambda: json.dumps(huge_list_objs), 150)
    print_metric("Python stdlib (dumps 1.4MB)", ms, ops, sz_huge)
    if HAS_ORJSON:
        ms, ops = bench(lambda: orjson.dumps(huge_list_objs), 150)
        print_metric("Python orjson (dumps 1.4MB)", ms, ops, sz_huge)
    print("=" * 78)

if __name__ == "__main__":
    main()
