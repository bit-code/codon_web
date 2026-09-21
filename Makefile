CODON ?= $(shell which codon 2>/dev/null || echo $(HOME)/.codon/bin/codon)

.PHONY: all native bench-json test-json test test-all clean

all: native

native:
	$(MAKE) -C src/native

test-json: native
	CODON_PATH=. $(CODON) build -release --linker-flags 'src/native/libcfastjson.a' -o test_fast_json_integration tests/test_fast_json_integration.codon
	./test_fast_json_integration

bench-json: native
	CODON_PATH=. $(CODON) build -release --linker-flags 'src/native/libcfastjson.a' -o test_json tests/benchmark/test_json.codon
	./test_json

test: native test-json
	CODON_PATH=. $(CODON) build -release --linker-flags 'src/native/libcfastjson.a' -o test_req tests/test_request_ext.codon
	./test_req
	CODON_PATH=. $(CODON) build -release --linker-flags 'src/native/libcfastjson.a' -o test_tc tests/test_testclient.codon
	./test_tc

test-all: native
	CODON_PATH=. $(CODON) run --linker-flags 'src/native/libcfastjson.a' tests/run_all.codon

clean:
	$(MAKE) -C src/native clean
	rm -f test_json test_fast_json_integration test_req test_tc test_run_all bench_mysql_server examples/bench_mysql_server
	rm -f tests/test_fast_json_integration tests/test_request_ext tests/test_testclient tests/benchmark/test_json.o

