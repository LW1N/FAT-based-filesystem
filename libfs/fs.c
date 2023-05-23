#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
// Superblock struct
struct __attribute__((__packed__)) superblock {
  // 8 bytes for signature
  uint8_t signature;
  

  
  // 1 byte for number of blocks for fat
  unsigned char fatBlocks; 

};

// FAT struct
struct __attribute__((__packed__)) FAT {
  
};

// Root directory struct
struct __attribute__((__packed__)) root {
  uint8_t 
};

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
  if (!diskname || block_disk_open(diskname)) {
    return -1;
  }

  

  return 0;

}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	return block_disk_close();
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

