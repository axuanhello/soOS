#ifndef __STRING_H
#define __STRING_H

#ifndef NULL
#define NULL (void*(0))
#endif
#include"types.h"
size_t strlen(const char* str);
size_t strlen_with_stop(const char* str, char stop);
int strcmp(const char* str1, const char* str2);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
bool isnum(char c);
int ctoi(char c);
char tolower(char c);
int strcmp_prefix(const char* str1, const char* str2, size_t n);
int strcmp_prefix_ignore_case(const char* str1, const char* str2, size_t n);
//若count输入负数将会导致复制次数错误。
void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* str, int c, size_t n);
int memcmp(const void* str1, const void* str2, size_t n);
#endif