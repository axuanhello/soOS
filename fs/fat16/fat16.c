#include "fat16.h"
#include "string.h"
#include "errno.h"
#include "types.h"
#include "disk.h"
#include "mm.h"

#define FAT16_SIGNATURE 0x29
#define FAT16_ENRTY_SIZE 0x02

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

typedef char FAT16_ITEM_TYPE;
#define FAT16_ITEM_TYPE_FILE 1
#define FAT16_ITEM_TYPE_DIRECTORY 0

// FAT16目录项attr属性bitmask
#define FAT16_FILE_READ_ONLY 0x01
#define FAT16_FILE_HIDDEN 0x02
#define FAT16_FILE_SYSTEM 0x04
#define FAT16_FILE_VOLUME_LABEL 0x08
#define FAT16_FILE_SUBDIRECTORY 0x10
#define FAT16_FILE_ARCHIVED 0x20
#define FAT16_FILE_DEVICE 0x40
#define FAT16_FILE_RESERVED 0x80



void* fat16_open_file(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
int resolve_fat16(struct disk* disk);
int fat16_read_file(struct disk* disk, void* fd, u32 size, u32 nb, char* out);
int fat16_close(void* private);

struct fat_header{
    u8 jump[3];
    u8 oem[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 root_entry_count;
    u16 total_sectors;
    u8 media_type;
    u16 sectors_per_fat;
    u16 sectors_per_track;
    u16 head_count;
    u32 hidden_sectors;
    u32 total_sectors_large;
} __attribute__((packed));

struct extended_bios_parameter_block{
    u8 drive_number;
    u8 reserved;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 system_id[8];
} __attribute__((packed));

// fat16_entry represents a file
struct fat16_entry{
    u8 name[8];
    u8 ext[3];
    u8 attr;
    u8 reserved;
    u8 creation_time_tenths;
    u16 creation_time;
    u16 creation_date;
    u16 last_access_date;
    u16 first_cluster_high;
    u16 last_mod_time;
    u16 last_mod_date;
    u16 first_cluster_low;
    u32 size;
} __attribute__((packed));

// Contains the FAT16 filesystem information
struct fat16_fs_info{
    struct fat_header header;
    union{
        struct extended_bios_parameter_block ebpb;
    } shared;
};

// fat16_directory represents a directory
struct fat16_directory{
    struct fat16_entry* entries; // Point to the first entry in this directory
    int totel_entries; // Total entries in this directory
    int sector_begin; // The first sector of this directory
    int sector_end; // The last sector of this directory
};

/** 
 * A fat16_item represents a file or directory
 * Wrapper for fat16_entry and fat16_directory
 * @param directory struct fat16_directory* - The directory that the item is pointing to(union shared with entry)
 * @param entry struct fat16_entry* - The entry that the item is pointing to(union shared with directory)
 * @param type FAT16_ITEM_TYPE - The type of the item
 * 
 */
struct fat16_item{
    union { // Use union to save memory
        struct fat16_directory* directory;
        struct fat16_entry* entry;
    };

    FAT16_ITEM_TYPE type;
};

/** 
 * fat16_file_descriptor represents a file descriptor
 * @param item struct fat16_item* - The item that the file descriptor is pointing to
 * @param offset uint32_t - The current offset(for seeking) of the file 
 * 
 */
struct fat16_file_descriptor{
    struct fat16_item* item;
    u32 offset;
};

struct fat16_data{
    struct fat16_fs_info header;
    struct fat16_directory root;

    // Stream for reading cluster data
    struct disk_stream* read_cluster_streamer;

    // Stream the file allocation table
    struct disk_stream* read_fat_streamer;

    // Stream for reading directory data
    struct disk_stream* directory_streamer;
};

struct filesystem fat16_fs = {
    .open_file = fat16_open_file,
    .read_file = fat16_read_file,
    .close = fat16_close,
    .resolve = resolve_fat16
};


struct filesystem* init_fat16()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

/**
 * Initialize the fat16_data structure
 * Create the disk streamers for reading cluster, fat and directory data
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @param[in,out] data struct fat16_data* - The fat16_data structure to initialize
 */
void init_fat16_data(struct disk* disk, struct fat16_data* data)
{
    memset(data, 0, sizeof(struct fat16_data));

    data->read_cluster_streamer = create_disk_stream(disk->disk_id);
    data->read_fat_streamer = create_disk_stream(disk->disk_id);
    data->directory_streamer = create_disk_stream(disk->disk_id);
}

/**
 * Convert a sector to an address
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @param[in] sector int - The sector to convert
 */
int sector_to_address(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

/**
 * Get total entries in a directory
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @param[in] start_sector u32 - The sector to start reading the directory from
 */
int get_total_entries(struct disk* disk, u32 start_sector)
{
    struct fat16_entry dir;
    struct fat16_entry empty_dir;

    memset(&empty_dir, 0, sizeof(struct fat16_entry));

    struct fat16_data* data = (struct fat16_data*)disk->data;

    int start_pos = sector_to_address(disk, start_sector);

    struct disk_stream* stream = data->directory_streamer;
    if(seek_disk_stream(stream, start_pos) != 0)
    {
        return -EIO;
    }

    int cnt = 0;
    while(1)
    {
        if(read_disk_stream(stream, sizeof(dir), &dir) != 0)
        {
            return -EIO;
        }

        if(dir.name[0] == 0x00) // Empty
        {
            break;
        } else if(dir.name[0] == 0xE5) // Deleted
        {
            continue;
        }
        cnt ++;
    }
    return cnt;
}

/**
 * @brief Get the root directory into memory
 */
int get_root_dir(struct disk* disk, struct fat16_data* data, struct fat16_directory* directory)
{
    struct fat16_fs_info* header = &data->header;
    int root_dir_sector_pos = (header->header.fat_count * header->header.sectors_per_fat) + header->header.reserved_sectors; 
    int root_dir_entries = header->header.root_entry_count;
    int root_dir_size = root_dir_entries * sizeof(struct fat16_entry);
    int total_sectors = root_dir_size / disk->sector_size;
    if(root_dir_size % disk->sector_size != 0)
    {
        total_sectors ++;
    }

    int total_entries = get_total_entries(disk, root_dir_sector_pos);

    struct fat16_entry* dir = (struct fat16_entry*) kmalloc(root_dir_size);
    if(!dir){
        return -ENOMEM;
    }

    struct disk_stream* stream = data->directory_streamer;
    if(seek_disk_stream(stream, sector_to_address(disk, root_dir_sector_pos)) != 0)
    {
        return -EIO;
    }

    if(read_disk_stream(stream, root_dir_size, dir) != 0)
    {
        return -EIO;
    }

    directory->entries = dir;
    directory->totel_entries = total_entries;
    directory->sector_begin = root_dir_sector_pos;
    directory->sector_end = root_dir_sector_pos + (root_dir_size / disk->sector_size); // Point to the last sector

    return 0;
}

/**
 * Resolve the fat16 filesystem
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @return int - 0 if the filesystem is resolved successfully, otherwise return an error code
 
 */
int resolve_fat16(struct disk* disk)
{
    struct fat16_data* data = (struct fat16_data*) kmalloc(sizeof(struct fat16_data));
    init_fat16_data(disk, data);

    disk->data = data;
    disk->filesystem = &fat16_fs;

    struct disk_stream* stream = create_disk_stream(disk->disk_id);
    if(!stream)
    {
        kfree(data);
        return -ENOMEM;
    }

    char buf[512];
    if(read_disk_stream(stream, sizeof(struct fat16_fs_info), &data->header) != 0)
    {
        kfree(data);
        return -EIO;
    }

    u8 signature = data->header.shared.ebpb.boot_signature;
    if(signature != 0x28 && signature != 0x29)
    {
        kfree(data);
        return -EINVARG;
    }
    
    if(get_root_dir(disk, data, &data->root) != 0)
    {
        kfree(data);
        return -EIO;
    }

    if(stream) destroy_disk_stream(stream);

    return 0;
}

/**
 * Convert a fat16 filename to a string
 * @param[in] in const char* - The fat16 filename to convert
 * @param[out] out char** - The double pointer to the output buffer to store the converted filename
 
 */
void fat16_filename_to_string(const char* in, char** out) {
    // 循环遍历输入字符串，直到遇到空字符（0x00）或空格（0x20）
    while (*in != 0x00 && *in != 0x20) {
        **out = *in;   // 将输入字符串的当前字符复制到输出字符串中
        *out += 1;     // 输出字符串指针移动到下一个位置
        in += 1;       // 输入字符串指针移动到下一个位置
    }

    // 如果遇到空格（0x20），在输出字符串的当前位置添加空字符（0x00）
    if (*in == 0x20) {
        **out = 0x00;
    }
}

/**
 * Get the full filename of a fat16 entry
 * @param[in] entry struct fat16_entry* - The fat16 entry to get the filename from
 * @param[out] out char* - The output buffer to store the filename
 * @param[in] max_len int - The maximum length of the output buffer
 
 */
void get_full_filename(struct fat16_entry* entry, char* out, int max_len)
{
    memset(out, 0x00, max_len);
    fat16_filename_to_string((const char*)entry->name, &out);
    if(entry->ext[0] != 0x00 && entry->ext[0] != 0x20) // If the filename's extension is not empty
    {
        *out++ = '.'; // Append extension name
        fat16_filename_to_string((const char*)entry->ext, &out);
    }
}

/**
 * @brief Clone a fat16 entry
 * @param[in] entry struct fat16_entry* - The fat16 entry to clone
 * @param[in] size int - The size of the entry to clone
 * @return struct fat16_entry* - The cloned fat16 entry
 */
struct fat16_entry* clone_fat16_entry(struct fat16_entry* entry, int size)
{
    struct fat16_entry* entry_clone = 0;
    
    if(size < sizeof(struct fat16_entry))
    {
        return 0;
    }
    entry_clone = (struct fat16_entry*)kmalloc(size);

    if(!entry_clone)
    {
        return 0;
    }

    memcpy(entry_clone, entry, size);
    return entry_clone;
}

/**
 * @brief Get the first cluster of a fat16 entry
 * @param entry struct fat16_entry* - The fat16 entry to get the first cluster from
 * @return u32 - The first cluster of the entry
 */
static u32 get_first_cluster(struct fat16_entry* entry)
{
    return (entry->first_cluster_high) | entry->first_cluster_low;
}

/**
 * @brief Get the sector of a cluster(usually a file's starting cluster);
 * @warning The cluster must start from 2
 * @param data struct fat16_data* - The fat16 data structure
 * @param cluster int - The cluster to get the sector from
 * @return int - The sector of the cluster

 */
static int cluster_to_sector(struct fat16_data* data, int cluster)
{
    return data->root.sector_end + ((cluster - 2) * data->header.header.sectors_per_cluster);
}

/**
 * @brief get the first sector of a fat16 data structure
 * @param data struct fat16_data* - The fat16 data structure
 * @return u32 - The first sector of the fat16 data structure
 
 */
static u32 get_first_sector(struct fat16_data* data)
{
    return data->header.header.reserved_sectors;
}

/**
 * @brief get the FAT entry for the specified cluster number
 * @param disk struct disk* - The disk that the filesystem is on
 * @param cluster int - The cluster to get the entry from
 * @return u16 - The FAT entry at the specified cluster
 */
static u16 get_entry(struct disk* disk, int cluster)
{
    struct fat16_data* data = (struct fat16_data*)disk->data;
    struct disk_stream* stream = data->read_fat_streamer;
    if(!stream)
    {
        return -EIO;
    }

    u32 fat_position = data->header.header.reserved_sectors * disk->sector_size; // Get the position of FAT area

    // Position the FAT entry for the specified cluster
    if(seek_disk_stream(stream, fat_position + (cluster * FAT16_ENRTY_SIZE)) != 0) // MOD: fat_pos + (xxx)
    {
        return -EIO;
    }

    u16 result = 0;

    // Get the FAT entry
    if(read_disk_stream(stream, sizeof(u16), &result) !=0){
        return -EIO;
    }

    return result;
}

/**
 * @brief Get the cluster to use based on the cluster and offset
 * @param disk struct disk* - The disk that the filesystem is on
 * @param cluster int - The current cluster
 * @return int - The next cluster in the chain
 
 */
static int get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset)
{ 
    struct fat16_data* data = disk->data;
    int cluster_size = data->header.header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / cluster_size; // Get the number of clusters ahead to read
    for (int i = 0; i < clusters_ahead; i++)
    {
        u16 entry = get_entry(disk, cluster_to_use);
        if (entry == 0xFF8 || entry == 0xFFF) // Last entry in the file
        {
            return -EIO;
        }

        // Check for reserved sectors
        if (entry == 0xFF0 || entry == 0xFF6)
        {
            return -EIO;
        }

        // If the entry is 0x00, the cluster is empty
        if (entry == 0x00)
        {
            return -EIO;
        }

        cluster_to_use = entry;
    }

    return cluster_to_use;
}

/**
 * @brief Read data from a cluster stream, the lower implementation of read_internal_data
 * @param disk struct disk* - The disk that the filesystem is on
 * @param stream struct disk_stream* - The stream to read from
 * @param cluster int - The cluster to read from
 * @param offset int - The offset within the cluster to read from
 * @param total int - The total bytes to read
 * @param out void* - The output buffer to store the read data
 * @return int - 0 if the data is read successfully, otherwise return an error code
 * @attention If the total bytes to read is greater than the cluster size, the function will find the next cluster in the FAT to read.
 
 */
static int read_data_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out)
{
    struct fat16_data* data = disk->data;
    int cluster_size = data->header.header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0)
    {
        return cluster_to_use;
    }

    int cluster_offset = offset % cluster_size;

    int start_sector = cluster_to_sector(data, cluster_to_use);
    int start_pos = (start_sector * disk->sector_size) + cluster_offset;
    int total_to_read = total > cluster_size ? cluster_size : total;

    if (seek_disk_stream(stream, start_pos) != 0)
    {
        return -EIO;
    }

    if(read_disk_stream(stream, total_to_read, out) != 0)
    {
        return -EIO;
    }

    total -= total_to_read;
    if (total > 0)
    {
        // Read the next cluster
        return read_data_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }

    return 0;
}

/**
 * @brief Read internal data from a cluster
 * @param disk struct disk* - The disk that the filesystem is on
 * @param cluster int - The cluster to read from
 * @param offset int - The offset within the cluster to read from
 * @param total int - The total bytes to read
 * @param out void* - The output buffer to store the read data
 * @return int - 0 if the data is read successfully, otherwise return an error code

 */
static int read_internal_data(struct disk* disk, int cluster, int offset, int total, void* out)
{
    struct fat16_data* data = disk->data;
    struct disk_stream* stream = data->read_cluster_streamer;
    if (!stream)
    {
        return -EIO;
    }

    return read_data_from_stream(disk, stream, cluster, offset, total, out);
}

void free_fat16_item(struct fat16_item* item)
{
    if(item->type == FAT16_ITEM_TYPE_DIRECTORY)
    {
        kfree(item->directory->entries);
    }
}

void free_fat16_directory(struct fat16_directory* dir){
    if (!dir)
    {
        return;
    }

    if (dir->entries)
    {
        kfree(dir->entries);
    }

    kfree(dir);
}

/**
 * @brief Load a fat16 directory from a fat16 entry
 * @param disk struct disk* - The disk that the filesystem is on
 * @param entry struct fat16_entry* - The entry to load the directory from
 * @return struct fat16_directory* - The loaded directory
 
 */
struct fat16_directory* load_fat16_directory(struct disk* disk, struct fat16_entry* entry)
{
    struct fat16_directory* dir = (struct fat16_directory*)kmalloc(sizeof(struct fat16_directory));
    if (!dir)
    {
        return 0;
    }

    // Get the first cluster of the directory 
    // The first cluster is the starting cluster of a new directory(like the root directory)
    // Subdirectory is essentially an array of fat16 entries
    int cluster = get_first_cluster(entry); 

    int sector = cluster_to_sector(disk->data, cluster); // Get the sector of the cluster
    int total_entries = get_total_entries(disk, sector);
    dir->totel_entries = total_entries;
    int dir_size = total_entries * sizeof(struct fat16_entry);

    // Load the entire directory into memory
    dir->entries = (struct fat16_entry*)kmalloc(dir_size);
    if(!dir->entries)
    {
        kfree(dir);
        return 0;
    }
    
    
    if(read_internal_data(disk, cluster, 0x00, dir_size, dir->entries) < 0)
    {
        kfree(dir->entries);
        kfree(dir);
        return 0;
    }

    return dir;
}

/**
 * @brief Create a fat16 item(For fat16_filedescriptor) for a directory
 * @param disk struct disk* - The disk that the filesystem is on
 * @param entry struct fat16_entry* - The entry to create the item from
 * @return struct fat16_item* - The created item
 
 */
struct fat16_item* create_fat_item_for_directory(struct disk* disk, struct fat16_entry* entry)
{
    struct fat16_item* item = (struct fat16_item*)kmalloc(sizeof(struct fat16_item));
    if(!item){
        return 0;
    }

    if(entry->attr & FAT16_FILE_SUBDIRECTORY)
    {
        item->type = FAT16_ITEM_TYPE_DIRECTORY;
        item->directory = load_fat16_directory(disk, entry); // Load in the directory
    } else {
        item->type = FAT16_ITEM_TYPE_FILE;
        item->entry = clone_fat16_entry(entry, sizeof(struct fat16_entry)); // Clone the file entry into the item structure
    }

    return item;
}

/**
 * Find an item in the root directory
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @param[in] dir struct fat16_directory* - The directory to search in
 * @param[in] name const char* - The name of the item to find
 
 */
struct fat16_item* find_item_in_root_directory(struct disk* disk, struct fat16_directory* dir, const char* name)
{
    for (int i = 0; i < dir->totel_entries; i++)
    {
        struct fat16_entry* entry = &dir->entries[i];
        char filename[MAX_PATH_LEN];
        get_full_filename(entry, filename, sizeof(filename));
        if(strcmp_prefix_ignore_case(filename, name, sizeof(name)) == 0)
        {
            return create_fat_item_for_directory(disk, entry);
        }
    }

    return 0;
}

/**
 * Get the item in the directory
 * @param[in] disk struct disk* - The disk that the filesystem is on
 * @param[in] path struct path_part* - The path to the item
 * @return struct fat16_item* - The item in the root directory
 */
struct fat16_item* get_root_directory_item(struct disk* disk, struct path_part* path)
{
    struct fat16_data* data = disk->data;
    struct fat16_item* root_item = find_item_in_root_directory(disk, &data->root, path->part); // Find the item in the root directory
    if(!root_item)
    {
        return 0;
    }

    struct path_part* next = path->next; // Get the next part of the path
    struct fat16_item* current = root_item; // Set the current item to the root item(Maybe a directory or a file)
    while(next)
    {
        if(current->type != FAT16_ITEM_TYPE_DIRECTORY) // If the current item is not a directory and has a subdirectory, return 0
        {
            free_fat16_item(current);
            return 0;
        }

        struct fat16_item* next_item = find_item_in_root_directory(disk, current->directory, next->part); // Find the next item in the current directory
        if(!next_item)
        {
            free_fat16_item(current);
            return 0;
        }

        free_fat16_item(current);
        current = next_item; // Set the current item to the next item
        next = next->next;
    }

    return current; // Return the current item
}

/**
 * @brief Fat16 filesystem's open method for opening a file, returns its file descriptor
 * @param disk struct disk* - The disk that the filesystem is on
 * @param path struct path_part* - The path to the file
 * @param mode FILE_OPEN_MODE - The mode to open the file in(not implemented yet)
 * @return void* - The file descriptor of the opened file(fat16_file_descriptor*)
 
 */
void* fat16_open_file(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode)
{
    struct fat16_item* item = get_root_directory_item(disk, path); // Get the item in the root directory
    if(!item)
    {
        return 0;
    }

    if(item->type != FAT16_ITEM_TYPE_FILE) // If the item is not a file, free the invalid item and return 0
    {
        free_fat16_item(item);
        return 0;
    }

    struct fat16_file_descriptor* fd = (struct fat16_file_descriptor*)kmalloc(sizeof(struct fat16_file_descriptor));
    if(!fd)
    {
        free_fat16_item(item);
        return 0;
    }

    fd->item = item; // Set the item of the file descriptor
    fd->offset = 0; // Set the offset of the file descriptor to 0(Start reading from the beginning of the file)

    return fd;
}

/**
 * @brief FAT16's read method for reading a file
 * @param disk struct disk* - The disk that the filesystem is on
 * @param fd void* - The file descriptor of the file to read
 * @param size u32 - The size of each read
 * @param nb u32 - The number of reads
 * @param out char* - The output buffer to store the read data
 * @return int - The number of reads that were successful

 */
int fat16_read_file(struct disk* disk, void* fd, u32 size, u32 nb, char* out)
{
    struct fat16_file_descriptor* descriptor = (struct fat16_file_descriptor*)fd;
    struct fat16_entry* entry = descriptor->item->entry;
    int offset = descriptor->offset;
    for(u32 i = 0; i < nb; i ++ )
    {
        if(read_internal_data(disk, get_first_cluster(entry), offset, size, out) < 0)
        {
            return 0;
        }

        out += size;
        offset += size;
    }

    return nb;
}

/**
 * @brief Free a fat16 file descriptor
 * @param desc struct fat16_file_descriptor* - The file descriptor to free

 */
static void fat16_free_file_descriptor(struct fat16_file_descriptor* desc)
{
    free_fat16_item(desc->item);
    kfree(desc);
}


int fat16_stat(struct disk* disk, void* private, struct file_stat* stat)
{
    int res = 0; 
    struct fat16_file_descriptor* descriptor = (struct fat16_file_descriptor*) private;
    struct fat16_item* desc_item = descriptor->item;
    if (desc_item->type != FAT16_ITEM_TYPE_FILE)
    {
        return -1;
    }

    struct fat16_entry* ritem = desc_item->entry;
    stat->filesize = ritem->size;
    stat->flags = 0x00;

    if (ritem->attr & FAT16_FILE_READ_ONLY)
    {
        stat->flags |= FILE_STAT_READ_ONLY;
    }

    return 0;
}

/**
 * @brief FAT16's close method for closing a file
 * @param private void* - The file descriptor of the file to close
 * @return int - 0 if the file is closed successfully
 */
int fat16_close(void* private)
{
    fat16_free_file_descriptor((struct fat16_file_descriptor*) private);
    return 0;
}