MAKE	   = make --print-directory
CC         = clang
CFLAGS    += --std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -pedantic
LDFLAGS   += 
LDLIBS    += 
SRCS       = simple-tcp-server.c
OBJS       = $(SRCS:.c=.o)

EXTERNAL_OBJS  = external/c_utility/collections/socketlist.o 
EXTERNAL_OBJS += external/c_utility/collections/bufferlist.o
EXTERNAL_OBJS += external/c_utility/net/net.o

TARGETS    = main

SUBDIRS   += external/c_utility/collections/ external/c_utility/net/

all: $(OBJS) $(TARGETS)

$(TARGETS): main.c $(OBJS)
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir; \
	done
	$(CC) -o main main.c $(OBJS) $(EXTERNAL_OBJS) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(ARGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) *.o $(TARGETS)
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir && $(MAKE) clean) ; \
	done

.PHONY: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)

