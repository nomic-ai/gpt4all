INCLUDE_PATH := $(abspath ./)
LIBRARY_PATH := $(abspath ./)
CMAKEFLAGS=

ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

ifndef UNAME_P
UNAME_P := $(shell uname -p)
endif

ifndef UNAME_M
UNAME_M := $(shell uname -m)
endif

CCV := $(shell $(CC) --version | head -n 1)
CXXV := $(shell $(CXX) --version | head -n 1)

# Mac OS + Arm can report x86_64
# ref: https://github.com/ggerganov/whisper.cpp/issues/66#issuecomment-1282546789
ifeq ($(UNAME_S),Darwin)
	ifneq ($(UNAME_P),arm)
		SYSCTL_M := $(shell sysctl -n hw.optional.arm64 2>/dev/null)
		ifeq ($(SYSCTL_M),1)
			# UNAME_P := arm
			# UNAME_M := arm64
			warn := $(warning Your arch is announced as x86_64, but it seems to actually be ARM64. Not fixing that can lead to bad performance. For more info see: https://github.com/ggerganov/whisper.cpp/issues/66\#issuecomment-1282546789)
		endif
	endif
endif

#
# Compile flags
#

# keep standard at C11 and C++11
CFLAGS   = -I. -I../../gpt4all-backend/llama.cpp -I../../gpt4all-backend -I -O3 -DNDEBUG -std=c11 -fPIC
CXXFLAGS = -I. -I../../gpt4all-backend/llama.cpp -I../../gpt4all-backend -O3 -DNDEBUG -std=c++17 -fPIC
LDFLAGS  =

# warnings
CFLAGS   += -Wall -Wextra -Wpedantic -Wcast-qual -Wdouble-promotion -Wshadow -Wstrict-prototypes -Wpointer-arith -Wno-unused-function
CXXFLAGS += -Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function -Wno-multichar

# OS specific
# TODO: support Windows
ifeq ($(UNAME_S),Linux)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),FreeBSD)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),NetBSD)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),OpenBSD)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),Haiku)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686))
	# Use all CPU extensions that are available:
	CFLAGS += -march=native -mtune=native
	CXXFLAGS += -march=native -mtune=native
endif
ifneq ($(filter ppc64%,$(UNAME_M)),)
	POWER9_M := $(shell grep "POWER9" /proc/cpuinfo)
	ifneq (,$(findstring POWER9,$(POWER9_M)))
		CFLAGS += -mcpu=power9
		CXXFLAGS += -mcpu=power9
	endif
	# Require c++23's std::byteswap for big-endian support.
	ifeq ($(UNAME_M),ppc64)
		CXXFLAGS += -std=c++23 -DGGML_BIG_ENDIAN
	endif
endif
ifndef LLAMA_NO_ACCELERATE
	# Mac M1 - include Accelerate framework.
	# `-framework Accelerate` works on Mac Intel as well, with negliable performance boost (as of the predict time).
	ifeq ($(UNAME_S),Darwin)
		CFLAGS  += -DGGML_USE_ACCELERATE
		LDFLAGS += -framework Accelerate
	endif
endif
ifdef LLAMA_OPENBLAS
	CFLAGS  += -DGGML_USE_OPENBLAS -I/usr/local/include/openblas
	LDFLAGS += -lopenblas
endif
ifdef LLAMA_GPROF
	CFLAGS   += -pg
	CXXFLAGS += -pg
endif
ifneq ($(filter aarch64%,$(UNAME_M)),)
	CFLAGS += -mcpu=native
	CXXFLAGS += -mcpu=native
endif
ifneq ($(filter armv6%,$(UNAME_M)),)
	# Raspberry Pi 1, 2, 3
	CFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access
endif
ifneq ($(filter armv7%,$(UNAME_M)),)
	# Raspberry Pi 4
	CFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access -funsafe-math-optimizations
endif
ifneq ($(filter armv8%,$(UNAME_M)),)
	# Raspberry Pi 4
	CFLAGS += -mfp16-format=ieee -mno-unaligned-access
endif

#
# Print build information
#

$(info I go-gpt4all build info: )
$(info I UNAME_S:  $(UNAME_S))
$(info I UNAME_P:  $(UNAME_P))
$(info I UNAME_M:  $(UNAME_M))
$(info I CFLAGS:   $(CFLAGS))
$(info I CXXFLAGS: $(CXXFLAGS))
$(info I LDFLAGS:  $(LDFLAGS))
$(info I CMAKEFLAGS:  $(CMAKEFLAGS))
$(info I CC:       $(CCV))
$(info I CXX:      $(CXXV))
$(info )

llmodel.o:
	[ -e buildllm ] || mkdir buildllm
	cd buildllm && cmake ../../../gpt4all-backend/ $(CMAKEFLAGS) && make
	cd buildllm && cp -rf CMakeFiles/llmodel.dir/llmodel_c.cpp.o ../llmodel_c.o
	cd buildllm && cp -rf CMakeFiles/llmodel.dir/llmodel.cpp.o ../llmodel.o

clean:
	rm -f *.o
	rm -f *.a
	rm -rf buildllm
	rm -rf example/main

binding.o: binding.cpp binding.h
	$(CXX) $(CXXFLAGS) binding.cpp -o binding.o -c $(LDFLAGS)

libgpt4all.a: binding.o llmodel.o
	ar src libgpt4all.a llmodel.o binding.o

test: libgpt4all.a
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go test -v ./...

example/main: libgpt4all.a
	C_INCLUDE_PATH=$(INCLUDE_PATH) LIBRARY_PATH=$(INCLUDE_PATH) go build -o example/main ./example/
