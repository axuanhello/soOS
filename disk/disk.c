#include "disk.h"
#include "io.h"
#include "config.h"
#include "errno.h"
#include "string.h"
#include "vfs.h"
#include "mm.h"

struct disk disk; // Primary hard disk

/**
 * LBA:Linear Block Address
 * Reference: https://wiki.osdev.org/ATA_Command_Matrix
 * Reference: https://wiki.osdev.org/ATA_read/write_sectors
 * 
*/
int read_disk_sector(int lba, int total, void* buf)
{
    /**
     * Port 0x1F6: send lba bit 24-27 and set master/slave bit
     * Port 0x1F2: Number of sectors to read
     * Port 0x1F3: LBA low byte(bit 0-7)
     * Port 0x1F4: LBA mid byte(bit 8-15)
     * Port 0x1F5: LBA high byte(bit 16-23)
    */
    outb(0x1F6, (lba >> 24) | 0xE0); //1f6h:port to send drive & head numbers; Make 32-bit lba to 8-bit and set the master bit
    outb(0x1F2, total); // Number of sectors to read
    outb(0x1F3, (unsigned char)(lba & 0xff)); // 1f3h: Sector number port with LBA low byte
    outb(0x1F4, (unsigned char)(lba >> 8)); // 1f4h: Cylinder low port;LBA mid byte
    outb(0x1F5, (unsigned char)(lba >> 16)); // 1f5h: Cylinder high port;LBA high byte
    
    // 0x20: READ SECTOR(S)	PIO	8-bit	IBM PC/AT to present
    // Reference: https://wiki.osdev.org/ATA_Command_Matrix
    outb(0x1F7, 0x20); // 1F7h:Command port; 0x20: Command to read sectors
    unsigned short* ptr = (unsigned short*) buf; // Read two bytes at a time
    for (int b = 0; b < total; b++)
    {
        // Wait for the disk to be ready
        // PIO data transfer is done by the CPU, so we need to wait for the disk to be ready
        char status = inb(0x1F7);
        //int cnt = 0;
        while (!(status & 0x08))
        {
            status = inb(0x1F7);
            
        }

        for (int i = 0; i < 256; i++)
        {
            unsigned short data = inw(0x1F0);
            *ptr = data;
            ptr++;
        }
    }

    return 0;
}

void search_and_init_disk()
{
    memset(&disk, 0, sizeof(struct disk));
    disk.type = REAL_DISK_TYPE;
    disk.sector_size = SECTOR_SIZE;
    disk.disk_id = 0; // TODO: More disk support
    disk.filesystem = resolve_fs(&disk);
}

// Only get the disk 0
struct disk* get_disk(int index)
{
    if(index == 0)
    {
        return &disk;
    }

    return 0;
}

int read_disk_block(struct disk* _disk, unsigned int lba, int total, void* buf)
{
    if (_disk != &disk){
        return -EIO;
    }

    return read_disk_sector(lba, total, buf);
}

/**
 * @brief Create a disk stream
 * @param disk_id: The disk id(Only support 0)
 * @return struct disk_stream*: The disk stream
 */
struct disk_stream* create_disk_stream(int disk_id)
{
    struct disk_stream* stream = (struct disk_stream*) kmalloc(sizeof(struct disk_stream));
    stream->pos = 0;
    stream->disk = (struct disk*)get_disk(disk_id);
    return stream;
}

/**
 * @brief Destroy the disk stream
 * @param stream: The disk stream
 */
void destroy_disk_stream(struct disk_stream* stream)
{
    kfree(stream);
}

/**
 * @brief Seek the disk stream
 * @param stream: The disk stream
 * @param pos: The position to seek
 */
int seek_disk_stream(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

/**
 * @brief Read from the disk stream
 * @param stream: The disk stream
 * @param total: The total bytes to read
 * @param out: The output buffer
 * @return int: 0 if success, otherwise error code
 */
int read_disk_stream(struct disk_stream* stream, int total, void* out)
{
    int sector = stream->pos / SECTOR_SIZE;
    int offset = stream->pos % SECTOR_SIZE;

    char buf[SECTOR_SIZE];
    int res = read_disk_block(stream->disk, sector, 1, buf);
    if(res < 0)
    {
        return res;
    }

    int total_to_read = total > SECTOR_SIZE? SECTOR_SIZE: total;
    for(int i = 0; i < total_to_read; i++)
    {
        ((char*)out)[i] = buf[offset + i];
    }

    stream->pos += total_to_read;
    if(total > SECTOR_SIZE)
    {
        // Iterate to read the next sector
        return read_disk_stream(stream, total - total_to_read, (char*)out);
    }

    return res;
}