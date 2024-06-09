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

struct disk;
typedef void*(*FS_OPEN_FILE)(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
typedef int (*FS_RESOLVE_FUNC)(struct disk* disk);
typedef int (*FS_READ_FILE)(struct disk* disk, void* data, uint32_t size, uint32_t count, char* out);
typedef int (*FS_CLOSE_FUNCTION)(void* private);

struct filesystem
{
    // Filesystem resolve should return zero if the disk is using the filesystem
    FS_OPEN_FILE open_file;
    FS_READ_FILE read_file;
    FS_CLOSE_FUNCTION close;
    FS_RESOLVE_FUNC resolve;

    // Name the filesystem
    char name[20];
};

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
int fclose(int fd);
void insert_filesystem(struct filesystem* fs);
struct filesystem* resolve_fs(struct disk* disk);

#endif
