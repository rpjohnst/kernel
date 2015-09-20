#include "parse.h"
#include "../apic.h"
#include "../hpet.h"
#include <assert.h>
#include <stdbool.h>

void acpi_parse(ACPI_TABLE_RSDP *Rsdp) {
	extern ACPI_TABLE_RSDP *AcpiOsRsdp;
	AcpiOsRsdp = Rsdp;

#if 0
	ACPI_STATUS status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status))
		panic("acpi error %d\n", status);
#endif

	ACPI_STATUS status = AcpiInitializeTables(NULL, 128, true);
	if (ACPI_FAILURE(status)) {
		panic("acpi error %d\n", status);
	}

	ACPI_TABLE_HPET *Hpet = NULL;
	AcpiGetTable(ACPI_SIG_HPET, 0, (ACPI_TABLE_HEADER**)&Hpet);
	if (Hpet == NULL) {
		panic("no hpet available");
	}
	hpet_init(Hpet->Address.Address);

	ACPI_TABLE_MADT *Madt = NULL;
	AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER**)&Madt);
	if (Madt == NULL) {
		panic("no madt available");
	}
	apic_init(Madt->Address, Madt->Flags & ACPI_MADT_PCAT_COMPAT);
}
