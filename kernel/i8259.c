/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
/* 
 * i8259_init
 *   DESCRIPTION: Initialize the 8259 pic
 *   INPUTS: none
 *   OUTPUTS: enables irq2 and irq1, and disable everything else
 *   RETURN VALUE: none
 */
void i8259_init(void) {
    printf("Initializing PIC device \n");

    master_mask = INIT_PIC_MASK; 
    slave_mask = INIT_PIC_MASK; 
 
	outb(ICW1, MASTER_8259_PORT);  // starts the initialization sequence (in cascade mode)
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_PORT | DATA_PORT_OFFSET); // ICW2 =  Master PIC vector offset 
	outb(ICW2_SLAVE, SLAVE_8259_PORT | DATA_PORT_OFFSET);   // ICW2: Slave PIC vector offset

	outb(ICW3_MASTER, MASTER_8259_PORT | DATA_PORT_OFFSET);   // ICW3 = tell Master that there is a slave PIC at IRQ2
	outb(ICW3_SLAVE, SLAVE_8259_PORT | DATA_PORT_OFFSET);   // ICW3 =  tell Slave PIC its cascade identity (0000 0010)
 
	outb(ICW4, MASTER_8259_PORT | DATA_PORT_OFFSET); // ICW4 = tell pic initialization is done
	outb(ICW4, SLAVE_8259_PORT | DATA_PORT_OFFSET);
 
	outb(master_mask, MASTER_8259_PORT | DATA_PORT_OFFSET);   // disable all irqs
	outb(slave_mask, SLAVE_8259_PORT | DATA_PORT_OFFSET); 

    enable_irq(2); // enable slave pic irq on master
    enable_irq(1); // enable nmi interrupts

}

/* Enable (unmask) the specified IRQ */
/* 
 * enable_irq
 *   DESCRIPTION: Enable (unmask) the specified IRQ
 *   INPUTS: irq_num -- which irq to enable (0-15) 
 *   OUTPUTS: enables an irq on the slave or master PIC
 *   RETURN VALUE: none
 */
void enable_irq(uint32_t irq_num) {
    if (irq_num > NUM_INTR_PORTS_IN_PIC) { // check if input is in bounds
        return;
    }

    uint16_t port;
    uint8_t value;
 
    if(irq_num < NUM_INTR_PORTS_IN_8259) { // checks if irq is on secondary or primary pic
        port = MASTER_8259_PORT | DATA_PORT_OFFSET;
        master_mask &= ~(1 << irq_num);
    } else {
        port = SLAVE_8259_PORT | DATA_PORT_OFFSET;
        irq_num -= NUM_INTR_PORTS_IN_8259;
        slave_mask &= ~(1 << irq_num);
    }
    value = inb(port) & ~(1 << irq_num);
    outb(value, port); 
}

/* Disable (mask) the specified IRQ */
/* 
 * enable_irq
 *   DESCRIPTION: Disable (mask) the specified IRQ
 *   INPUTS: irq_num -- which irq to disable (0-15)
 *   OUTPUTS: enables an irq on the slave or master PIC
 *   RETURN VALUE: none
 */
void disable_irq(uint32_t irq_num) {
    if (irq_num > NUM_INTR_PORTS_IN_PIC) { // check if input is in bounds
        return;
    }
    uint16_t port;
    uint8_t value;
 
    if(irq_num < NUM_INTR_PORTS_IN_8259) { // checks if irq is on secondary or primary pic
        port = MASTER_8259_PORT | DATA_PORT_OFFSET;
        master_mask |= (1 << irq_num);
    } else {
        port = SLAVE_8259_PORT | DATA_PORT_OFFSET;
        irq_num -= NUM_INTR_PORTS_IN_8259;
        slave_mask |= (1 << irq_num);

    }
    value = inb(port) | (1 << irq_num);
    outb(value, port);
}

/* Send end-of-interrupt signal for the specified IRQ */
/* 
 * send_eoi
 *   DESCRIPTION:Send end-of-interrupt signal for the specified IRQ
 *   INPUTS: irq num  -- which irq to enable (0-15) 
 *   OUTPUTS: Sends the eoi signal to master pic, or master and slave pic
 *   RETURN VALUE: none
 */
void send_eoi(uint32_t irq_num) {
    if (irq_num > NUM_INTR_PORTS_IN_PIC) { // check if input is in bounds
        return;
    }
    if(irq_num >= NUM_INTR_PORTS_IN_8259) { // check if an eoi to the secondary pic must be sent
		outb(EOI | (irq_num - NUM_INTR_PORTS_IN_8259), SLAVE_8259_PORT);
        irq_num = SECONDARY_IRQ_CONNECTION_TO_PRIMARY;
    }
    
	outb(EOI | irq_num, MASTER_8259_PORT);
}
