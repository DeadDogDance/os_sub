PROGRAM = hello_world.a
CC = gcc
CFLAGS = -g -Wall


SRCS := $(wildcard *.c)
HDRS := $(wildcard *.h)
OBJS := $(SRCS:.c=.o)


all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $^ -o ../$@

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@


.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS) $(PROGRAM)

.PHONY: all clean
