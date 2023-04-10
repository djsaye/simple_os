#include "types.h"

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#define FILENAME_LEN 32

typedef struct __attribute__ ((packed)) dentry {
    uint8_t filename[FILENAME_LEN];
    uint32_t filetype;
    uint32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

typedef struct __attribute__ ((packed)) boot_block {
    uint32_t dir_count;
    uint32_t inode_count;
    uint32_t data_count;
    uint8_t reserved[52];
    dentry_t dentries[63];
} boot_block_t;

typedef struct __attribute__ ((packed)) inode {
    uint32_t length;
    uint32_t data_block_num[1023];
} inode_t;

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t get_file_name(int index, void* buf);

void fs_init(uint32_t fs_start);

void list_filesystem();

int get_dir_inode();

#endif // FILESYSTEM_H
