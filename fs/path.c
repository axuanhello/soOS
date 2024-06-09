#include "path.h"
#include "string.h"
#include "mm.h"
#include "config.h"
#include "errno.h"

static int valid_path_format(const char* path)
{
    int len = strlen(path) > MAX_PATH_LEN ? MAX_PATH_LEN : strlen(path);
    return (len >= 3 && isnum(path[0]) && !memcmp((void*)&path[1], ":/", 2));
}

/**
 * @name get_path_drive
 * @param path The original file path
 * @return The drive number from the path; if the path is invalid, return -EINVAPTH(-4)
 * @brief Get the drive number from the path; Add the path pointer to skip the drive number and the colon and the slash(e.g. "0:/path/sub" -> "path/sub")
*/
static int get_path_drive(const char** path)
{
    if(!valid_path_format(*path))
    {
        return -EINVAPTH;
    }
    int drv_no =  ctoi(*path[0]);

    // Add 3 to skip the drive number and the colon and the slash
    *path += 3;
    return drv_no;
}

/**
 * @name create_root
 * @param drv_no The drive number (Typically Get from get_path_drive(original_path))
 * @brief Create a new path_root struct(without exception handling)
 * @return A new path_root struct(allocated by kmalloc in heap without exception handling) with the drive number and the first_part set to 0(null)
*/
static struct path_root* create_root(int drv_no)
{
    struct path_root *root = kmalloc(sizeof(struct path_root));
    root->drive_no = drv_no;
    root->first_part = 0;

    return root;
}

/**
 * @name create_path_part
 * @param part The path skipping "0:/", typically get from get_path_drive(original_path); part will be updated to the next part after returning(e.g. "part/sub/sub2" -> "sub/sub2")
 * @brief Create a new path_part struct(without exception handling)
 * @return A new path_part struct(allocated by kmalloc in heap without exception handling) with the part set to the path part and the next set to 0(null)
*/
static const char* create_path_part(const char** part)
{
    char* path_part = kmalloc(MAX_PATH_LEN);
    int cnt = 0;
    while(**part != '/' && **part != '\0')
    {
        path_part[cnt] = **part;
        cnt ++;
        (*part)++;
    }

    if(**part == '/')
    {
        // Skip the slash
        // In case next time we call this function, we don't want to copy the slash
        (*part)++;
    }

    if(cnt == 0)
    {
        kfree(path_part);
        path_part = 0;
        return 0;
    }

    return path_part;
}

/**
 * @name parse_path
 * @param last_part The last struct path_part(Typically get from create_path_part), its next will point to the part returned by this function; if it's the first part after root, set it to 0(null)
 * @param path The path typically get from create_path_part or get_path_drive; path will be updated to the next part after returning(e.g. "part/sub/sub2" -> "sub/sub2")
 * @return A new path_part struct(allocated by kmalloc in heap without exception handling) with the part set to the path part and the next set to 0(null)
*/
struct path_part* parse_path(struct path_part* last_part, const char** path)
{
    const char* part_str = create_path_part(path);
    if(!part_str)
    {
        return 0;
    }

    struct path_part* part = kmalloc(sizeof(struct path_part));
    part->part = part_str;
    part->next = 0;

    if(last_part){ // if last_part is not null
        last_part->next = part;
    }

    return part;
}

/**
 * @name free_path
 * @param root The root struct path_root for the whole path to be freed
 * @brief free the whole path
*/
void free_path(struct path_root* root){
    struct path_part* part = root->first_part;
    while(part)
    {
        struct path_part* next = part->next;
        kfree((void*)part->part);
        kfree((void*)part);
        part = next;
    }

    kfree((void*)root);
}

struct path_root* parse(const char* path)
{
    int drive_no = 0;
    const char* tmp = path;
    struct path_root* root = 0;

    if(strlen(path) > MAX_PATH_LEN)
    {
        return root;
    }

    drive_no = get_path_drive(&tmp);

    root = create_root(drive_no);
    if(!root)
    {
        return root;
    }

    struct path_part* first_part = parse_path(0, &tmp); // Get the first part
    if(first_part) // if the first part is not null
    {
        root->first_part = first_part; // Set the first part
    }

    // root->first_part = first_part;
    struct path_part* last_part = first_part;
    while(last_part)
    {
        last_part = parse_path(last_part, &tmp);
    }    

    return root;
}
