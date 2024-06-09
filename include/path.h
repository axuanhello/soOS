# ifndef PATH_H
# define PATH_H

#define MAX_PATH_LEN 128

struct path_root{
    int drive_no;
    struct path_part* first_part;
};

struct path_part
{
    const char* part;
    struct path_part* next;
};

struct path_root* parse(const char* path);
void free_path(struct path_root* root);

# endif