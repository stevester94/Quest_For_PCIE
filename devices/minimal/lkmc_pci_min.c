#include "qemu/osdep.h"
#include "hw/pci/pci.h"

#define DEVICE_NAME "lkmc_pci_min"

typedef struct {
    PCIDevice pdev;
    MemoryRegion mmio;
} State;

static uint64_t mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    printf(DEVICE_NAME " mmio_read addr = %llx size = %x", (unsigned long long)addr, size);
    return 0x1234567812345678;
}

static void mmio_write(void *opaque, hwaddr addr, uint64_t val,
        unsigned size)
{
    State *state = opaque;

    printf(DEVICE_NAME " mmio_read addr = %llx val = %llx size = %x\n",
            (unsigned long long)addr, (unsigned long long)val, size);
    switch (addr) {
        case 0x0:
            pci_set_irq(&state->pdev, 1);
            break;
        case 0x4:
            pci_set_irq(&state->pdev, 0);
            break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read = mmio_read,
    .write = mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void realize(PCIDevice *pdev, Error **errp)
{
    State *state = DO_UPCAST(State, pdev, pdev);

    pci_config_set_interrupt_pin(pdev->config, 1);
    memory_region_init_io(&state->mmio, OBJECT(state), &mmio_ops, state,
            DEVICE_NAME, 8);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &state->mmio);
}

static void class_init(ObjectClass *class, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = realize;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = 0x11e9;
    k->revision = 0x0;
    k->class_id = PCI_CLASS_OTHERS;
}

static InterfaceInfo interfaces[] = {
    { INTERFACE_CONVENTIONAL_PCI_DEVICE },
    { },
};
static const TypeInfo type_info = {
    .name          = DEVICE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(State),
    .class_init    = class_init,
    .interfaces    = interfaces
};

static void register_types(void)
{
    type_register_static(&type_info);
}

type_init(register_types)
