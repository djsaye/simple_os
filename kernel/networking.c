#include "networking.h"
#include "types.h"
#include "pci.h"
#include "lib.h"
#include "outl.h"
#include "i8259.h"
#include "paging.h"

static volatile ethernet_card_t card;
static  uint32_t next_avail_mem;
static uint32_t flag[3];
static uint32_t terminal_idx = 0;
static uint8_t * eth_buffers[3];

static ethernet_frame_t test_packet = {
    .preamble = {10,10,10,10,10,10,10},
    .sfd = 0xAB,
    .dest_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    .src_mac = {0},
    .length = 73,
    .data = {0},
    //.crc = 0x8f8d61c8
};

// static transmitter_desc_t  * tx_buf[E1000_NUM_TX_DESC];
// static receiver_desc_t * rx_buf[E1000_NUM_RX_DESC];

void * eth_malloc(uint32_t size) {
    next_avail_mem += size;
    return (void *) (next_avail_mem - size);
}

int init_ethernet_config_from_pci() {
    // qemu tells us it's
    // bus 0, device 3, function 0
    // bar0 has offset 0x10

    int32_t bar0 = read_pci_conf(0, 3, 0, 0x10);
    int32_t pde_index;
    int i = 0;
    // fetch the type of bar0; this is almost defintely 0
    card.bar0_type = bar0 & 1;
    // fetch the base of bar0, which just forces the last 4 bits to 0
    card.mem_base_addr = bar0 & 0xFFFFFFF0;
    next_avail_mem = 0x07c00000;

    int32_t status_command = read_pci_conf(0, 3, 0, 0x4);

    card.has_eeprom = detect_eeprom();
    // assign the memory space

    // read mac address

    uint32_t temp;
    temp = read_from_eeprom(0);
    card.mac[0] = temp & 0xFF;
    card.mac[1] = temp >> 8;
    temp = read_from_eeprom(1);
    card.mac[2] = temp & 0xFF;
    card.mac[3] = temp >> 8;
    temp = read_from_eeprom(2);
    card.mac[4] = temp &0xff;
    card.mac[5] = temp >> 8;

    volatile_write(0, volatile_read(0) | 0x40);

    for(i = 0; i < 0x80; i++)
        volatile_write(0x5200 + i*4, 0);


    // this is the last 32 bits of ethernet pci conf
    uint32_t pci_last_line = read_pci_conf(0, 3, 0, 0x3C);
    // and the last 8 bits of that gives us the irq number
    card.irq_num = pci_last_line & 0xFF;

    volatile_write(0x00D0, 0x1F6DC);
    volatile_write(0x00D0, 0xFF & ~4);
    volatile_read(0xC0);   
    enable_irq(11);

    receiver_init();
    trasmitter_init();

    return 0;

}

int detect_eeprom() {
    int i;
    uint32_t read_result;
    volatile_write(REG_EEPROM, 0x1);
    for (i = 0; i < 1000; i++) {
        read_result = volatile_read(REG_EEPROM);
        if (read_result & 0x10) {
            return 1;
        }
    }
    return 0;
}

uint32_t read_from_eeprom(uint32_t addr) {
    uint32_t data;
    uint32_t temp;

    volatile_write(REG_EEPROM, (1) | (addr << 8));
    while (!((temp =volatile_read(REG_EEPROM)) & (1 << 4)));
    data = (temp >> 16) & 0xFFFF;
    return data;
}

// both volatile read and write operates relative to the mem base addr
// they communicates with the network card
void volatile_write(uint32_t addr, uint32_t val) {
    *((volatile uint32_t*)(addr + card.mem_base_addr)) = val;
}

void volatile_write_direct(uint32_t addr, uint32_t val) {
    *((volatile uint32_t*)(addr)) = val;
}

int32_t volatile_read(uint32_t addr) {
    return *((volatile uint32_t*)(addr + card.mem_base_addr));
}

void ethernet_handler() {
    
    printf("packet received : ");
    volatile_write(REG_IMASK, 0x1);
    uint32_t status = volatile_read(0xc0);
    printf("%d", status);
    uint32_t txaddr = volatile_read(REG_TXDESCLO);

    if (status & 1) {
        printf("processed rs\n");
    }
    
    if (status & 0x02) {
        printf(" status queue is empty\n");
    }
    else if(status & 0x04)
    {
        //startLink();
    }
    else if(status & 0x10)
    {
        // good threshold
    }
    else if(status & 0x80)
    {
        printf(" reception ");
        handle_receive();
    }
    // printf(" finish handler\n");
    send_eoi(11);
}


// fd = p_address
int read_from_i217(int fd, void * buf, int nbytes) {
    // outl_asm(fd, card.bar0);
    // memset(buf, inl(card.bar0 + 4), nbytes);
    

    // memcpy(buf, (void *) (card.mem_base_addr + fd), nbytes);
    return 0;
    
}

// fd = p_address, buf = pointer to p_value
int write_to_i217(int fd, const void * buf, int nbytes) {
    // outl_asm(fd, card.bar0);
    // outl_asm(*((uint32_t *) buf), card.bar0 + 4);

    // memcpy((void *) (card.mem_base_addr + fd), buf, nbytes);
    return 0;
}

int read_mac_addr() {
    // if (card.has_eeprom)
    // {
    //     uint32_t temp;
    //     temp = eepromRead( 0);
    //     card.mac[0] = temp &0xff;
    //     card.mac[1] = temp >> 8;
    //     temp = eepromRead( 1);
    //     card.mac[2] = temp &0xff;
    //     card.mac[3] = temp >> 8;
    //     temp = eepromRead( 2);
    //     card.mac[4] = temp &0xff;
    //     card.mac[5] = temp >> 8;
    // } else {
    //     uint8_t * mac_addr_8 = (uint8_t *) (card.mem_base_addr+0x5400);
    //     uint32_t * mac_addr_32 = (uint32_t *) (card.mem_base_addr+0x5400);
    //     if ( mac_addr_32[0] != 0 )
    //     {
    //         for(i = 0; i < 6; i++)
    //         {
    //             card.mac[i] = mac_addr_8[i];
    //         }
    //     }
    //     else return 1;
    // }
    return 0;
}

void receiver_init() {
    uint8_t * ptr;
    receiver_desc_t * descs;
    int i;

    uint32_t size = sizeof(receiver_desc_t) * E1000_NUM_RX_DESC + 16;
    ptr =  ((uint8_t *) eth_malloc(size));

    descs = (receiver_desc_t *) (ptr);
    for(i = 0; i < E1000_NUM_RX_DESC; i++) {
        card.r_desc_ptrs[i] = (receiver_desc_t *)((uint8_t *)descs + i*16);
        card.r_desc_ptrs[i]->address[1] = 0;
        card.r_desc_ptrs[i]->address[0] = (uint32_t)(uint8_t *)(eth_malloc(8192 + 16));
        card.r_desc_ptrs[i]->status = 0;
    }

    volatile_write(REG_TXDESCLO, (uint32_t) ptr);
    volatile_write(REG_TXDESCHI, 0);

    volatile_write(REG_RXDESCLO, (uint32_t)ptr);
    volatile_write(REG_RXDESCHI, 0);

    volatile_write(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    volatile_write(REG_RXDESCHEAD, 0);
    volatile_write(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
    card.r_cur = 0;
    volatile_write(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_8192);

}

void trasmitter_init() {
    uint8_t * ptr;
    transmitter_desc_t * descs;
    int i;

    uint32_t size = sizeof(transmitter_desc_t) * E1000_NUM_TX_DESC + 16;
    ptr =  ((uint8_t *) eth_malloc(size));

    descs = (transmitter_desc_t *) (ptr);
    for(i = 0; i < E1000_NUM_TX_DESC; i++) {
        card.t_desc_ptrs[i] = (transmitter_desc_t *)((uint8_t*)descs + i*16);
        card.t_desc_ptrs[i]->address[0] = 0;
        card.t_desc_ptrs[i]->address[1] = 0;
        card.t_desc_ptrs[i]->cmd = 0;
        //card.t_desc_ptrs[i]->status = TSTA_DD;
    }

    volatile_write(REG_TXDESCHI, 0 );
    volatile_write(REG_TXDESCLO, (uint32_t)ptr);

    //now setup total length of descriptors
    volatile_write(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);

    //setup numbers
    volatile_write( REG_TXDESCHEAD, 0);
    volatile_write( REG_TXDESCTAIL, 0);
    card.t_cur = 0;
    volatile_write(REG_TCTRL,  TCTL_EN | TCTL_PSP);

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards 
    // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    // volatile_write(REG_TCTRL,  0b0110000000000111111000011111010);
    // volatile_write(REG_TIPG,  0x0060200A);


}

int send_packet(const void * p_data, uint16_t p_len)
{    
    //ethernet_frame_t frame;
    //memcpy(&frame, p_data, sizeof(ethernet_frame_t));
    // card.t_desc_ptrs[card.t_cur]->address[1] = (uint32_t)(&frame);
    // volatile_write_direct((uint32_t) (&(card.t_desc_ptrs[card.t_cur]->address[1])), (uint32_t)(&test_packet));
    // volatile_write_direct((uint32_t) (&(card.t_desc_ptrs[card.t_cur]->length)), (uint32_t)(&test_packet));
    // volatile_write_direct((uint32_t) (&(card.t_desc_ptrs[card.t_cur]->cmd)), CMD_EOP | CMD_IFCS | CMD_RS);
    // volatile_write_direct((uint32_t) (&(card.t_desc_ptrs[card.t_cur]->status)), 0);
    card.t_desc_ptrs[card.t_cur]->address[0] = (uint32_t)(p_data);
    card.t_desc_ptrs[card.t_cur]->length = p_len;
    card.t_desc_ptrs[card.t_cur]->cmd = CMD_EOP | CMD_RS;
    card.t_desc_ptrs[card.t_cur]->status = 0;
    uint8_t old_cur = card.t_cur;   
    card.t_cur = (card.t_cur + 1) % E1000_NUM_TX_DESC;
    volatile_write(REG_TXDESCTAIL, card.t_cur);
    uint32_t tail = volatile_read(REG_TXDESCTAIL);   
    while(!(card.t_desc_ptrs[old_cur]->status & 1)) {
       // printf("%d", (card.t_desc_ptrs[old_cur]->status));
    }    
    return 0;
}

int first_test_send() {
    
    int i =0;
    for (i=0; i<6; i++) {
        test_packet.src_mac[i] = card.mac[i];
    }

    for (i=0; i<47; i++) {
        test_packet.data[i] = (uint8_t)'A';
    }
    for (i = 0; i < 1000; i++){
        send_packet(&test_packet, 73);
    }
    return 0;
}

void handle_receive() {
    uint16_t old_cur;
    int got_packet = 0;
    while((card.r_desc_ptrs[card.r_cur]->status & 0x1))
    {
            got_packet = 1;
            uint8_t *buf = (uint8_t *)card.r_desc_ptrs[card.r_cur]->address;
            uint16_t len = card.r_desc_ptrs[card.r_cur]->length;
            // Here you should inject the received packet into your network stack
            card.r_desc_ptrs[card.r_cur]->status = 0;
            old_cur = card.r_cur;
            card.r_cur = (card.r_cur + 1) % E1000_NUM_RX_DESC;
            volatile_write(REG_RXDESCTAIL, old_cur );
    }    
}

int eth_read(int fd, void * buf, int nbytes) {
    if (flag[terminal_idx] == 2) {
        uint32_t size = strlen(eth_buffers[terminal_idx]);
        memcpy(buf, eth_buffers[terminal_idx], size);
        flag[terminal_idx] = 0;
        return size;
    }

    if (flag[terminal_idx] == 1)
        return 0;
    
    memcpy(buf, card.mac, sizeof(uint8_t) * 6);
    flag[terminal_idx] = 1;
    return (sizeof(uint8_t) * 6);
}
int eth_write(int fd, const void * buf, int nbytes) {
    first_test_send();
    flag[terminal_idx] = 2;
    return 0;
}
int eth_open(const unsigned char * filename) {
    flag[terminal_idx] = 0;
    return 0;
}
int eth_close(int fd) {
    flag[terminal_idx] = 1;
    return 0;
}

void eth_terminal_switch(int terminal_num) {
    terminal_idx = terminal_num;
}

