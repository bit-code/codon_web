CODON ?= $(shell which codon 2>/dev/null || echo $(HOME)/.codon/bin/codon)

.PHONY: all native bench-json test-json test test-orm test-schema demo-schema codegen test-all clean

all: native codegen

native:
	$(MAKE) -C src/native

LINK_FLAGS ?= src/native/libcfastjson.a -lpq
CODON_FLAGS ?= -release --mcpu=native --inline-threshold=1000

test-json: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_fast_json_integration tests/test_fast_json_integration.codon
	./test_fast_json_integration

bench-json: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_json tests/benchmark/test_json.codon
	./test_json

test: native codegen test-json
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_req tests/test_request_ext.codon
	./test_req
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_tc tests/test_testclient.codon
	./test_tc
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_za tests/test_zero_alloc.codon
	./test_za
	rm -f test_req test_tc test_za

test-orm: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_orm_dialects tests/test_orm_dialects.codon
	./test_orm_dialects
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_sql tests/test_sql_builder.codon
	./test_sql
	rm -f test_orm_dialects test_sql

test-sql: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_sql tests/test_sql_builder.codon
	./test_sql
	rm -f test_sql

test-pg: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_pg tests/test_async_postgres.codon
	./test_pg
	rm -f test_pg

test-taskq: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_taskq tests/test_taskq.codon
	./test_taskq
	rm -f test_taskq

test-async-taskq: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_async_taskq tests/test_async_taskq.codon
	./test_async_taskq
	rm -f test_async_taskq

test-cancellation: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_cancellation tests/test_cancellation.codon
	./test_cancellation
	rm -f test_cancellation

test-redis: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_taskq_redis tests/test_taskq_redis.codon
	./test_taskq_redis
	rm -f test_taskq_redis

test-advanced: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_taskq_advanced tests/test_taskq_advanced.codon
	./test_taskq_advanced
	rm -f test_taskq_advanced

test-redis-driver: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_redis_driver tests/test_redis_driver.codon
	./test_redis_driver
	rm -f test_redis_driver

test-all: native codegen
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_all tests/run_all.codon
	./test_all

bench-pipeline: native
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o bench_pipeline tests/benchmark/bench_fast_pipeline.codon
	./bench_pipeline

# -----------------------------------------------------------------------------
# Code-First AOT Schema 零配置全工程自动探测与预编译流水线
# 采用双重特征防碰撞校验（Double-Key Verification）：
# 1. 继承自 SchemaModel 或 BaseModel
# 2. 且显式导入了 from src.schema 或 from codon_web
# 彻底杜绝第三方库或 ORM 的同名 BaseModel 命名污染
# -----------------------------------------------------------------------------
AUTO_SCHEMA_SRCS := $(shell find src examples -type f -name "*.codon" 2>/dev/null | \
	grep -v "src/generated" | grep -v "src/schema" | grep -v "tools/codegen" | grep -v "tests/test_aot_schema.codon" | \
	xargs grep -l -E "(SchemaModel|\(BaseModel\))" 2>/dev/null | \
	xargs grep -l -E "(from src\.schema|from codon_web)" 2>/dev/null)

# CodeGen Engine: jinja (modern AST+Jinja2, recommended) or codon (legacy parser)
CODEGEN_ENGINE ?= jinja

codegen:
	@if [ -n "$(AUTO_SCHEMA_SRCS)" ]; then \
		echo "[CodeGen Auto-Discovery] Discovered verified schema files: $(AUTO_SCHEMA_SRCS)"; \
		if [ "$(CODEGEN_ENGINE)" = "jinja" ]; then \
			python3 tools/codegen/py_codegen/cli.py --in-place $(AUTO_SCHEMA_SRCS); \
		else \
			CODON_PATH=. $(CODON) run tools/codegen/main.codon --in-place $(AUTO_SCHEMA_SRCS); \
		fi \
	else \
		echo "[CodeGen Auto-Discovery] No verified schema definitions detected."; \
	fi

codegen-force:
	@if [ -n "$(AUTO_SCHEMA_SRCS)" ]; then \
		echo "[CodeGen Auto-Discovery] Force re-generating schema files: $(AUTO_SCHEMA_SRCS)"; \
		if [ "$(CODEGEN_ENGINE)" = "jinja" ]; then \
			python3 tools/codegen/py_codegen/cli.py --in-place --force $(AUTO_SCHEMA_SRCS); \
		else \
			CODON_PATH=. $(CODON) run tools/codegen/main.codon --in-place --force $(AUTO_SCHEMA_SRCS); \
		fi \
	fi

test-schema: native codegen
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o test_aot_schema tests/test_aot_schema.codon
	./test_aot_schema
	rm -f test_aot_schema

demo-schema: native codegen
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o demo_schema examples/aot_schema_web_demo.codon
	./demo_schema
	rm -f demo_schema

demo-fulfillment: native codegen
	CODON_PATH=. $(CODON) build $(CODON_FLAGS) --linker-flags '$(LINK_FLAGS)' -o demo_fulfillment examples/smart_fulfillment/test_fulfillment_app.codon
	./demo_fulfillment
	rm -f demo_fulfillment

clean:
	$(MAKE) -C src/native clean
	rm -f test_all test_json test_fast_json_integration test_req test_tc test_mem test_mime test_rt test_parser bench_mysql_server examples/bench_mysql_server bench_pipeline test_orm_dialects test_orm_crud test_pg test_aot_schema demo_schema demo_fulfillment
	rm -f tests/test_fast_json_integration tests/test_request_ext tests/test_testclient tests/benchmark/test_json.o

