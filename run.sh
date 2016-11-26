make -j && qemu-system-x86_64 -enable-kvm -s \
	-machine pc-q35-2.7 -cpu host,migratable=no,+invtsc -smp 4 -m 4G \
	-drive if=pflash,format=raw,readonly,file=edk2/Build/OvmfX64/RELEASE_GCC5/FV/OVMF.fd \
	-drive format=raw,file=fat:img
