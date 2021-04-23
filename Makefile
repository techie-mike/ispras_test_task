# Required packages:
# Ubuntu 20.04: clang-11 gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user
# Ubuntu 18.04 (change 11 to 10): clang-10 gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user
CC=clang-11
CXX=clang++-11
OPT=opt-11
LLVM_CONFIG=llvm-config-11
LLC=llc-11
QEMU_USER=qemu-aarch64
QEMU_LD_PREFIX=/usr/aarch64-linux-gnu
BENCH_ARG=21

CFLAGS=-O2 -Werror -Wall -pedantic -fno-inline-functions -fPIC
CFLAGS_CROSS=-I$(QEMU_LD_PREFIX)/include -target aarch64-linux-gnu -ffixed-x28
LDLIBS=-lm

BENCH=binary_trees
BENCH_REF=$(BENCH).ref
BENCH_OPT=$(BENCH).opt
OUTPUT_REF=out.ref
OUTPUT_OPT=out.opt
PASS_NAME=asm_inserter
#PASS_NAME=reg_inserter

$(BENCH_REF): $(BENCH).c
	$(CC) $(CFLAGS) $(CFLAGS_CROSS) $(LDLIBS) -o $@ $<

$(PASS_NAME).so: $(PASS_NAME).cpp
	$(CXX) $(CFLAGS) `$(LLVM_CONFIG) --cxxflags` -shared -fPIC -o $@ $<

$(BENCH_OPT): $(BENCH).c $(PASS_NAME).so
	$(CC) $(CFLAGS) $(CFLAGS_CROSS) -S -emit-llvm -o $(BENCH).orig.ll $<
	$(OPT) -load ./$(PASS_NAME).so -S -$(PASS_NAME) < $(BENCH).orig.ll > $(BENCH).ll
	$(LLC) -O2 --relocation-model=pic -o $(BENCH).s $(BENCH).ll
	$(CC) $(CFLAGS_CROSS) $(LDLIBS) $(BENCH).s -o $@

.PHONY: run-ref
run-ref: $(BENCH_REF)
	time -p $(QEMU_USER) -L $(QEMU_LD_PREFIX) ./$(BENCH_REF) $(BENCH_ARG) > $(OUTPUT_REF)

.PHONY: run-opt
run-opt: $(BENCH_OPT)
	time -p $(QEMU_USER) -L $(QEMU_LD_PREFIX) ./$(BENCH_OPT) $(BENCH_ARG) > $(OUTPUT_OPT)

.PHONY: compare
compare: run-ref run-opt
	diff $(OUTPUT_REF) $(OUTPUT_OPT)

.PHONY: clean
clean:
	rm -f $(BENCH_REF) $(OUTPUT_REF) \
	      $(PASS_NAME).so \
	      $(BENCH).orig.ll $(BENCH).ll $(BENCH).s $(BENCH_OPT) $(OUTPUT_OPT)

