#include <fcntl.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "DiskSize.h"

long get_size_of_disk(disk)
const char *disk;
{
	int fd;
	unsigned long ul;
	if ((fd = open(disk, O_RDONLY)) == -1) {
		return 0;				/* couldn't open */
	}
	if (ioctl(fd, BLKGETSIZE, &ul) == -1) {
		return 0;				/* couldn't ioctl */
	}
	close(fd);
	return ul / 2;
}
