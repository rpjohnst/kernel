make -j && qemu-system-x86_64 -s -m 5G \
	-drive if=pflash,format=raw,readonly,file=OVMF.fd \
	-drive format=raw,file=fat:img
