#include <err.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ASPEED_IO_BASE 0x10000000
#define ASPEED_IO_SZ   0x0f000000

void hw_sha1(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha256(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha512(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);

static void *map;

uint32_t readl(uint32_t addr)
{
	return *(uint32_t *)(map + (addr & 0x0FFFFFFF));
}

void writel(uint32_t val, uint32_t addr)
{
	*(uint32_t *)(map + (addr & 0x0FFFFFFF)) = val;
}


static const uint8_t test_input[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };

int main()
{
	int i;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	uint8_t *buf;

	// map the io registers
	map = mmap(NULL, ASPEED_IO_SZ,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, ASPEED_IO_BASE);
	if (map == MAP_FAILED)
		errx(1, "map failed");

	// do bad things to dram
	buf = mmap(NULL, 0x2000,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, 0x80000000);
	if (buf == MAP_FAILED)
		errx(1, "map failed");
	memcpy(buf, test_input, sizeof(test_input));

	hw_sha512((void *)0x80000000, sizeof(test_input), (void *)0x80001000, 1024);

	for (i = 0; i < 64; i++) {
		printf("%02x", buf[0x1000+i]);
	}
	puts("\n");

	return 0;
}
