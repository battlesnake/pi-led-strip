#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "sk9822.h"

#define spi_config(fd, name, value) (_spi_config(fd, #name, SPI_IOC_RD_##name, SPI_IOC_WR_##name, value))

static int _spi_config(int fd, const char *name, int read_code, int write_code, int value)
{
	if (ioctl(fd, write_code, &value) != 0) {
		perror("ioctl read");
		return 1;
	}
	int verify = 0;
	if (ioctl(fd, read_code, &verify) != 0) {
		perror("ioctl write");
		return 1;
	}
	if (value != verify) {
		fprintf(stderr, "ioctl write successful but value not set exactly for %s: write %d, read %d\n", name, value, verify);
	}
	return 0;
}

struct sk9822 *sk9822_init(const char *spidev, int speed, size_t num_leds)
{
	struct sk9822* this = malloc(sizeof(*this));
	if (!this) {
		return NULL;
	}
	memset(this, 0, sizeof(*this));
	this->fd = open(spidev, O_RDWR);
	if (this->fd < 0) {
		perror("open(spidev)");
		goto fail;
	}
	if (spi_config(this->fd, MODE, SPI_NO_CS) != 0) {
		perror("MODE");
		goto fail;
	}
	if (spi_config(this->fd, BITS_PER_WORD, 8) != 0) {
		perror("BITS_PER_WORD");
		goto fail;
	}
	if (spi_config(this->fd, MAX_SPEED_HZ, speed) != 0) {
		perror("MAX_SPEED_HZ");
		goto fail;
	}
	this->num_leds = num_leds;
	this->leds = malloc(sizeof(*this->leds) * num_leds);
	if (!this->leds) {
		perror("malloc");
		goto fail;
	}
	return this;
fail:
	sk9822_free(this);
	return NULL;
}

void sk9822_free(struct sk9822 *this)
{
	if (!this) {
		return;
	}
	if (this->leds) {
		free(this->leds);
	}
	if (this->fd >= 0) {
		if (!close(this->fd)) {
			perror("close");
		}
	}
	free(this);
}

static int clamp(float value)
{
	return value < 0 ? 0 : value > 1 ? 255 : (int) roundf(value * 255);
}

int sk9822_update(struct sk9822 *this)
{
	int ret = -1;
	const size_t message_size = sizeof(uint32_t) * (this->num_leds + 2 + this->num_leds / 64);
	uint8_t * const message = malloc(message_size);
	if (!message) {
		perror("malloc");
		goto fail0;
	}
	memset(message, 0, message_size);
	uint8_t *it = message + 4;
	for (const struct led *led = this->leds, *end = led + this->num_leds; led != end; ++led) {
		*it++ = 0xe0 | ((clamp(led->brightness) >> 3) & 0x1f);
		*it++ = clamp(led->colour.b);
		*it++ = clamp(led->colour.g);
		*it++ = clamp(led->colour.r);
	}
	if (write(this->fd, message, message_size) != (ssize_t) message_size) {
		perror("write");
		goto fail1;
	}
	ret = 0;
fail1:
	free(message);
fail0:
	return ret;
}
