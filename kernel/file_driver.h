#include "types.h"
#include "filesystem.h"

#ifndef FILE_DRIVER_H
#define FILE_DRIVER_H

typedef struct file_object {
    int32_t inode;
    int32_t curr_offset;
    int32_t filetype;
} file_object_t;

int32_t fs_read (int32_t fd, void* buf, int32_t nbytes);
int32_t fs_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t fs_open (const uint8_t* filename);
int32_t fs_close (int32_t fd);
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes);
int32_t fs_load_exe(const uint8_t* filename, uint32_t program_num);
int32_t fs_reload_exe(uint32_t program_num);
#endif
