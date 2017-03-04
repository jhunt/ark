#include "ark.h"

#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
	char mode[11];
	char sizedev[21];
	char *path;
} lentry;

static int ark_list1(int infd, lentry *e)
{
	int rc;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

	rc = ark_read64(infd, &u64);
	if (rc != 0) return 1;

	rc = ark_read32(infd, &u32);
	if (rc != 0) return 1;

	rc = ark_read16(infd, &u16);
	if (rc != 0) return 1;

	switch (u16 & S_IFMT) {
	case S_IFBLK:
	case S_IFCHR: rc = snprintf(e->sizedev, 21, "%d, %d", major(u32), minor(u32)); break;
	default:      rc = snprintf(e->sizedev, 21, "%llu", u64);                      break;
	}
	if (rc < 0) return 1;

	memset(e->mode, '-', 10);

	switch (u16 & S_IFMT) {
	case S_IFIFO:  e->mode[0] = 'p'; break;
	case S_IFCHR:  e->mode[0] = 'c'; break;
	case S_IFDIR:  e->mode[0] = 'd'; break;
	case S_IFBLK:  e->mode[0] = 'b'; break;
	case S_IFREG:  e->mode[0] = '-'; break;
	case S_IFLNK:  e->mode[0] = 'l'; break;
	case S_IFSOCK: e->mode[0] = 's'; break;
	default:       e->mode[0] = '?';
	}

	if (u16 & 0400) e->mode[1] = 'r';
	if (u16 & 0200) e->mode[2] = 'w';
	if (u16 & 0100) e->mode[3] = 'x';

	if (u16 & 0040) e->mode[4] = 'r';
	if (u16 & 0020) e->mode[5] = 'w';
	if (u16 & 0001) e->mode[6] = 'x';

	if (u16 & 0004) e->mode[7] = 'r';
	if (u16 & 0002) e->mode[8] = 'w';
	if (u16 & 0001) e->mode[9] = 'x';

	e->mode[10] = '\0';

	rc = ark_read16(infd, &u16);
	if (rc != 0) return 1;

	e->path = malloc(u16+1);
	if (!e->path) return 1;

	rc = ark_readn(infd, e->path, u16);
	if (rc != 0) {
		free(e->path);
		return 1;
	}
	e->path[u16] = '\0';

	lseek(infd, u64, SEEK_CUR);
	return 0;
}

int ark_list(int infd)
{
	char magic[4];
	int rc, w;
	uint32_t i, n;
	lentry *entries;

	rc = ark_readn(infd, magic, 4);
	if (rc != 0) return 1;
	if (memcmp(magic, "ARK\0", 4) != 0) return 1;

	rc = ark_read32(infd, &n);
	if (rc != 0) return 1;
	fprintf(stdout, "%d archive entries\n", n);

	entries = calloc(n, sizeof(lentry));
	if (!entries) return 1;

	w = 0;
	for (i = 0; i < n; i++) {
		rc = ark_list1(infd, &entries[i]);
		if (rc != 0) {
			free(entries);
			return 1;
		}
		if (strlen(entries[i].sizedev) > w) {
			w = strlen(entries[i].sizedev);
		}
	}

	for (i = 0; i < n; i++) {
		fprintf(stdout, "%s  %*s  %s\n",
			entries[i].mode, w, entries[i].sizedev, entries[i].path);
		free(entries[i].path);
	}
	free(entries);

	return 0;
}
