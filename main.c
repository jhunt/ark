#include "ark.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {
	int rc;

	if (argc == 2 && strcmp(argv[1], "pack") == 0) {
		rc = ark_pack(1);

	} else if (argc == 2 && strcmp(argv[1], "unpack") == 0) {
		rc = ark_unpack(0);

	} else if (argc == 2 && strcmp(argv[1], "ls") == 0) {
		rc = ark_list(0);

	} else {
		fprintf(stderr, "usage: ark (pack|unpack|ls)\n");
		return 1;
	}

	if (rc != 0) fprintf(stderr, "failed: %s (error %d)\n", strerror(errno), errno);
	return rc;
}
