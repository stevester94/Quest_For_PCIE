#! /bin/bash
$PWD/qemu/build/x86_64-softmmu/qemu-system-x86_64 --no-hpet   --snapshot -m 1024 -device e1000,netdev=n0,id=nic0  -netdev user,id=n0,hostfwd=tcp::2222-:22 -device dma_demo_device $PWD/debian8.qcow2
