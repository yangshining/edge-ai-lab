RE_BOOT = re_boot

RE_BOOT_OBJS = re-boot.o

all: $(RE_BOOT)

$(RE_BOOT): $(RE_BOOT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(RE_BOOT_OBJS) $(LDLIBS) -lgpiod -lubootenv -lz

clean:
	-rm -f $(RE_BOOT) *.elf *.gdb *.o
