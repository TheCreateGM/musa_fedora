# musa-infer — build against the MUSA 1.5.4 runtime through the CUDA-compat
# nvcc wrapper. Override MUSA_CUDA_DIR/MUSA_RT if the installation differs.

MUSA_CUDA_DIR ?= /usr/local/cuda
MUSA_RT       ?= /usr/local/musa
NVCC          ?= $(MUSA_CUDA_DIR)/bin/nvcc
CXX           ?= g++
CXXFLAGS      ?= -O3 -std=c++17 -fopenmp -Wall -Wextra
CPPFLAGS      ?= -I.
LDFLAGS       ?= -L$(MUSA_CUDA_DIR)/lib64 -L$(MUSA_RT)/lib \
                 -Wl,-rpath,$(MUSA_CUDA_DIR)/lib64 -Wl,-rpath,$(MUSA_RT)/lib
LDLIBS        ?= -lcudart -lmusart -lmusa -fopenmp

OBJS = gguf.o tokenizer.o model.o main.o musa_host.o musa_backend.o

all: musa-infer

musa-infer: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

musa_backend.o: musa_backend.cu musa_layout.h
	$(NVCC) -O3 -c $< -o $@

%.o: %.cpp gguf.h model.h tokenizer.h musa_backend.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) musa-infer

.PHONY: all clean
