APP = package_handle

# Add any other object files to this list below
OBJS = package_handle.o
OBJS_SO = package_handle.so.o

LIB = libauxfpgaload.so.0.0.1
LDFLAGS_SO = -shared -Wl,-soname,libauxfpgaload.so.0
CFLAGS_SO = -fPIC

all: $(APP) $(LIB)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(LIB): $(OBJS_SO)
	$(CC) $(CFLAGS) $(CFLAGS_SO) -o $@ $(OBJS_SO) $(LDFLAGS) $(LDFLAGS_SO)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.so.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_SO) -c -o $@ $<

clean:
	-rm -f $(APP) $(LIB) *.elf *.gdb *.o
