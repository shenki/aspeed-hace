#include <err.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HACE_BASE 0x1E6D0000

void hw_sha1(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha256(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha512(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);

static void *map;

uint32_t readl(uint32_t addr)
{
	if ((addr & 0xFFFF0000) != HACE_BASE) {
		printf("Ingoring read at %08x\n", addr);
		return 0;
	}

	return *(uint32_t *)(map + (addr & 0xFFFF));
}

void writel(uint32_t val, uint32_t addr)
{
	if ((addr & 0xFFFF0000) != HACE_BASE) {
		printf("Ingoring write at %08x=%08x\n", addr, val);
		return;
	}

	*(uint32_t *)(map + (addr & 0xFFFF)) = val;
}


static const uint8_t test_input[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };
static uint8_t test_output[64];

int main()
{
	int i;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	uint8_t *input;

	// map the hace registers
	map = mmap(NULL, 0x1000,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, HACE_BASE);
	if (map == MAP_FAILED)
		errx(1, "map failed");

	// do bad things to dram
	input = mmap(NULL, 0x2000,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, 0x80000000);
	if (input == MAP_FAILED)
		errx(1, "map failed");
	memcpy(input, test_input, sizeof(test_input));

	hw_sha512(0x80000000, sizeof(test_input), 0x80001000, 1024);

	for (i = 0; i < 64; i++) {
		printf("%02x", input[0x1000+i]);
	}
	puts("\n");

	return 0;
}
