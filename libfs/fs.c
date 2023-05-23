#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SUPERBLOCK_UNUSED_BYTES 4079
#define ENTRIES_PER_FAT_BLOCK 2048
#define SIGNATURE_BYTES 8 
#define FILENAME_SIZE 16
#define ROOT_UNUSED_BYTES 16
#define FAT_EOC 0xFFFF

/* TODO: Phase 1 */
// Superblock
struct __attribute__((__packed__)) superblock {
  // 8 bytes for signature
  uint64_t signature;
  // 2 bytes for amount of blocks of virtual disk
  uint16_t numBlocks;
  // 2 bytes for root directory block index
  uint16_t rootIndex;
  // 2 bytes for data block start index
  uint16_t dataIndex;
  // 2 bytes for amount of data blocks
  uint16_t numDataBlocks;
  // 1 byte for number of blocks for fat
  uint8_t numFATBlocks;
  // 4079 bytes unused/padding
  uint8_t padding[SUPERBLOCK_UNUSED_BYTES];
};

// FAT
struct __attribute__((__packed__)) FAT {
  // 2 bytes for each entry and 2048 entries per FAT block
  uint16_t entry[ENTRIES_PER_FAT_BLOCK];
};

// Root directory
struct __attribute__((__packed__)) root {
  // 16 bytes for filename
  uint8_t name[FILENAME_SIZE];
  // 4 bytes for size of the file
  uint32_t size;
  // 2 bytes for index of the first data block
  uint16_t firstIndex;
  // 10 bytes unused/padding
  uint8_t padding[ROOT_UNUSED_BYTES];
};

// File Descriptor
struct FD{
	int ID;
	int offset;
	// Index of file in root directory
	int index;
};


// Global variables
static struct superblock *superB;
static struct FAT *fat;
static struct root rootD[FS_FILE_MAX_COUNT];

static struct FD fds[FS_OPEN_MAX_COUNT];
static int numOpenFiles = 0;


int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
  if (!diskname || block_disk_open(diskname)) {
    return -1;
  }

  // Read superblock(First block of fs)
  superB = malloc(BLOCK_SIZE);
  block_read(0, superB);

  // ERROR CHECKING
  // Check for correct signature
  if (strncmp((char*)(superB->signature), "ECS150FS", 8)) {
	  fprintf(stderr, "Wrong signature\n");
	  return -1;
  }
  // Check correct number of blocks
  if (superB->numBlocks != block_disk_count()) {
  	fprintf(stderr, "Wrong number of blocks\n");
	  return -1;
  }
  // Check if correct root index
  if (superB->rootIndex != (superB->numFATBlocks + 1)) {
	  fprintf(stderr, "Wrong root index\n");
	  return -1;
  }
  // Check for correct data index
  if (superB->dataIndex != (superB->rootIndex + 1)) {
	  fprintf(stderr, "Wrong data index\n");
	  return -1;
  }

  // Read FAT
  fat = malloc((superB->numFATBlocks)*BLOCK_SIZE);
  for (int i = 0; i <= superB->numFATBlocks; i++) {
	  block_read(i, (char*)fat + BLOCK_SIZE*(i - 1));
  }

  // ERROR CHECKING
  // First entry of FAT is always invalid
  if (fat->entry[0] != FAT_EOC) {
	  return -1;
  }

  // Read root directory
  block_read(superB->rootIndex, rootD);

  // Initialize fds
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    fds[i].ID = -1;
    fds[i].offset = 0;
    fds[i].index = -1;
  }

  return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	// Write meta-info back to disk
	// Superblock
	block_write(0, superB);
	// FAT
	for (int i = 0; i <= superB->numFATBlocks; i++){
		block_write(i, (char*)fat + BLOCK_SIZE*(i - 1));
	}
	// Root Directory
	block_write(superB->rootIndex, rootD);

	// If closing fails
	if(block_disk_close()){
		return -1;
	}

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
	// Return -1 if no underlying virtual disk was opened
	if (block_disk_count() == -1){
		return -1;
	}

	// Display fs info
	printf("Filesystem info\n");
	printf("Total Block Count = %d\n", superB->numBlocks);
	printf("FAT Block Count = %d\n", superB->numFATBlocks);
	printf("Root Directory Block = %d\n", superB->rootIndex);
	printf("Data Block = %d\n", superB->dataIndex);
	printf("Total Number of Data Blocks = %d\n", superB->numDataBlocks);
	
	return 0;
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

