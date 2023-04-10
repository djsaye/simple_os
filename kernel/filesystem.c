#include "filesystem.h"
#include "types.h"
#include "lib.h"

static boot_block_t* fs_boot_block;

static int curr_dir_inode = -1;

#define FOUR_KILO 4096

/* fs_init
 * 
 * DESCRIPTION: Initializes the filesystem 
 * 
 * INPUTS: fs_start: address pointing to the start of the file system
 *                   in memory
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Has boot block point to the start of the file system
 *               prints file names to terminal as well
 */
void fs_init(uint32_t fs_start) {
    fs_boot_block = (boot_block_t*)fs_start; // convert start address to a pointer
    int i, j; // loop vars
    for (i = 0; i < fs_boot_block->dir_count; i++) { // loop through dentries
        for (j = 0; j < FILENAME_LEN; j++) { // print names of files
            printf("%c", fs_boot_block->dentries[i].filename[j]);
        }
        printf("\n");
    }
    // dentry_t test_entry;
    // uint8_t buf[100];
    // if (!read_dentry_by_name((uint8_t*) "frame0.txt", &test_entry)) {
    //     printf("acquired dentry %s\n", test_entry.filename);
    //     read_data(test_entry.inode_num, 0, buf, 50);
    //     clear();
    //     printf("padding\n");
    //     printf("%s\n", buf);
    // } else {
    //     printf("catastrophic failure.\n");
    // }
}


/* read_dentry_by_name
 * 
 * DESCRIPTION: Populaltes a dentry with the info about the dentry
 *              with that name in the filesystem
 * 
 * INPUTS: fname: file name to search for
 *         dentry: the dentry to be populated by this read
 * OUTPUTS: 0 if successful, -1 if invalid
 * RETURN VALUE: int, indicating success (0 for success)
 * SIDE EFFECTS: populates dentry pointed to by parameter from boot block
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    int idx;
    int fname_len = strlen((int8_t *) fname); // get the legnth of the string
    int dir_flag = 0;
    if (fname_len > FILENAME_LEN) { // if it is an invalid string, return failure
        return -1;
    }
    if(strncmp((int8_t*) fname, ".", FILENAME_LEN) == 0){
        dir_flag = 1;
    }
    for (idx = 0; idx < fs_boot_block->dir_count; idx++) { // go through every file int the system
        // check if the name is equivalent to the filename, break if it is
        if (!(strncmp((int8_t *) fs_boot_block->dentries[idx].filename, (int8_t *) fname, FILENAME_LEN))) {
            break;
        }
    }
    // idx is corrent, call read_by_index from here
    int32_t retval = read_dentry_by_index(idx, dentry);
    if(dir_flag) curr_dir_inode = dentry->inode_num;
    return retval;
}

/* read_dentry_by_index
 * 
 * DESCRIPTION: Populaltes a dentry with the info about the dentry
 *              at that index in the filesystem
 * 
 * INPUTS: index: index into the list of dentries to read
 *         dentry: the dentry to be populated by this read
 * OUTPUTS: 0 if successful, -1 if invalid
 * RETURN VALUE: int, indicating success (0 for success)
 * SIDE EFFECTS: populates dentry pointed to by parameter from boot block
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    // if index is out of bounds, return failure
    if (index >= fs_boot_block->dir_count) {return -1;}
    int i;
    // get pointer to the dentry we want to read
    dentry_t* temp_entry = &(fs_boot_block->dentries[index]);

    for (i = 0; i < FILENAME_LEN; i++) { // fill in the file name
        dentry->filename[i] = temp_entry->filename[i];
    }
    // fill in the file type
    dentry->filetype = temp_entry->filetype;
    // fill in the inode number
    dentry->inode_num = temp_entry->inode_num;

    // for (i = 0; i < 24; i++) { // fill in the  reserved bytes
    //     dentry->reserved[i] = temp_entry->reserved[i];
    // }
    // return success
    return 0;
}

/* read_data
 * 
 * DESCRIPTION: reads length bytes starting at offset from the file pointed to by
 *              the inode into the buffer that is the argument
 * 
 * INPUTS: inode: index node to the file we want to read from
 *         offset: the offset of addresses we want to start reading from
 *         buf: the buffer we want to copy data in to
 *         length: the number of bytes we want to read
 * OUTPUTS: The number of bytes successfully read
 * RETURN VALUE: int, the number of bytes read
 * SIDE EFFECTS: populates buf with bytes read from the file
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    // pointer to the inode we want to read from, +1 to skip boot block
    inode_t* inode_ptr = (inode_t*)(fs_boot_block + (inode + 1)); 


    if (offset >= inode_ptr->length) {
        return 0;
    }
    if (offset + length >= inode_ptr->length) {
        length = inode_ptr->length - offset;
    }

    uint32_t start_block = offset / FOUR_KILO;
    uint32_t end_block = (offset + length) / FOUR_KILO;
    uint32_t start_offset = offset % FOUR_KILO;
    uint32_t end_offset = (start_offset + length) - (end_block - start_block) * FOUR_KILO;
    
    

    uint32_t i;
    uint32_t length_copied = 0;
    for (i = start_block; i <= end_block; i++) {
        int32_t copy_start;
        int32_t copy_end;
        if (i == start_block) {
            copy_start = start_offset;
        } else {
            copy_start = 0;
        }
        if (i == end_block) {
            copy_end = end_offset;
        } else {
            copy_end = FOUR_KILO;
        }
        uint32_t copy_length = copy_end - copy_start;
        // pointer to the start of data we want to read
        uint8_t* data_start_pointer = 
            ((uint8_t*)( //cast
            (fs_boot_block + 1 + fs_boot_block->inode_count) // boot block + 1 is first inode, plus inode_count to skip inodes
            + (inode_ptr->data_block_num[i]))) // start at the first data block according to the inode
            + copy_start; // add the offset we want to start reading from
        memcpy(buf + length_copied, data_start_pointer, copy_length);
        length_copied += copy_length;
    }

    return length_copied; // return number of bytes read
}

/* list_filesystem
 * 
 * DESCRIPTION: prints out the filename, file size, file type of every
 *              file in file system
 * 
 * INPUTS: none
 * OUTPUTS: should print 17 lines of code
 * RETURN VALUE: none
 * SIDE EFFECTS: 
 */
void list_filesystem() {
    int i, j; // loop vars
    dentry_t * dentry;
    uint32_t size;

    for (i = 0; i < fs_boot_block->dir_count; i++) { // loop through dentries
        dentry = &(fs_boot_block->dentries[i]); // get ptr to dentry for current file

        /* gets min value between 32 and filename size*/
        size = strlen((int8_t*) dentry->filename);
        if (size > FILENAME_LEN) {
            size = FILENAME_LEN;
        }

        /* iterates through filename till size from above and prints*/
        puts("file_name: ");
        for (j=0; j<size; j++) {
            putc(dentry->filename[j]); // access filename through dentry
        }

        puts(", file_type: ");
        printf("%d", dentry->filetype); // access filetype through dentry

        size = ((inode_t*)(fs_boot_block + dentry->inode_num + 1))->length; // accesses size of file from inode
        puts(", file_size: ");
        printf("%d", size);

        putc('\n');
    }
}

/* get_file_name
 * 
 * DESCRIPTION: reads filename of the file specified by index in the file system
 * INPUTS:
 *     int index: the index of the file whose filename we are trying to get
 *     void *buf: the buffer we want to copy our results to
 * OUTPUTS: None
 * RETURN VALUE: 
 *       0 if the index is outside of valid range, otherwise return length of the filename
 *       return value is at most 32
 * SIDE EFFECTS: None
 */
int32_t get_file_name(int index, void* buf) {
    // check if index is greater than the allowed range
    if (index >= fs_boot_block->dir_count || index < 0) {
        return 0;
    }

    dentry_t * dentry;
    uint32_t size;

    dentry = &(fs_boot_block->dentries[index]); // get ptr to dentry for current file

    /* gets min value between 32 and filename size*/
    size = strlen((int8_t*) dentry->filename);
    if (size > FILENAME_LEN) {
        size = FILENAME_LEN;
    }
    memcpy(buf, dentry->filename, size);
    return size;
}

/* get_dir_inode
 * 
 * DESCRIPTION: gets the inode number of the directory file "."
 * INPUTS: None
 * OUTPUTS: None
 * RETURN VALUE: 
 *       the inode number of the directory file "."
 * SIDE EFFECTS: None
 */
int get_dir_inode(){
    return curr_dir_inode;
}
