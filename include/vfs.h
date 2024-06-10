#ifndef VFS_H
#define VFS_H

#include "path.h"
#include "types.h"

typedef unsigned int FILE_SEEK_MODE;
enum
{
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END
};

typedef unsigned int FILE_OPEN_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

enum
{
    FILE_STAT_READ_ONLY = 0b00000001
};

typedef unsigned int FILE_STAT_FLAGS;

struct disk;
typedef void*(*FS_OPEN_FILE)(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
typedef int (*FS_RESOLVE_FUNC)(struct disk* disk);
typedef int (*FS_READ_FILE)(struct disk* disk, void* data, uint32_t size, uint32_t count, char* out);
typedef int (*FS_CLOSE_FUNCTION)(void* private);

struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

typedef int (*FS_STAT_FUNCTION)(struct disk* disk, void* private, struct file_stat* stat);

struct filesystem
{
    // Filesystem resolve should return zero if the disk is using the filesystem
    FS_OPEN_FILE open_file;
    FS_READ_FILE read_file;
    FS_CLOSE_FUNCTION close;
    FS_RESOLVE_FUNC resolve;
    FS_STAT_FUNCTION stat;

    // Name the filesystem
    char name[20];
};

/**
 * File descriptor structure
 * @param index The file descriptor index
 * @param fs The filesystem that the file descriptor is using
 * @param data The private data for the file descriptor(points to the filesystem's file descriptor, which contains fat item and r/w pointer location)
 * @param disk The disk that the file descriptor is using
 */
struct file_descriptor
{
    // File descriptor index
    int index;
    struct filesystem* fs;
    
    // Private data for internal file descriptor
    void* data;

    // The disk that the fd is using
    struct disk* disk;
};

void init_fs();
int fopen(const char* filename, const char* mode);
int fread(void* ptr, uint32_t size, uint32_t count, int fd);
int fstat(int fd, struct file_stat* stat);

int fclose(int fd);
void insert_filesystem(struct filesystem* fs);
struct filesystem* resolve_fs(struct disk* disk);

#endif
