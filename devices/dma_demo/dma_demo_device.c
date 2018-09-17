#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include <inttypes.h>


#define DEVICE_NAME "dma_demo_device"

typedef struct {
    PCIDevice pdev;
    MemoryRegion mmio;
} State;

static dma_addr_t dma_address;
static uint64_t dma_mask = 0xFFFFFFF;
#define DMA_SIZE 4096 // Transfers shall be of uint8_t

static int num_transfers = -1;
static uint64_t checksum = 0;

static uint64_t start_time, end_time;

static uint64_t mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    printf(DEVICE_NAME " mmio_read addr = %llx size = %x", (unsigned long long)addr, size);
    return 0x1234567812345678;
}


static void write_to_dma(State* state)
{
    int i;
    uint8_t buffer[DMA_SIZE];
    uint8_t random = rand() % 0xFF;
    num_transfers--;
    for(i = 0; i < DMA_SIZE; i++)
    {
        buffer[i] = random;
        checksum += random;
    }
    // printf("Writing to DMA: %d\n", (int)random);
    pci_dma_write(&state->pdev, dma_address & dma_mask, buffer, DMA_SIZE);
}
static uint64_t get_epoch_ms()
{
    uint64_t            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    ms = spec.tv_sec * 1000;

    return ms;
}
static void mmio_write(void *opaque, hwaddr addr, uint64_t val,
        unsigned size)
{
    State *state = opaque;

    switch (addr) {

        case 0: // Indicates from host machine that DMA has been handled (copied or whatever), ready for next one
            pci_set_irq(&state->pdev, 0);
            if(num_transfers < 0)
            {
                printf("Error, number of remaining transfers is negative!\n");
            }
            if(num_transfers > 0)
            {
                write_to_dma(state);
                pci_set_irq(&state->pdev, 1);
            }
        break;
        case 4: // This will be the DMA address
            dma_address = (dma_addr_t)val;
            printf("dma_address: %d\n", (uint32_t)(dma_address & dma_mask));
        break;
        case 8: 
            // This will indicate how many DMAs we are going to attempt
            // It will also trigger the first dma write
            // subsequent writes will be made once we get the ack (case 0)
            checksum = 0;
            num_transfers = val;
            printf("Attempting %d transfers\n", num_transfers);
            start_time =  get_epoch_ms();
            write_to_dma(state);
            pci_set_irq(&state->pdev, 1); // Indicate DMA is complete, ready to be handled
        break;
        case 12: // This indicates the host is done with DMA, we should be as well, verify
            pci_set_irq(&state->pdev, 0);
            if(num_transfers != 0)
            {
                printf("We should be done, but have %d transfers remaing\n", num_transfers);
            }
            printf("checksum: %"PRIu64"\n", checksum);
            end_time = get_epoch_ms();
            printf("Done at %" PRIu64 "\n", end_time);
            printf("Effective data rate: %f Bytes/sec\n", ((float)num_transfers)*DMA_SIZE / ((end_time-start_time)/1000));
        break;
    }
}

// static void mmio_write(void *opaque, hwaddr addr, uint64_t val,
//         unsigned size)
// {
//     printf("Addr: %d Val: %"PRIu64"\n", (int)addr, val);
// }

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
            DEVICE_NAME, 1 << 20);
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
