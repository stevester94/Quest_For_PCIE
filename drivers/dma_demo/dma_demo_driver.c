/*
Only tested in x86.

PCI driver for our minimal pci_min.c QEMU fork device.

probe already does a mmio write, which generates an IRQ and tests everything.
*/

#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/dma.h>
#include <inttypes.h>


#define BAR 0
#define CDEV_NAME "lkmc_hw_pci_min"
#define EDU_DEVICE_ID 0x11e9
#define QEMU_VENDOR_ID 0x1234

MODULE_LICENSE("GPL");

static struct pci_device_id id_table[] = {
    { PCI_DEVICE(QEMU_VENDOR_ID, EDU_DEVICE_ID), },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, id_table);
static int major;
static struct pci_dev *pdev;
static void __iomem *mmio;
static struct file_operations fops = {
    .owner   = THIS_MODULE,
};

// Before we are ready to handle interrupts, we need to setup our coherent DMA buffer
#define DMA_SIZE 4096 // Transfers shall be of uint8_t
#define NUM_TRANSFERS 10
static dma_addr_t dma_addr_for_device; // This is the address that the device will use
static void* dma_addr_for_kernel; // This is a virtual address, used by kernel to access the buffer
static int flags = GFP_KERNEL;
#define DMA_CHANNEL_NUM 0 // Abitrary I guess???
static uint64_t checksum = 0;
static int transfers_remaining = NUM_TRANSFERS;


// Idk about channel, just go for it
//DMA_MODE_WRITE, DMA_MODE_READ
int dma_prepare(int channel, int mode, dma_addr_t buf,
 unsigned int count)
{
    unsigned long flags;
    flags = claim_dma_lock( );
    disable_dma(channel);
    clear_dma_ff(channel);
    set_dma_mode(channel, mode);
    // set_dma_addr(channel, virt_to_bus(buf)); // Not sure about type of dma_addr
    set_dma_addr(channel, buf);
    set_dma_count(channel, count);
    enable_dma(channel);
    release_dma_lock(flags);
    return 0;
}

int dma_is_done(int channel)
{
    int residue;
    unsigned long flags = claim_dma_lock ( );
    residue = get_dma_residue(channel);
    release_dma_lock(flags);
    pr_info("Remaining items: %d", residue);

    return (residue == 0);
}

static irqreturn_t irq_handler(int irq, void *dev)
{
    pr_info("irq_handler\n");
    int i;
    uint8_t* temp = dma_addr_for_kernel;

    // if(!dma_is_done(DMA_CHANNEL_NUM))
    // {
    //     pr_info("DMA is not done!");
    // }

    for(i = 0; i < DMA_SIZE; i++)
    {
        checksum += temp[i];
    }

    transfers_remaining--;

    if(transfers_remaining > 0)
        iowrite32(0, mmio + 0x0); // Signal that we have handled the DMA buffer, and are ready for another
    else
    {
        iowrite32(0, mmio + 0x3); // Signal we have handled the DMA, but we are done
        pr_info("checksum: %"PRIu64"\n", checksum);
    }
    return IRQ_HANDLED;
}


static int probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    pr_info("probe\n");
    major = register_chrdev(0, CDEV_NAME, &fops);
    pdev = dev;
    int ret_code;
    int i;

    if (pci_enable_device(dev) < 0) {
        dev_err(&(pdev->dev), "pci_enable_device\n");
        goto error;
    }
    if (pci_request_region(dev, BAR, "myregion0")) {
        dev_err(&(pdev->dev), "pci_request_region\n");
        goto error;
    }
    mmio = pci_iomap(pdev, BAR, pci_resource_len(pdev, BAR));
    pr_info("dev->irq = %u\n", dev->irq);

    if(dma_set_coherent_mask(&dev->dev, 0xFFFFFFF) != 0)
    {
        pr_info("Error setting mask!\n");
    }

    pci_set_master(dev); // Per some stack overflow guy, looks like it works!
    dma_addr_for_kernel = dma_alloc_coherent(&dev->dev, size_buffer, &dma_addr_for_device, flags);

    pr_info("dma_addr_for_device: %u\n", (uint32_t)dma_addr_for_device);

    dma_prepare(DMA_CHANNEL_NUM, DMA_MODE_READ, dma_addr_for_device, DMA_SIZE); // Question is do we need to call this guy each time

    if (request_irq(dev->irq, irq_handler, IRQF_SHARED, "pci_irq_handler0", &major) < 0) {
        dev_err(&(dev->dev), "request_irq\n");
        goto error;
    }
    iowrite32(dma_addr_for_device, mmio+0x1); // initialize device with DMA address
    iowrite32(NUM_TRANSFERS, mmio+0x2);

    return 0;
error:
    return 1;
}

static void remove(struct pci_dev *dev)
{
    pr_info("remove\n");
    dma_free_coherent(&dev->dev, size_buffer, dma_addr_for_kernel, dma_addr_for_device);
    free_irq(dev->irq, &major);
    pci_release_region(dev, BAR);
    unregister_chrdev(major, CDEV_NAME);
}

static struct pci_driver pci_driver = {
    .name     = CDEV_NAME,
    .id_table = id_table,
    .probe    = probe,
    .remove   = remove,
};

static int myinit(void)
{
    if (pci_register_driver(&pci_driver) < 0) {
        return 1;
    }
    return 0;
}

static void myexit(void)
{
    pci_unregister_driver(&pci_driver);
}

module_init(myinit);
module_exit(myexit);
