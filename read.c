#include "ark.h"

#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
int ark_readn(int fd, void *v, size_t len)
{
	ssize_t n;
	size_t i;

	for (i = 0; i < len; ) {
		n = read(fd, v + i, len - i);
		if (n == 0) { fprintf(stderr, "read EOF\n"); return 1; }
		if (n < 1) return 1;
		i += n;
	}
	return 0;
}

int ark_read8(int fd, uint8_t *v) {
	return ark_readn(fd, v, 1);
}

int ark_read16(int fd, uint16_t *v) {
	return ark_readn(fd, v, 2);
}

int ark_read32(int fd, uint32_t *v) {
	return ark_readn(fd, v, 4);
}

int ark_read64(int fd, uint64_t *v) {
	return ark_readn(fd, v, 8);
}
