#include "vfs.h"
#include "config.h"
#include "mm.h"
#include "print.h"
#include "string.h"
#include "fat16.h"
//#include "types.h"
#include "disk.h"
#include "errno.h"

struct filesystem* filesystems[MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[MAX_FILE_DESCRIPTORS];

/**
 * @brief Initialize the filesystem
 * @return pointer to the free filesystem; otherwise 0
 */
static struct filesystem** get_free_filesystem()
{
    int i = 0;
    for (i = 0; i < MAX_FILESYSTEMS; i++)
    {
        // if the filesystem is not in use(The pointer is null)
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

void insert_filesystem(struct filesystem* fs)
{
    struct filesystem** fs_ptr = get_free_filesystem();
    if (fs_ptr == 0)
    {
        print("No more filesystems available\n");
        while(1){}
    }

    *fs_ptr = fs;
}

/**
 * @brief Load the filesystem
 */
static void load_fs()
{
    memset(filesystems, 0, sizeof(filesystems));
    insert_filesystem(init_fat16());
}

/**
 * @brief Init the filesystem
 */
void init_fs()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    load_fs();
}

/**
 * @brief Get the new file descriptor for fd
 * @param fd: The double pointer to the file descriptor
 * @return 0 if success, otherwise -1
 */
static int get_new_file_descriptor(struct file_descriptor** fd)
{
    int i = 0;
    for (i = 0; i < MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            *fd = kmalloc(sizeof(struct file_descriptor));
            (*fd)->index = i + 1;
            file_descriptors[i] = *fd;
            
            return 0;
        }
    }

    return -1;
}

static struct file_descriptor* get_file_descriptor(int fd)
{
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    // Descriptor is 1 based
    return file_descriptors[fd - 1];
}

struct filesystem* resolve_fs(struct disk* disk)
{
    int i = 0;
    for (i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != 0)
        {
            if ((*filesystems[i]->resolve)(disk) == 0)
            {
                return filesystems[i];
            }
        }
    }

    return 0;
}

FILE_OPEN_MODE get_file_open_mode(const char* mode)
{
    if(strcmp(mode, "r") == 0)
    {
        return FILE_MODE_READ;
    }
    else if(strcmp(mode, "w") == 0)
    {
        return FILE_MODE_WRITE;
    }
    else if(strcmp(mode, "a") == 0)
    {
        return FILE_MODE_APPEND;
    }

    return FILE_MODE_INVALID;
}

int fopen(const char* filename, const char* mode)
{
    struct path_root* root = parse(filename);
    if(!root)
    {
        return -1;
    }
    
    // Check if just having the root path
    if(!root->first_part)
    {
        return -1;
    }

    struct disk* disk = get_disk(root->drive_no);
    if(!disk)
    {
        return -1;
    }

    if(!disk->filesystem)
    {
        return -1;
    }

    FILE_OPEN_MODE open_mode = get_file_open_mode(mode);
    if(open_mode == FILE_MODE_INVALID)
    {
        return -1;
    }

    void* data_to_descriptor = (*disk->filesystem->open_file)(disk, root->first_part, open_mode);

    struct file_descriptor* fd = 0;
    if(get_new_file_descriptor(&fd) != 0)
    {
        return -1;
    }

    fd->fs = disk->filesystem;
    fd->data = data_to_descriptor;
    fd->disk = disk;
    return fd->index;
}

int fread(void* ptr, uint32_t size, uint32_t count, int fd)
{
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS || size <= 0 || count <= 0)
    {
        return -1;
    }

    struct file_descriptor* file = get_file_descriptor(fd);
    if(!file)
    {
        return -1;
    }

    return (*file->fs->read_file)(file->disk, file->data, size, count, ptr);
}

static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0x00;
    kfree(desc);
}

int fclose(int fd)
{
    int res = 0;
    struct file_descriptor* desc = get_file_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->fs->close(desc->data);
    if(res == 0)
    {
        file_free_descriptor(desc);
    
    }
out:
    return res;
}

