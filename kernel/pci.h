#ifndef PCI_H
#define PCI_H
#include "types.h"

uint32_t read_pci_conf(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

#endif

