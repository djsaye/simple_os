#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

#define NUM_ENTRIES 1024
#define KERNEL_ADDR 0x400
#define VIDEO_ADDR 0xb8
#define VIDEO_IDX 0xb8

#define TERM1_VIDEO_ADDR 0xb9
#define TERM1_VIDEO_IDX 0xb9

#define TERM2_VIDEO_ADDR 0xba
#define TERM2_VIDEO_IDX 0xba

#define TERM3_VIDEO_ADDR 0xbb
#define TERM3_VIDEO_IDX 0xbb

#define USER_VIDEO_PDE_IDX 33

/* This struct has the info for a page directory entry */
typedef struct page_dir_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present                : 1;
            uint32_t read_write             : 1;
            uint32_t user_supervisor        : 1;
            uint32_t page_writethrough      : 1;
            uint32_t page_cache_disabled    : 1;
            uint32_t accessed               : 1;
            uint32_t reserved               : 1;
            uint32_t page_size              : 1;
            uint32_t global                 : 1;
            uint32_t avail                  : 3;
            uint32_t page_table_base_addr   :20;
        } __attribute__ ((packed));
    };
} page_dir_entry_t;

/* This struct has the contents for page table entry */
typedef struct page_table_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present                : 1;
            uint32_t read_write             : 1;
            uint32_t user_supervisor        : 1;
            uint32_t page_writethrough      : 1;
            uint32_t page_cache_disabled    : 1;
            uint32_t accessed               : 1;
            uint32_t dirty                  : 1;
            uint32_t page_attr_tab_idx      : 1;    
            uint32_t global                 : 1;
            uint32_t avail                  : 3;
            uint32_t page_base_address      :20;
        } __attribute__ ((packed));
    };
} page_table_entry_t;

/* The page directory, defined not here, but in .S file */
extern page_dir_entry_t page_directory[1024] __attribute__((aligned(4096)));
/* The page table, defined in .S file */
extern page_table_entry_t page_table[1024] __attribute__((aligned(4096)));
/* Page Table for video mapping*/
extern page_table_entry_t video_map_table[1024] __attribute__((aligned(4096)));



/* enable_paging
 * 
 * DESCRIPTION: This function will enable paging on the CPU
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: will enable paging on the CPU, and set the CPU page directory address to page_directory
 */
extern void enable_paging();

/* init_paging
 * 
 * DESCRIPTION: This function initializes the page table and page
 *              directory for proper paging. It intializes a page
 *              directory entry for the kernel, the page table,
 *              and a page table entry for the video memory
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: initializes page_directory and page_table properly
 */
void init_paging();

/* flush_tlb
 * 
 * DESCRIPTION: flushes the TLB
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: flushes the TLB
 */
extern void flush_tlb();

#endif /* _x86_DESC_H */
