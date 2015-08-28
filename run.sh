make -j && qemu-system-x86_64 -enable-kvm -s \
	-smp 4 -m 4G \
	-drive if=pflash,format=raw,readonly,file=OVMF.fd \
	-drive format=raw,file=fat:img
