#include "ark.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static int ark_pack_file(int outfd, const char *path);
static int ark_pack_dir(int outfd, const char *path);

static int ark_count_entries(const char *path)
{
	DIR *dh;
	struct dirent *ent;
	struct stat st;
	char *file;
	int rc, n;

	dh = opendir(path);
	if (!dh) return -1;

	n = 0;
	while ((ent = readdir(dh)) != NULL) {
		if (strcmp(ent->d_name, ".")  == 0
		 || strcmp(ent->d_name, "..") == 0) continue;

		rc = asprintf(&file, "%s/%s", path, ent->d_name);
		if (rc < 0) {
			closedir(dh);
			return -1;
		}

		rc = lstat(file, &st);
		if (rc != 0) {
			free(file);
			closedir(dh);
			return -1;
		}

		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			rc = ark_count_entries(file);
			if (rc < 0) {
				free(file);
				closedir(dh);
				return rc;
			}
			n += rc;
		} else {
			free(file);
			n++;
		}
	}
	return n;
}

static int ark_pack_file(int outfd, const char *path)
{
	struct stat st;
	int n, rc, fd = -1;
	char buf[PATH_MAX];

	rc = lstat(path, &st);
	if (rc != 0) goto done;

	/* for directories, immediately co-recurse back to ark_pack_dir */
	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		return ark_pack_dir(outfd, path);
	}

	/* for regular files, we need a file descriptor to copy from */
	if ((st.st_mode & S_IFMT) == S_IFREG) {
		rc = open(path, O_RDONLY);
		if (rc < 0) goto done;
		fd = rc;
	}

	/* for symlinks, read the contents of the link into buf[] */
	if ((st.st_mode & S_IFMT) == S_IFLNK) {
		rc = readlink(path, buf, PATH_MAX);
		if (rc == -1) goto done;
		n = rc;
	}

	/* amount of data; for regular files, this is file size.
	                   for symbolic links, this is length of target path
	                   for everyone else, this is zero. */
	switch (st.st_mode & S_IFMT) {
	case S_IFREG:  rc = ark_write64(outfd, st.st_size); break;
	case S_IFLNK:  rc = ark_write64(outfd, n);          break;
	default:       rc = ark_write64(outfd, 0);          break;
	}
	if (rc != 0) goto done;

	rc = ark_write32(outfd, st.st_rdev);
	if (rc != 0) goto done;

	rc = ark_write16(outfd, st.st_mode);
	if (rc != 0) goto done;

	// FIXME: check length of path ! >2^16-1
	rc = ark_write16(outfd, strlen(path));
	if (rc != 0) goto done;

	// FIXME: strlen called twice...
	rc = ark_writen(outfd, path, strlen(path));
	if (rc != 0) goto done;

	if ((st.st_mode & S_IFMT) == S_IFREG) {
		rc = ark_copyfd(outfd, fd);
		if (rc != 0) goto done;

	} else if ((st.st_mode & S_IFMT) == S_IFLNK) {
		rc = ark_writen(outfd, buf, n);
		if (rc != 0) goto done;
	}

done:
	if (fd >= 0) close(fd);
	return rc;
}

static int ark_pack_dir(int outfd, const char *path)
{
	DIR *dh;
	struct dirent *ent;
	char *file;
	int rc;

	dh = opendir(path);
	if (!dh) return 1;

	while ((ent = readdir(dh)) != NULL) {
		if (strcmp(ent->d_name, ".")  == 0
		 || strcmp(ent->d_name, "..") == 0) continue;

		rc = asprintf(&file, "%s/%s", path, ent->d_name);
		if (rc < 0) {
			closedir(dh);
			return 1;
		}

		rc = ark_pack_file(outfd, file);
		free(file);
		if (rc != 0) {
			closedir(dh);
			return 1;
		}
	}
	return 0;
}

int ark_pack(int outfd)
{
	int rc, n;

	rc = ark_writen(outfd, "ARK\0", 4);
	if (rc != 0) return rc;

	n = ark_count_entries(".");
	if (n < 0) return n;
	rc = ark_write32(outfd, n);
	if (rc != 0) return rc;

	return ark_pack_dir(outfd, ".");
}
