
#define NUM_PDE 1024 /* number of page directory entries */
#define NUM_PTE 1024 /* number of page table entries */

.text
# make functions and page table/directory global, other files can access them
.globl page_directory, page_table, video_map_table
.globl enable_paging, flush_tlb

# align function properly
.align 4

/* enable_paging
 * 
 * DESCRIPTION: This function will enable paging on the CPU
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: will enable paging on the CPU, and set the CPU page directory address to page_directory
 */
enable_paging:
    pushl %ebp
    # setup stack
    movl %esp, %ebp
    pushl %eax

    # using enabling strategy from https://wiki.osdev.org/Paging

    # move the page directory address into cr3, control register with page dir address
    movl $page_directory, %eax
    mov %eax, %cr3

    # enable page size extension using CR4 bit 1
    mov %cr4, %eax
    or $0x00000010, %eax
    mov %eax, %cr4
    
    # enable paging by setting PG flag in CR0
    mov %cr0, %eax
    orl $0x80000000, %eax
    mov %eax, %cr0

    # tear down stack and return
    popl %eax
    leave
    ret

/* flush_tlb
 * 
 * DESCRIPTION: flushes the TLB
 * 
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: flushes the TLB
 */
flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret


.align 4096 # align directory properly
page_directory:
    # page directory
    .rept NUM_PDE # 1024 entires
    .long 0x0 # initialize them to 0 for now
    .endr

.align 4096
page_table:
    # page table
    .rept NUM_PTE # 1024 entries
    .long 0x0 # initialize them to 0
    .endr

.align 4096
video_map_table:
    # video map page table
    .rept NUM_PTE # 1024 entries
    .long 0x0 # initialize them to 0
    .endr

