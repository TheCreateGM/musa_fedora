# Build the jump shims and regenerate the mapping tables.
#
# `make`        assemble the shims from src/*.S (what the RPM build runs)
# `make regen`  regenerate maps + shims from the installed MUSA SDK/runtime,
#               then show the diff so a maintainer can review it
#
# SPDX-License-Identifier: MIT
CC      ?= gcc
MUSA_RT ?= /usr/local/musa
MUSA_SDK ?= /usr/local/musa-5.2

SHIMS := libcudart.so.11.0 libcuda.so.1

all: $(SHIMS)

libcudart.so.11.0: src/cudart_jumps.S
	$(CC) -O2 -fPIC -shared -o $@ $< -L$(MUSA_RT)/lib -lmusart \
	      -Wl,-soname,$@ -Wl,--no-undefined

libcuda.so.1: src/cuda_jumps.S
	$(CC) -O2 -fPIC -shared -o $@ $< -L$(MUSA_RT)/lib -lmusa \
	      -Wl,-soname,$@ -Wl,--no-undefined

regen:
	python3 gen-cuda-compat.py --include $(MUSA_SDK)/include \
	        --old-include $(MUSA_RT)/include --libdir $(MUSA_RT)/lib --out .
	mv cudart_jumps.S cuda_jumps.S src/
	git --no-pager diff --stat -- include src
	$(MAKE) clean all

clean:
	rm -f $(SHIMS)

.PHONY: all regen clean
