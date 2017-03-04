#include "ark.h"

#include <sys/types.h>
#include <unistd.h>

int ark_writen(int fd, const void *v, size_t len)
{
	ssize_t n;
	size_t i;

	for (i = 0; i < len; ) {
		n = write(fd, v + i, len - i);
		if (n < 1) return 1;
		i += n;
	}
	return 0;
}

int ark_write8(int fd, uint8_t v) {
	return ark_writen(fd, &v, 1);
}

int ark_write16(int fd, uint16_t v) {
	return ark_writen(fd, &v, 2);
}

int ark_write32(int fd, uint32_t v) {
	return ark_writen(fd, &v, 4);
}

int ark_write64(int fd, uint64_t v) {
	return ark_writen(fd, &v, 8);
}

int ark_copyfd(int dst, int src)
{
	uint8_t buf[8192];
	ssize_t n;
	size_t i, len;

	for (;;) {
		n = read(src, buf, 8192);
		if (n == 0) break;
		if (n <  0) return 1;

		for (len = n, i = 0; i < len; ) {
			n = write(dst, buf + i, len - i);
			if (n < 1) return 1;
			i += n;
		}
	}

	return 0;
}

int ark_copyfdn(int dst, int src, size_t only)
{
	uint8_t buf[8192];
	ssize_t n;
	size_t i, len;

	while (only > 0) {
		n = read(src, buf, 8192 < only ? 8192 : only);
		if (n == 0) break;
		if (n <  0) return 1;
		only -= n;

		for (len = n, i = 0; i < len; ) {
			n = write(dst, buf + i, len - i);
			if (n < 1) return 1;
			i += n;
		}
	}

	return 0;
}
