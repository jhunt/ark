#ifndef ARK_H
#define ARK_H

#include <sys/types.h>
#include <stdint.h>

typedef struct {
	uint64_t  data_size;
	uint16_t  devno;
	uint16_t  mode;
	uint16_t  links;
	uint16_t  name_size;
	uint8_t  *path;
	uint8_t  *data;
} ark_entry;

typedef struct {
	uint8_t     version;
	uint32_t    nentries;
	ark_entry **entries;
} ark_header;

int ark_readn(int fd, void *v, size_t len);
int ark_read8(int fd, uint8_t *v);
int ark_read16(int fd, uint16_t *v);
int ark_read32(int fd, uint32_t *v);
int ark_read64(int fd, uint64_t *v);

int ark_writen(int fd, const void *v, size_t len);
int ark_write8(int fd, uint8_t v);
int ark_write16(int fd, uint16_t v);
int ark_write32(int fd, uint32_t v);
int ark_write64(int fd, uint64_t v);
int ark_copyfd(int dst, int src);
int ark_copyfdn(int dst, int src, size_t only);

int ark_pack(int outfd);
int ark_unpack(int infd);
int ark_list(int infd);

#endif
