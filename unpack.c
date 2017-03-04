#include "ark.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static int mkdir_p(const char *dir)
{
	int rc;
	char *copy, *p;
	struct stat st;

	if (lstat(dir, &st) == 0) return 0;

	p = copy = strdup(dir);
	p = strchr(p, '/');
	while (p) {
		*p = '\0';
		if (lstat(copy, &st) != 0) {
			if (errno != ENOENT) return 1;

			rc = mkdir(copy, 0777);
			if (rc != 0) return 1;
		}
		*p++ = '/';
		p = strchr(p, '/');
	}

	if (lstat(copy, &st) != 0) {
			if (errno != ENOENT) return 1;

		rc = mkdir(copy, 0777);
		if (rc != 0) return 1;
	}

	return 0;
}

static int ark_unpack_file(int infd)
{
	int rc, fd;
	uint16_t mode, plen;
	uint32_t devno;
	uint64_t dlen;
	char *path, *p;
	char *target; /* only for symlink(2) */

	rc = ark_read64(infd, &dlen);
	if (rc != 0) return 1;

	rc = ark_read32(infd, &devno);
	if (rc != 0) return 1;

	rc = ark_read16(infd, &mode);
	if (rc != 0) return 1;

	rc = ark_read16(infd, &plen);
	if (rc != 0) return 1;

	path = malloc(plen + 1);
	if (!path) return 1;

	rc = ark_readn(infd, path, plen);
	if (rc != 0) {
		free(path);
		return 1;
	}
	path[plen] = '\0';

	p = strrchr(path, '/');
	if (p) {
		*p = '\0';
		rc = mkdir_p(path);
		if (rc != 0) {
			free(path);
			return 1;
		}
		*p = '/';
	}

	switch (mode & S_IFMT) {
	case S_IFIFO:
		rc = mkfifo(path, mode);
		if (rc != 0) {
			free(path);
			return 1;
		}
		break;

	case S_IFCHR:
	case S_IFBLK:
		rc = mknod(path, mode, devno);
		if (rc != 0) {
			free(path);
			return 1;
		}
		break;

	case S_IFDIR:
		rc = mkdir_p(path);
		if (rc != 0) {
			free(path);
			return 1;
		}
		break;

	case S_IFREG:
		fd = open(path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (fd < 0) {
			free(path);
			return 1;
		}

		rc = ark_copyfdn(fd, infd, dlen);
		close(fd);
		if (rc != 0) {
			free(path);
			return 1;
		}
		break;

	case S_IFLNK:
		target = malloc(dlen + 1);
		if (!target) {
			free(path);
			return 1;
		}
		rc = ark_readn(infd, target, dlen);
		if (rc != 0) {
			free(target);
			free(path);
			return 1;
		}
		target[dlen] = '\0';

		rc = symlink(target, path);
		free(target);
		if (rc != 0) {
			free(path);
			return 1;
		}
		break;

	case S_IFSOCK: fprintf(stderr, "socket: not implemented\n"); return 1;
	}

	if ((mode & S_IFMT) != S_IFLNK) {
		/* FIXME: umask is still set */
		rc = chmod(path, mode & ~S_IFMT);
		if (rc != 0) {
			free(path);
			return 1;
		}
	}

	free(path);
	return 0;
}

int ark_unpack(int infd)
{
	char magic[4];
	int rc;
	uint32_t n;

	rc = ark_readn(infd, magic, 4);
	if (rc != 0) return 1;
	if (memcmp(magic, "ARK\0", 4) != 0) return 1;

	rc = ark_read32(infd, &n);
	if (rc != 0) return 1;

	n--;
	while (n-- > 0) {
		rc = ark_unpack_file(infd);
		if (rc != 0) return 1;
	}
	return 0;
}
