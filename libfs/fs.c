#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"

#define SUPERBLOCK_UNUSED_BYTES 4079
#define ENTRIES_PER_FAT_BLOCK 2048
#define SIGNATURE_BYTES 8 
#define FILENAME_SIZE 16
#define ROOT_UNUSED_BYTES 10
#define FAT_EOC 0xFFFF

/* TODO: Phase 1 */
// Struct representation of a superblock(4096 bytes)
struct __attribute__((__packed__)) superblock {
  // Signature(8 bytes)
  uint8_t signature[SIGNATURE_BYTES];
  // Total # of blocks of virtual disk(2 bytes)
  uint16_t numBlocks;
  // Root directory block index(2 bytes)
  uint16_t rootIndex;
  // Data block start index(2 bytes)
  uint16_t dataIndex;
  // Amount of data blocks(2 bytes)
  uint16_t numDataBlocks;
  // # of blocks for FAT(1 byte)
  uint8_t numFATBlocks;
  // Unused/Padding(4079 bytes)
  uint8_t padding[SUPERBLOCK_UNUSED_BYTES];
};

// Struct representation of a FAT entry
// 1 FAT block(4096 bytes) is 2048 FAT entries
struct __attribute__((__packed__)) FAT {
  // FAT entry(2 bytes)
  uint16_t entry;
};

// Struct representation of a root directory entry(32 bytes)
struct __attribute__((__packed__)) root {
  // Filename(16 bytes)
  uint8_t fileName[FILENAME_SIZE];
  // Size of the file(4 bytes)
  uint32_t size;
  // Index of the first data block(2 bytes)
  uint16_t firstIndex;
  // Unused/Padding(10 bytes)
  uint8_t padding[ROOT_UNUSED_BYTES];
};

// Struct representation of a file descriptor
struct fileDesc{
  // Unique ID number of file(actual file descriptor #)
	int ID;
  // Current position of file
	int offset;
	// Index of file in root directory
	int index;
};


// GLOBAL VARIABLES
// Pointer to superblock
static struct superblock *superB;
// Dynamic array of pointers to FAT entries
static struct FAT *fat;
// Linear array of [128]root directory entries
static struct root rootD[FS_FILE_MAX_COUNT];
// Linear array of [32]file descriptors
static struct fileDesc fds[FS_OPEN_MAX_COUNT];
// Current running # of open files
static int numOpenFiles = 0;
// Current ID #(file descriptor #) to be assigned to a file
static int currentID = 0;

// True if a file system is mounted, false otherwise
static bool FS = false;

// Mount a file system
int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
  // ERROR CHECKING
  // Check diskname validity
  if (block_disk_open(diskname)) {
    fprintf(stderr, "Can't open\n");
    return -1;
  }

  // Read superblock(First block of fs)
  // Buffer to read in superblock
  char SBBuffer[BLOCK_SIZE];
  block_read(0, SBBuffer);
  // Typecast into pointer to superblock struct for easy handling
  superB = (struct superblock*)&SBBuffer;

  // ERROR CHECKING
  // Check correct signature
  if (strncmp((char*)(superB->signature), "ECS150FS", 8)) {
	  fprintf(stderr, "Wrong signature\n");
	  return -1;
  }
  // Check correct number of blocks
  if (superB->numBlocks != block_disk_count()) {
  	fprintf(stderr, "Wrong number of blocks\n");
	  return -1;
  }
  // Check correct root index
  if (superB->rootIndex != (superB->numFATBlocks + 1)) {
	  fprintf(stderr, "Wrong root index\n");
	  return -1;
  }
  // Check correct data index
  if (superB->dataIndex != (superB->rootIndex + 1)) {
	  fprintf(stderr, "Wrong data index\n");
	  return -1;
  }

  // Read FAT(next blocks of fs)
  // Allocate memory for 1D array of fat entries
  fat = malloc(superB->numFATBlocks*ENTRIES_PER_FAT_BLOCK*sizeof(struct FAT*));
  // Read FAT block[s](total # found in superblock) into 2D buffer
  struct FAT FATBuffer[superB->numFATBlocks][ENTRIES_PER_FAT_BLOCK];
  for (int i = 1; i < superB->numFATBlocks + 1; i++) {
	  block_read(i, FATBuffer[i-1]);
  }
  // Set FAT entries to data retrieved from buffer(convert 2D array -> 1D array)
  for (int i = 0; i < superB->numFATBlocks; i++) {
    for (int j = 0; j < ENTRIES_PER_FAT_BLOCK; j++) {
      // fat[i*ENTRIES_PER_FAT_BLOCK + j] = malloc(sizeof(struct FAT *));
      fat[i*ENTRIES_PER_FAT_BLOCK + j] = FATBuffer[i][j];
    }
  }

  // ERROR CHECKING
  // First entry of FAT should always be invalid
  if (fat[0].entry != FAT_EOC) {
    fprintf(stderr, "First FAT entry not invalid\n");
	  return -1;
  }

  // Read root directory(next block of fs, right before data blocks)
  block_read(superB->rootIndex, rootD);

  // Initialize array of file descriptors(fds)
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    fds[i].ID = -1;
    fds[i].offset = 0;
    fds[i].index = -1;
  }

  // Assert FS as true, when filesystem is fully mounted
  FS = true;
  return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
  // ERROR CHECKING
  // Check if there's a file system currently mounted & if any open FDs
  if (!FS || numOpenFiles){
    return -1;
  }

	// Write Superblock, FAT, & Root Directory meta-info back to disk
	block_write(0, superB);
  for (int i = 1; i < superB->numFATBlocks + 1; i++) {
    block_write(i, (fat + ENTRIES_PER_FAT_BLOCK*(i - 1)));
  }
	block_write(superB->rootIndex, rootD);

  free(fat);

	// If no disk is currently open, return -1
	if (block_disk_close()) {
		return -1;
	}
  // Filesystem successfully unmounted
  FS = false;
	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
	// Return -1 if no underlying virtual disk was opened
	if (!FS) {
		return -1;
	}

  // Retrieve number of empty data blocks & rootD entries
  int FATFree = 0;
  int rootDFree = 0;
  for (int i = 0; i < superB->numDataBlocks; i++) {
    if(fat[i].entry == 0){
      FATFree++;
    }
  }

  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if(rootD[i].fileName[0] == 0){
      rootDFree++;
    }
  }

	// Display fs info
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", superB->numBlocks);
	printf("fat_blk_count=%d\n", superB->numFATBlocks);
	printf("rdir_blk=%d\n", superB->rootIndex);
	printf("data_blk=%d\n", superB->dataIndex);
	printf("data_blk_count=%d\n", superB->numDataBlocks);
  printf("fat_free_ratio=%d/%d\n", FATFree, superB->numDataBlocks);
  printf("rdir_free_ratio=%d/%d\n", rootDFree, FS_FILE_MAX_COUNT);
	return 0;
}

// HELPER FUNCTION - finds free FAT block
int find_freeFAT(){
  for (int i = superB->dataIndex; i < superB->numDataBlocks; i++) {
    if(fat[i].entry == 0){
      return i;
    }
  }

  // In case of no room
  return -1;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
  // ERROR CHECKING
  // No filesystem mounted, or null filename
  if (!FS || !filename) {
		return -1;
	}
  // Length of filename too long
  if (strlen(filename) > FS_FILENAME_LEN) {
    return -1;
  }

  // Check if filename already exists
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if (!strncmp((char*)rootD[i].fileName, filename, strlen(filename))) {
      return -1;
    }
  }

  // Find empty entry in root directory
  int foundI = -1;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    // If entry is empty, grab index
    if((char)rootD[i].fileName[0] == '\0'){
      foundI = i;
      break;
    }
  }

  // If index was never found, root directory is full
  if (foundI == -1) {
    return foundI;
  }
  
  // Else, set found empty entry to new filename
  strcpy((char*)rootD[foundI].fileName, filename);
  rootD[foundI].size = 0;
  rootD[foundI].firstIndex = FAT_EOC;
  // fat[rootD[foundI].firstIndex].entry = FAT_EOC;
  return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
  // ERROR CHECKING
  // No filesystem mounted, or NULL filename
  if (!FS || !filename) {
		return -1;
	}

  // Find file in root directory
  int foundI = -1;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if(!strncmp((char*)rootD[i].fileName, filename, strlen(filename))) {
      foundI = i;
      break;
    }
  }

  // If file isn't found, return -1
  if (foundI == -1) {
    return -1;
  }

  // Else, reset name and empty FAT data blocks
  rootD[foundI].fileName[0] = '\0';
  // Iterate through FAT data blocks, stop at beginning of next file
  uint16_t ind = rootD[foundI].firstIndex;
  while(ind != FAT_EOC) {
    uint16_t ind2 = fat[ind].entry;
    fat[ind].entry = 0;
    ind = ind2;
  }
  
  return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
  // ERROR CHECKING
  // No filesystem mounted
  if (!FS) {
    return -1;
  }

  // Find and list non-empty files in the root directory
  printf("FS Ls:\n");
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if (rootD[i].fileName[0] != '\0') {
      printf("file: %s, size: %d, data_blk: %d\n", rootD[i].fileName, 
      rootD[i].size, rootD[i].firstIndex);
    }
  }
  return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
  // ERROR CHECKING
  // No filesystem mounted
  if (!FS) {
    return -1;
  }
  // FS_FILE_MAX_COUNT amount of open files already
  if (numOpenFiles == FS_FILE_MAX_COUNT) {
    return -1;
  }

  // Find the file in the root directory
  int foundI = -1;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if (!strncmp((char*)rootD[i].fileName, filename, strlen(filename))) {
      foundI = i;
      break;
    }
  }

  // If file isn't found, return -1
  if (foundI == -1) {
    return -1;
  }

  // Find empty file descriptor entry
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    if (fds[i].ID == -1) {
      fds[i].ID = currentID;
      fds[i].index = i;
      fds[i].offset = 0;
      currentID++;
      numOpenFiles++;
      return fds[i].ID;
    }
  }

  // If file descriptor array is full
  return -1;
}

// HELPER FUNCTION - finds index of fds given fds.ID
int find_fdsIndex(int fd) {
  // ERROR CHECKING
  // Invalid fd(out of bounds or not currently open)
  if (fd > currentID || fd < 0) {
    return -1;
  }

  // Find file in fds
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    // If found, return index of file in fds array
    if (fds[i].ID == fd) {
      return i;
    }
  }

  // If given fd is not in array of file descriptors
  return -1;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
  // ERROR CHECKING
  // No filesystem mounted
  if (!FS) {
    return -1;
  }
  // Invalid fd(not currently open or out of bounds)
  if (fd < 0 || fd > currentID) {
    return -1;
  }

  // Find given FD in fds
  int ind = find_fdsIndex(fd);

  // If given fd is not in array of file descriptors
  if (ind == -1) {
    return -1;
  }

  // If found, reset FD values in fds
  fds[ind].ID = -1;
  fds[ind].offset = 0;
  fds[ind].index = -1;
  numOpenFiles--;
  return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
  // ERROR CHECKING
  // No filesystem mounted
  if (!FS) {
    return -1;
  }
  // Invalid fd(not currently open or out of bounds)
  if (fd < 0 || fd > currentID) {
    return -1;
  }

  // Find given FD in fds
  int ind = find_fdsIndex(fd);

  // If given fd is not in array of file descriptors
  if (ind == -1) {
    return -1;
  }

  // If found, grab and return size of file pointed to by FD
  return rootD[fds[ind].index].size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
  // Grab size of file using fs_stat function
  int fsize = fs_stat(fd);

  // ERROR CHECKING
  // fs_stat fails or given offset > than actual file size
  if (fsize == -1 || fsize < (int)offset) {
    return -1;
  }
  
  // Find file in fds
  int ind = find_fdsIndex(fd);

  // If given fd is not in array of file descriptors
  if (ind == -1) {
    return -1;
  }

  // If found, set offset of file to given offset
  fds[ind].offset = offset;
  return 0;
}

// HELPER FUNCTION - finds index of data block indicated by offset of fd
int find_DBIndex(int fd) {
  // Grab index of fds from fds.ID
  int fdIndex = find_fdsIndex(fd);
  // Grab index of file in root directory
  int rootDIndex = fds[fdIndex].index;
  // Grab index of first data block
  uint16_t DBIndex = rootD[rootDIndex].firstIndex;
  // Grab file offset
  int offset = fds[fd].offset;

  // If empty file, return -1
  if (DBIndex == FAT_EOC) {
    return -1;
  }

  // Iterate through file's datablocks until datablock containing offset of file
  while (offset >= BLOCK_SIZE) {
    // Grab next data block of file
    DBIndex = fat[DBIndex].entry;
    // If next index wasn't allocated, out of bounds of file
    if (DBIndex == FAT_EOC) {
      return -1;
    }
    // Subtract BLOCK_SIZE after entering a new data block
    offset = offset - BLOCK_SIZE;
  }

  // Actual index of data block containing offset of file
  // Need to account for actual data block start index from superblock
  return DBIndex; // + superB->dataIndex;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
  // ERROR CHECKING
  // No filesystem mounted or buf is NULL
  if (!FS || !buf) {
    return -1;
  }
  
  // Grab index of file in fds
  int fdIndex = find_fdsIndex(fd);

  // Invalid fd or file wasn't found
  if (fdIndex == -1) {
    return -1;
  }

  // Index of file in root directory
  int rootDIndex = fds[fdIndex].index;
  // Offset of buffer holding stuff to write
  int bufferOffset = 0;
  // Remaing # of bytes to write
  size_t remainBytes = count;
  // Left & right offsets, # of bytes written
  size_t lOffset, rOffset, writtenBytes;

  int DBIndex = -1;

  // Loop until no more bytes to write
  int ind = 0;
  while (remainBytes != 0) {
    lOffset = 0;
    // Left offset != 0 only during first block read
    if (ind == 0) {
      lOffset = fds[fdIndex].offset % BLOCK_SIZE;
    }

    rOffset = 0;
    // Right offset != 0 if don't need to read another block
    if (remainBytes + lOffset < BLOCK_SIZE) {
      rOffset = BLOCK_SIZE - remainBytes - lOffset;
    }

    DBIndex = find_DBIndex(fd);

    // Bounce buffer to read entire data block into
    void *bounceBuffer = malloc(BLOCK_SIZE);
    // If can't find data block & no free FAT blocks
    if (DBIndex == -1 && find_freeFAT() == -1) {
      rootD[rootDIndex].size += count - remainBytes;
      return count - remainBytes;
    }
    // If can't find data block & free FAT blocks, allocate new data block
    if (DBIndex == -1) {
      // Check for free FAT blocks
      int newIndex = find_freeFAT();
      // Grab index of first data block of file
      uint16_t FATIndex = rootD[rootDIndex].firstIndex;
      // If empty file size = 0,(first DBBlock == FAT_EOC), set first DBindex of file
      if (FATIndex == FAT_EOC && newIndex != -1 && rootD[rootDIndex].size == 0) {
        // index of first data block equal to newIndex
        // i.e. 0xFFFF 0 0 3 0 0 0 0 0 0 ... 
        rootD[rootDIndex].firstIndex = newIndex;
        // fat[65535] = 3
        FATIndex = newIndex;
        // fat[3] = 0xFFFF
        fat[newIndex].entry = FAT_EOC;
      }
      // Iterate through FAT until entry that points to index of FAT_EOC
      while (fat[FATIndex].entry != FAT_EOC) {
        FATIndex = fat[FATIndex].entry;
      }
      // FATIndex is now index of FAT_EOC
      // If available FAT block(s), expand FAT link by 1 to found free FAT block
      if (fat[FATIndex].entry != newIndex && newIndex != -1) {
        fat[FATIndex].entry = newIndex;
        fat[fat[FATIndex].entry].entry = FAT_EOC;
      }
    }
    // If there is a data block, read DB into bounceBuffer
    if (DBIndex != -1) {
      block_read(DBIndex, bounceBuffer);
    }

    // Write into bounceBuffer, keep offsets in mind
    writtenBytes = BLOCK_SIZE - lOffset - rOffset;
    memcpy((char*)bounceBuffer+lOffset, (char*)buf+bufferOffset, writtenBytes);

    DBIndex = find_DBIndex(fd);
    // Write back to disk
    block_write(DBIndex, bounceBuffer);

    // Update variables & free allocated mem for bounce buffer each iteration
    fds[fd].offset += writtenBytes;
    bufferOffset += writtenBytes;
    remainBytes -= writtenBytes;
    free(bounceBuffer);
    bounceBuffer = NULL;
    ind++;
  }

  rootD[rootDIndex].size += count - remainBytes;
  return count - remainBytes;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
  // ERROR CHECKING
  // No filesystem mounted or buf is NULL
  if (!FS || !buf) {
    return -1;
  }

  // Grab index of file in fds
  int fdIndex = find_fdsIndex(fd);

  // Invalid fd or file wasn't found
  if (fdIndex == -1) {
    return -1;
  }

  // Read buffer offset
  int bufferOffset = 0;
  // Remaing # of bytes to read
  size_t remainBytes = count;
  // Left & right offsets, # of bytes read
  size_t lOffset, rOffset, readBytes;

  // Loop until no more bytes left to read
  int ind = 0;
  while (remainBytes != 0) {
    lOffset = 0;
    if (ind == 0) {
      lOffset = fds[fdIndex].offset % BLOCK_SIZE;
    }

    rOffset = 0;
    if (remainBytes + lOffset < BLOCK_SIZE) {
      rOffset = BLOCK_SIZE - remainBytes - lOffset;
    }

    // Read data block into bounceBuffer
    void *bounceBuffer = malloc(BLOCK_SIZE);
    if (block_read(find_DBIndex(fd), bounceBuffer) == -1) {
      fprintf(stderr, "Block reading ERROR\n");
      return -1;
    }

    // Copy bounceBuffer into read buffer, keep offsets in mind
    readBytes = BLOCK_SIZE - lOffset - rOffset;
    memcpy((char*)buf+bufferOffset, (char*)bounceBuffer+lOffset, readBytes);

    // Update variables
    fds[fd].offset += readBytes;
    bufferOffset += readBytes;
    remainBytes -= readBytes;
    free(bounceBuffer);
    ind++;
  }

  return count - remainBytes;
}

