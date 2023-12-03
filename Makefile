sources := $(wildcard *.c)

objects := $(sources:%.c=%.o)
program := led-animation

cflags := -O2 -march=native -mtune=native -Wall -Wextra -Werror -ffunction-sections -fdata-sections -flto -c

ldflags := -O2 -Wall -Wextra -Werror -Wl,--gc-sections -flto -s

libs := m

.PHONY: default build clean sysinit install

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

install:
	while read -r line; do \
		printf -- "%s\n" "$$line"; \
		if printf "%s" "$$line" | grep -sq ^Type=; then \
			printf -- "%s=%s\n" "WorkingDirectory" "$(CURDIR)"; \
		fi; \
	done < leds.service | sudo tee /etc/systemd/system/leds.service
	sudo systemctl daemon-reload
	sudo systemctl enable --now leds.service

-include: $(wildcard *.d)
