LIST = list

# Add any other object files to this list below
LIST_OBJS = list.o

LDLIBS += -lubox -lubus -lz -ljson-c -lblobmsg_json

all: build

build: $(LIST)

$(LIST): $(LIST_OBJS)
	$(CC) -o $@ $(LIST_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(LIST) *.o
