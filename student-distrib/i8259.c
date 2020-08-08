/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Name: i8259_init()
 * Description: initializes the ports for the master and the slave used for PIC.
 * Inputs: None 
 * Outputs: None 
 * Return Value: None 
 * Side Effects: Initializes master and slave values
 */
void i8259_init(void) {
  // mask all interrupts (including slave input on master)
  outb(INTR_MASK, MASTER_PIC_DATA);
  outb(INTR_MASK, SLAVE_PIC_DATA);

  // Set ICW1, ICW2, ICW3, and ICW4 on master and slave
  outb(ICW1, MASTER_PIC_CMD);
  outb(ICW1, SLAVE_PIC_CMD);

  outb(ICW2_MASTER, MASTER_PIC_DATA);
  outb(ICW2_SLAVE, SLAVE_PIC_DATA);

  outb(ICW3_MASTER, MASTER_PIC_DATA);
  outb(ICW3_SLAVE, SLAVE_PIC_DATA);

  outb(ICW4, MASTER_PIC_DATA);
  outb(ICW4, SLAVE_PIC_DATA);

  // mask all interrupts (except for slave input on master)
  outb(MASTER_INIT_MASK, MASTER_PIC_DATA);
  outb(SLAVE_INIT_MASK, SLAVE_PIC_DATA);
  master_mask = MASTER_INIT_MASK;
  slave_mask = SLAVE_INIT_MASK;
}

/* Name: enable_irq(uint32_t irq_num)
 * Description: Enable the specific IRQ so that the interrupt can be used
 * Inputs: irq_num: the number of the interrupt that is being called
 * Outputs: Writes the interrupt active bit to the Data of Master and Slave
 * Return Value: None
 * Side Effects: IRQ is enabled for the specific interrupt and written to the
 * master and slave ports
 */
void enable_irq(uint32_t irq_num) {
  // check if the interrupt number is in bounds
  if (irq_num < 0 || irq_num > MAX_IRQ) {
    return;
  }

  uint8_t mask = 0x01;

  if (irq_num < PIC_MAX_IRQ) {
    mask = mask << irq_num;
    mask = ~mask;
    master_mask =
        master_mask &
        mask;  // assign the master_mask the value for the specific IRQ
    outb(master_mask, MASTER_PIC_DATA);
  } else {
    mask = mask << (irq_num -
                    PIC_MAX_IRQ);  // if the IRQ number is above 8, assign the
                                   // slave_mask for the specific IRQ.
    mask = ~mask;
    slave_mask = slave_mask & mask;
    outb(slave_mask, SLAVE_PIC_DATA);
  }
}

/* Name: disable_irq(uint32_t irq_num)
 * Description: Disable the specific IRQ so that the interrupt is not used
 * anymore Inputs: irq_num: the number of the interrupt that is being called
 * Outputs: Writes the interrupt inactive bit to the Data of Master and Slave
 * Return Value: None
 * Side Effects: IRQ is disabled for the specific interrupt and written to the
 * master and slave ports
 */
void disable_irq(uint32_t irq_num) {
  // check if the interrupt number is in bounds
  if (irq_num < 0 || irq_num > MAX_IRQ) {
    return;
  }

  uint8_t mask = 0x01;

  if (irq_num < PIC_MAX_IRQ) {
    mask = mask << irq_num;
    // get the correct mask for the IRQ to be disabled on master
    master_mask = master_mask | mask;
    outb(master_mask, MASTER_PIC_DATA);
  } else {
    mask = mask << (irq_num - PIC_MAX_IRQ);
    slave_mask =
        slave_mask |
        mask;  // get the correct mask for the IRQ to be disabled on the slave
    outb(slave_mask, SLAVE_PIC_DATA);
  }
}

/* Name: send_eoi(uint32_t irq_num)
 * Description: Send the end-of-interrupt signal for the specified IRQ
 * Inputs: irq_num: the number of the interrupt that is being called
 * Outputs: Sets the ports with the EOI signal
 * Return Value: None
 * Side Effects: None
 */
void send_eoi(uint32_t irq_num) {
  // check if the interrupt number is in bounds
  if (irq_num < 0 || irq_num > MAX_IRQ) {
    return;
  }

  if (irq_num < PIC_MAX_IRQ) {  // master
    outb((irq_num | EOI),
         MASTER_PIC_CMD);  // send the EOI signal to the Master port
  } else {                 // slave
    outb(((irq_num - PIC_MAX_IRQ) | EOI),
         SLAVE_PIC_CMD);  // send the EOI signal to the Slave port
    outb((SLAVE_ON_MASTER | EOI),
         MASTER_PIC_CMD);  // tells master that there is an interrupt completed
                           // on the slave port
  }
}
