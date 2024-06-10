#ifndef DISK_H
#define DISK_H

// Represents a real physical disk
#define REAL_DISK_TYPE 0

typedef unsigned int disk_type;
struct disk{
    disk_type type;
    unsigned int sector_size;

    int disk_id;

    struct filesystem* filesystem;

    void* data;
};


// Represents a stream of data from a disk. 
// @param pos pos is the current position in the stream
// @param disk disk is the pointer to disk that the stream is reading from
struct disk_stream
{
    int pos;
    struct disk* disk;
};

// int read_disk_sector(int lba, int total, void* buf);
struct disk* get_disk(int index);
void search_and_init_disk();
int read_disk_block(struct disk* _disk, unsigned int lba, int total, void* buf);

struct disk_stream* create_disk_stream(int disk_id);
void destroy_disk_stream(struct disk_stream* stream);
int seek_disk_stream(struct disk_stream* stream, int pos);
int read_disk_stream(struct disk_stream* stream, int total, void* out);

#endif