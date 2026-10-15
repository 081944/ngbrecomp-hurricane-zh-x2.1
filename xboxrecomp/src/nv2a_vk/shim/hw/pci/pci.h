#ifndef NV2A_SHIM_PCI_H
#define NV2A_SHIM_PCI_H
#include "qemu/osdep.h"
#include "exec/memory.h"
/* Config space only: PBUS mirrors it at 0x1800. */
typedef struct PCIDevice { uint8_t config[256]; } PCIDevice;
typedef void *qemu_irq;
typedef struct PCIBus PCIBus;
#define PCI_DEVICE(obj) (&(obj)->parent_obj)
#define PCI_VENDOR_ID       0x00
#define PCI_COMMAND         0x04
#define PCI_CLASS_REVISION  0x08
static inline uint32_t pci_get_long(const uint8_t *config) { return ldl_le_p(config); }
static inline void pci_set_long(uint8_t *config, uint32_t val) { stl_le_p(config, val); }
#endif
