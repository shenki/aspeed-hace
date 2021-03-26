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


static const uint8_t test_input_deadbeef[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };

#define RAM_BASE 0x90000000
#define RAM_SIZE 0x08000000
#define DEST_BUF (RAM_BASE + RAM_SIZE - 64)

int main(int argc, char **argv)
{
	int i;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	int file_fd;
	struct stat st;
	uint8_t *buf;
	const void *test_input;
	size_t length;

	if (argc == 2) {
		printf("Using file '%s' as test vector\n", argv[1]);
		file_fd = open(argv[1], O_RDONLY);
		if (file_fd == -1)
			errx(1, "opening '%s' failed", argv[1]);
		if (fstat(file_fd, &st) == -1)
			errx(1, "fstat failed");
		test_input = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
		if (test_input == MAP_FAILED)
			errx(1, "mmap of file failed");
		length = st.st_size;
	} else {
		printf("Using '0xDEADBEEF' as test vector\n");
		test_input = test_input_deadbeef;
		length = sizeof(test_input_deadbeef);
	}

	// map the io registers
	map = mmap(NULL, ASPEED_IO_SZ,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, ASPEED_IO_BASE);
	if (map == MAP_FAILED)
		errx(1, "io registers map failed");

	// do bad things to dram
	buf = mmap(NULL, RAM_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, RAM_BASE);
	if (buf == MAP_FAILED)
		errx(1, "ram map failed");
	printf("copying %d bytes from %p to 0x%08x\n", length, test_input, buf);
	memcpy(buf, test_input, length);

	hw_sha512((void *)RAM_BASE, length, (void *)DEST_BUF, 1024);

	for (i = 0; i < 64; i++) {
		printf("%02x", buf[RAM_SIZE - 64 +i]);
	}
	puts("\n");

	return 0;
}
