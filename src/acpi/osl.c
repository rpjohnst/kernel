#include <cache.h>
#include <paging.h>
#include <acpi/acpi.h>
#include <kprintf.h>

/// environment and tables

ACPI_STATUS AcpiOsInitialize() {
	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() {
	return AE_OK;
}

ACPI_TABLE_RSDP *AcpiOsRsdp;
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
	return (ACPI_PHYSICAL_ADDRESS)AcpiOsRsdp;
}

ACPI_STATUS AcpiOsPredefinedOverride(
	const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue
) {
	*NewValue = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
	*NewTable = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(
	ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength
) {
	*NewAddress = 0;
	return AE_OK;
}

/// memory

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
	return VIRT_DIRECT(PhysicalAddress);
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
	uintptr_t virt = (uintptr_t) LogicalAddress;
	if (virt >= KERNEL_BASE)
		*PhysicalAddress = PHYS_KERNEL(LogicalAddress);
	else
		*PhysicalAddress = PHYS_DIRECT(LogicalAddress);

	return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size) {
	return kmalloc(Size);
}

void AcpiOsFree(void *Memory) {
	kfree(Memory);
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
	return TRUE;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
	return TRUE;
}

/// threads

ACPI_THREAD_ID AcpiOsGetThreadId() {
	return 0;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
	//Function(Context);
	return AE_NOT_IMPLEMENTED;
}

void AcpiOsSleep(UINT64 Milliseconds) {}

void AcpiOsStall(UINT32 Milliseconds) {}

void AcpiOsWaitEventsComplete(void) {
	kprintf("acpi waiting\n");
	for (;;);
}

/// synchronization

/*ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {}*/

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
	return AE_NOT_IMPLEMENTED;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) {}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {}

/// interrupt handling

ACPI_STATUS AcpiOsInstallInterruptHandler(
	UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context
) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler) {
	return AE_NOT_IMPLEMENTED;
}

/// memory

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
	return AE_NOT_IMPLEMENTED;
}

/// port I/O

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
	return AE_NOT_IMPLEMENTED;
}

/// pci

ACPI_STATUS AcpiOsReadPciConfiguration(
	ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width
) {
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(
	ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width
) {
	return AE_NOT_IMPLEMENTED;
}

/// formatted output

void AcpiOsPrintf(const char *Format, ...) {
	va_list ap;
	va_start(ap, Format);
	kvprintf(Format, ap);
	va_end(ap);
}

void AcpiOsVprintf(const char *Format, va_list ap) {
	kvprintf(Format, ap);
}

/// misc

UINT64 AcpiOsGetTimer(void) {
	return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
	return AE_NOT_IMPLEMENTED;
}
