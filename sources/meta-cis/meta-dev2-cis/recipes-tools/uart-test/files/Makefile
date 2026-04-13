SRCS := $(wildcard *.c)

EXECS := $(SRCS:.c=)


.PHONY: all
all: $(EXECS)

%: %.c
	$(CC) $(LDFLAGS) $(CCFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm -f $(EXECS) *.elf *.gdb *.o


