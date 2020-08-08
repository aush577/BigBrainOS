/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _I8259_H
#define _I8259_H

#include "types.h"
/* --- Constant / Literal Definitions --- */

/* Ports that each PIC sits on */
#define MASTER_PIC_CMD 0x20   // for command
#define MASTER_PIC_DATA 0x21  // for data
#define SLAVE_PIC_CMD 0xA0    // for command
#define SLAVE_PIC_DATA 0xA1   // for data

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1 0x11
#define ICW2_MASTER 0x20
#define ICW2_SLAVE 0x28
#define ICW3_MASTER 0x04
#define ICW3_SLAVE 0x02
#define ICW4 0x01

#define MASTER_INIT_MASK 0xFB
#define SLAVE_INIT_MASK 0xFF
#define INTR_MASK   0xFF

#define SLAVE_ON_MASTER 0x02  // Which irq the slave is connected to on master
#define MAX_IRQ 15            // Number of irqs on a PIC
#define PIC_MAX_IRQ 8

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI 0x60

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);


#endif /* _I8259_H */
