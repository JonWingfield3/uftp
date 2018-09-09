CPP = g++
CFLAGS = -std=c++14 -g
CPPFLAGS = -I../common/

# Run make DEBUG=1 to enable debug build
DEBUG_FLAG = -D__DEBUG__
DEBUG ?= 1
ifeq ($(DEBUG), 1)
  CPPFLAGS += $(DEBUG_FLAG)
endif

all: uftp_client

uftp_client.o: uftp_client.cpp uftp_client.h ../common/uftp_utils.h ../common/uftp_defs.h
	$(CPP) $(CFLAGS) $(CPPFLAGS) $< -c

uftp_utils.o: ../common/uftp_utils.cpp ../common/uftp_utils.h ../common/uftp_defs.h
	$(CPP) $(CFLAGS) $(CPPFLAGS) $< -c

uftp_client: uftp_client.o uftp_utils.o
	$(CPP) $(CFLAGS) $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm uftp_client *.o
