#! /bin/bash
$PWD/qemu/build/x86_64-softmmu/qemu-system-x86_64    -m 1024 -device e1000,netdev=n0,id=nic0  -netdev user,id=n0,hostfwd=tcp::2222-:22 -device dma_demo_device $PWD/debian8.qcow2

#/home/smackey/qemu.built/build/x86_64-softmmu/qemu-system-x86_64    -device edu -enable-kvm -m 1024 -device e1000,netdev=n0,id=nic0  -netdev user,id=n0,hostfwd=tcp::2222-:22 /home/smackey/QEMU_Testing/debian8.qcow2
