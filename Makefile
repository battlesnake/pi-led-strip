sources := $(wildcard *.c)

objects := $(sources:%.c=%.o)
program := led-animation

cflags := -O2 -march=native -mtune=native -Wall -Wextra -Werror -ffunction-sections -fdata-sections -flto -c

ldflags := -O2 -Wall -Wextra -Werror -Wl,--gc-sections -flto -s

libs := m

.PHONY: default build clean sysinit

default: build

sysinit:
	sudo modprobe spidev
	sudo dtparam spi=on

build: $(program)

clean:
	rm -rf -- $(objects) $(program) *.d

$(program): $(objects)
	$(CC) $(ldflags) -MMD -o $(program) $(objects) $(addprefix -l,$(libs))

%.o: %.c
	$(CC) $(cflags) -MMD -o $@ $<

-include: $(wildcard *.d)
