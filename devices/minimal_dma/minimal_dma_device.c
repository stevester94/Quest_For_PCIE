#include "qemu/osdep.h"
#include "hw/pci/pci.h"

#define DEVICE_NAME "minimal_dma"

typedef struct {
    PCIDevice pdev;
    MemoryRegion mmio;
} State;

static dma_addr_t dma_address;
static char buffer[100];
static uint64_t dma_mask = 0xFFFFFFF;

static uint64_t mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    printf(DEVICE_NAME " mmio_read addr = %llx size = %x", (unsigned long long)addr, size);
    return 0x1234567812345678;
}

// static inline int pci_dma_write(PCIDevice *dev, dma_addr_t addr,
//                                 const void *buf, dma_addr_t len)


static void write_to_dma(State* state)
{
    char str[800];
    char temp2[10];
    str[0] = '\0';
    unsigned int i;

    pci_dma_write(&state->pdev, dma_address & dma_mask, buffer, 100);
    // pci_dma_read(&state->pdev, dma_address & dma_mask, buffer, 100);
    // cpu_physical_memory_write(dma_address & dma_mask, buffer, 100); / I think this is cheating

    printf("dma_address: %d\n", (uint32_t)(dma_address & dma_mask));
    for(i = 0; i < 100; i++)
    {
        sprintf(temp2, "%d, ", (int)buffer[i]);
        strcat(str, temp2);
    }
    printf("%s\n", str);

}

static void mmio_write(void *opaque, hwaddr addr, uint64_t val,
        unsigned size)
{
    State *state = opaque;

    printf(DEVICE_NAME " mmio_write addr = %llx val = %llx size = %x\n",
            (unsigned long long)addr, (unsigned long long)val, size);
    switch (val) {

        case 0x0: // Indicates from host machine that DMA has been handled (copied or whatever)
            pci_set_irq(&state->pdev, 0);
            break;
        default: // This will be any non-zero value, containing the DMA address
            dma_address = (dma_addr_t)val;
            write_to_dma(state);
            pci_set_irq(&state->pdev, 1); // Indicate DMA is complete, ready to be handled
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
    unsigned int i;
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = realize;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = 0x11e9;
    k->revision = 0x0;
    k->class_id = PCI_CLASS_OTHERS;

    for(i = 0; i < 100; i++)
    {
        buffer[i] = (char)i;
    }


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
