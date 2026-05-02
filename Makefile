CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -I. -Ithird_party

PGO_DIR ?= pgo

PGO_GEN_FLAGS = -fprofile-generate -fprofile-dir=$(PGO_DIR)
PGO_USE_FLAGS = -fprofile-use -fprofile-correction -fprofile-dir=$(PGO_DIR)

PGO ?=
ifeq ($(PGO),gen)
	CXXFLAGS += $(PGO_GEN_FLAGS)
else ifeq ($(PGO),use)
	CXXFLAGS += $(PGO_USE_FLAGS)
endif

# Generate Tables
GEN_H = generate_tables/common.h \
    generate_tables/fsm.h \
    generate_tables/fsm.inl \
    generate_tables/memory_layout.h \
    generate_tables/memory_layout.inl \
    generate_tables/phe.h \
    generate_tables/phe.inl \
    third_party/senzee/poker.h \
    third_party/senzee/mtrand.h

GEN_CC = generate_tables/generate_tables.cc \
    generate_tables/common.cc \
    third_party/senzee/poker.cc \
    third_party/senzee/mtrand.cc

bin/generate_tables: $(GEN_H) $(GEN_CC)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(GEN_CC)

.PHONY: tables
tables: bin/generate_tables
	mkdir -p tables
	./bin/generate_tables

# Benchmarks
BENCH_H = poker_hand_eval.h
BENCH_CC = benchmarks/benchmarks.cc
bin/benchmarks: $(BENCH_H) $(BENCH_CC)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(BENCH_CC)

.PHONY: bench
bench: bin/benchmarks
	./bin/benchmarks

.PHONY: clean
clean:
	rm -rf bin tables $(PGO_DIR)

.PHONY: pgo-bench
pgo-bench:
	mkdir -p $(PGO_DIR)
	rm -f bin/benchmarks
	rm -rf $(PGO_DIR)/*.gcda
	$(MAKE) PGO=gen bin/benchmarks
	./bin/benchmarks > /dev/null
	rm -f bin/benchmarks
	$(MAKE) PGO=use bin/benchmarks
	./bin/benchmarks
