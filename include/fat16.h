#ifndef FAT16_H
#define FAT16_H

#include "vfs.h"

struct filesystem;

struct filesystem* init_fat16();
int resolve_fat16(struct disk* disk);
void* fat16_open_file(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
#endif // FAT16_H