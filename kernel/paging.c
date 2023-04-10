
#include "paging.h"
#include "lib.h"

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
void init_paging(){

    int i;

    for(i = 0; i < NUM_ENTRIES; i ++){ // initialize all of the page directory / table
        page_directory[i].present               = 0; // all are not present, except ones we set
        page_directory[i].read_write            = 1; // want to read/write
        page_directory[i].user_supervisor       = 0; // want to be in supervisor mode
        page_directory[i].page_writethrough     = 0;
        page_directory[i].page_cache_disabled   = 0;
        page_directory[i].accessed              = 0;
        page_directory[i].reserved              = 0;
        page_directory[i].page_size             = 0;
        page_directory[i].global                = 0;
        page_directory[i].avail                 = 0;
        page_directory[i].page_table_base_addr  = 0;

        page_table[i].present              = 0; // nothing is present
        page_table[i].read_write           = 1; // want to read/write
        page_table[i].user_supervisor      = 0; // supervisor mode
        page_table[i].page_writethrough    = 0;
        page_table[i].page_cache_disabled  = 0;
        page_table[i].accessed             = 0;
        page_table[i].dirty                = 0;
        page_table[i].page_attr_tab_idx    = 0;
        page_table[i].global               = 0;
        page_table[i].avail                = 0;
        page_table[i].page_base_address    = i; // base address of each 4kb page corresponds to i, direct mapping
        

        video_map_table[i].present              = 0; // nothing is present
        video_map_table[i].read_write           = 1; // want to read/write
        video_map_table[i].user_supervisor      = 1; // user should be able to read this
        video_map_table[i].page_writethrough    = 0;
        video_map_table[i].page_cache_disabled  = 0;
        video_map_table[i].accessed             = 0;
        video_map_table[i].dirty                = 0;
        video_map_table[i].page_attr_tab_idx    = 0;
        video_map_table[i].global               = 0;
        video_map_table[i].avail                = 0;
        video_map_table[i].page_base_address    = i; // base address of each 4kb page corresponds to i, direct mapping
    }

    // first entry in page directory is page table
    page_directory[0].present = 1; // set first page dir entry to present
    page_directory[0].page_table_base_addr = ((int)page_table) >> 12; // shift by 12 to have value fit in struct
    page_directory[0].page_size = 0; // these will be 4 kb pages

    // set up page directory entry for kernel
    page_directory[1].present = 1; // set second page dir entry to present
    page_directory[1].page_table_base_addr = KERNEL_ADDR;
    page_directory[1].page_size = 1; // this is a 4 MB page

    // // set up page directory entry for first exe
    // page_directory[32].present = 1;
    // page_directory[32].page_table_base_addr = 0x800;
    // page_directory[32].page_size = 1; // this is a 4 MB page

    // video memory
    page_table[VIDEO_IDX].present = 1; // video memory is present
    page_table[VIDEO_IDX].read_write = 1; // we can read/write to it

    page_table[TERM1_VIDEO_ADDR].present = 1; // video memory is present
    page_table[TERM1_VIDEO_ADDR].read_write = 1; // we can read/write to it

    page_table[TERM2_VIDEO_ADDR].present = 1; // video memory is present
    page_table[TERM2_VIDEO_ADDR].read_write = 1; // we can read/write to it

    page_table[TERM3_VIDEO_ADDR].present = 1; // video memory is present
    page_table[TERM3_VIDEO_ADDR].read_write = 1; // we can read/write to it

    // USER_VIDEO_PDE_INDEX is 33, since program image ends at 132 MB and this is where
    // the next page will start. index 33 corresponds to 132 MB in virtual space. 
    // initialize (BUT DON'T ALLOCATE) this memory region
    page_directory[USER_VIDEO_PDE_IDX].present = 0;
    page_directory[USER_VIDEO_PDE_IDX].read_write = 1;
    page_directory[USER_VIDEO_PDE_IDX].user_supervisor = 1; // user should be able to read this
    page_directory[USER_VIDEO_PDE_IDX].page_table_base_addr = ((int)video_map_table) >> 12; // PT for video map
    page_directory[USER_VIDEO_PDE_IDX].page_size = 0; 

    // // 1018 seems to be the page where the network card will be on
    page_directory[1018].page_table_base_addr = 0xFE800;
    page_directory[1018].read_write = 1;
    page_directory[1018].user_supervisor = 0;
    page_directory[1018].page_size = 1; 
    page_directory[1018].present = 1;

    page_directory[31].page_table_base_addr = 0x7c00;
    page_directory[31].read_write = 1;
    page_directory[31].user_supervisor = 0;
    page_directory[31].page_size = 1; 
    page_directory[31].present = 1;
}
