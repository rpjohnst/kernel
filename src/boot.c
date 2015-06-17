#include <elf.h>
#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <stddef.h>

EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;
EFI_GUID gEfiAcpiTableGuid = EFI_ACPI_TABLE_GUID;

int memcmp(const void *aptr, const void *bptr, size_t n) {
	const unsigned char *a = aptr, *b = bptr;
	for (size_t i = 0; i < n; i++) {
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	// open the kernel file from the device this app was loaded from
	EFI_FILE *Kernel;
	{
		EFI_HANDLE_PROTOCOL HandleProtocol = SystemTable->BootServices->HandleProtocol;

		EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
		HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
		HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

		EFI_FILE *Root;
		FileSystem->OpenVolume(FileSystem, &Root);

		EFI_STATUS s = Root->Open(Root, &Kernel, L"kernel", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
		if (s != EFI_SUCCESS) {
			SystemTable->ConOut->OutputString(SystemTable->ConOut, L"kernel is missing\r\n");
			return s;
		}
	}

	EFI_ALLOCATE_POOL AllocatePool = SystemTable->BootServices->AllocatePool;

	// load the elf header from the kernel
	Elf64_Ehdr header;
	{
		UINTN FileInfoSize;
		EFI_FILE_INFO *FileInfo;
		Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, NULL);
		AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
		Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, (void**)&FileInfo);

		UINTN size = sizeof(header);
		Kernel->Read(Kernel, &size, &header);
	}

	// verify the kernel binary
	if (
		memcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		header.e_ident[EI_CLASS] != ELFCLASS64 ||
		header.e_ident[EI_DATA] != ELFDATA2LSB ||
		header.e_type != ET_EXEC ||
		header.e_machine != EM_AMD64 ||
		header.e_version != EV_CURRENT
	) {
		SystemTable->ConOut->OutputString(SystemTable->ConOut, L"kernel format is bad\r\n");
		return -1;
	}

	// load the kernel segment headers
	Elf64_Phdr *phdrs;
	{
		Kernel->SetPosition(Kernel, header.e_phoff);
		UINTN size = header.e_phnum * header.e_phentsize;
		AllocatePool(EfiLoaderData, size, (void**)&phdrs);
		Kernel->Read(Kernel, &size, phdrs);
	}

	EFI_ALLOCATE_PAGES AllocatePages = SystemTable->BootServices->AllocatePages;

	// load the actual kernel binary based on its segment headers
	for (
		Elf64_Phdr *phdr = phdrs;
		(char*)phdr < (char*)phdrs + header.e_phnum * header.e_phentsize;
		phdr = (Elf64_Phdr*)((char*)phdr + header.e_phentsize)
	) {
		switch (phdr->p_type) {
		case PT_LOAD: {
			int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000; // round up
			Elf64_Addr segment = phdr->p_paddr;
			AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);

			Kernel->SetPosition(Kernel, phdr->p_offset);
			UINTN size = phdr->p_filesz;
			Kernel->Read(Kernel, &size, (void*)segment);
			break;
		}
		}
	}

	// get the memory map from the firmware
	EFI_MEMORY_DESCRIPTOR *Map = NULL;
	UINTN MapSize, MapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	{
		EFI_GET_MEMORY_MAP GetMemoryMap = SystemTable->BootServices->GetMemoryMap;

		GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
		AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
		GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
	}

	// get the acpi tables from the firmware
	void *rsdp = NULL;
	for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *Config = &SystemTable->ConfigurationTable[i];
		if (memcmp(&Config->VendorGuid, &gEfiAcpiTableGuid, sizeof(Config->VendorGuid)) == 0) {
			rsdp = Config->VendorTable;
			break;
		}
	}

	// finish with firmware and jump to the kernel
	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	((__attribute__((sysv_abi)) void (*)(void*, size_t, size_t, void*))header.e_entry)(
		Map, MapSize, DescriptorSize,
		rsdp
	);
	return EFI_SUCCESS;
}
