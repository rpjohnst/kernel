#include "parse.h"
#include "../apic.h"
#include "../hpet.h"
#include "../smp.h"
#include "../pci.h"
#include <kprintf.h>
#include <assert.h>
#include <stdbool.h>

static void madt_parse(ACPI_TABLE_MADT *Madt) {
	apic_init(Madt->Address, Madt->Flags & ACPI_MADT_PCAT_COMPAT);

	for (
		ACPI_SUBTABLE_HEADER *Apic = (ACPI_SUBTABLE_HEADER*)(Madt + 1);
		(char*)Apic < (char*)Madt + Madt->Header.Length;
		Apic = (ACPI_SUBTABLE_HEADER*)((char*)Apic + Apic->Length)
	) {
		switch (Apic->Type) {
		case ACPI_MADT_TYPE_LOCAL_APIC: {
			ACPI_MADT_LOCAL_APIC *p = (ACPI_MADT_LOCAL_APIC*)Apic;
			if (!(p->LapicFlags & 0x1))
				break;

			lapic_by_cpu[lapic_count] = p->Id;
			lapic_count++;
			break;
		}

		case ACPI_MADT_TYPE_LOCAL_X2APIC: {
			ACPI_MADT_LOCAL_X2APIC *p = (ACPI_MADT_LOCAL_X2APIC *)Apic;
			if (!(p->LapicFlags & 0x1))
				break;

			lapic_by_cpu[lapic_count] = p->LocalApicId;
			lapic_count++;
			break;
		}

#if 0
		// TODO: initialize ioapic
		case ACPI_MADT_TYPE_IO_APIC: {
			ACPI_MADT_IO_APIC *p = (ACPI_MADT_IO_APIC*)Apic;
			kprintf("IOAPIC %d %#x %d\n", p->Id, p->Address, p->GlobalIrqBase);
			break;
		}

		case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE: {
			static const char *polarity[] = { "bus", "active-high", "", "active-low" };
			static const char *trigger[] = { "bus", "edge-triggered", "", "level-triggered" };

			ACPI_MADT_INTERRUPT_OVERRIDE *p = (ACPI_MADT_INTERRUPT_OVERRIDE*)Apic;
			kprintf(
				"INT_SRC_OVR %d -> %d %s %s\n", p->SourceIrq, p->GlobalIrq,
				polarity[p->IntiFlags & 0x03], trigger[(p->IntiFlags >> 2) & 0x3]
			);
			break;
		}

		case ACPI_MADT_TYPE_NMI_SOURCE: {
			ACPI_MADT_NMI_SOURCE *p = (ACPI_MADT_NMI_SOURCE*)Apic;
			kprintf("NMI_SRC %d %d\n", p->IntiFlags, p->GlobalIrq);
			break;
		}

		case ACPI_MADT_TYPE_LOCAL_APIC_NMI: {
			ACPI_MADT_LOCAL_APIC_NMI *p = (ACPI_MADT_LOCAL_APIC_NMI*)Apic;
			kprintf("LAPIC_NMI %d %d %d\n", p->ProcessorId, p->IntiFlags, p->Lint);
			break;
		}
#endif
		}
	}
}

static void mcfg_parse(ACPI_TABLE_MCFG *Mcfg) {
	for (
		ACPI_MCFG_ALLOCATION *Allocation = (ACPI_MCFG_ALLOCATION*)(Mcfg + 1);
		(char*)Allocation < (char*)Mcfg + Mcfg->Header.Length;
		Allocation++
	) {
		pci_add_segment(
			Allocation->PciSegment, Allocation->Address,
			Allocation->StartBusNumber, Allocation->EndBusNumber
		);
	}
}

#define ACPI_TABLE_COUNT 128
static ACPI_TABLE_DESC acpi_tables[ACPI_TABLE_COUNT];

void acpi_parse(ACPI_TABLE_RSDP *Rsdp) {
	extern ACPI_TABLE_RSDP *AcpiOsRsdp;
	AcpiOsRsdp = Rsdp;

	ACPI_STATUS status = AcpiInitializeTables(acpi_tables, ACPI_TABLE_COUNT, false);
	if (ACPI_FAILURE(status)) {
		panic("acpi error %d\n", status);
	}

	ACPI_TABLE_MADT *Madt = NULL;
	AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER**)&Madt);
	if (Madt == NULL) {
		panic("no madt available");
	}
	madt_parse(Madt);

	ACPI_TABLE_HPET *Hpet = NULL;
	AcpiGetTable(ACPI_SIG_HPET, 0, (ACPI_TABLE_HEADER**)&Hpet);
	if (Hpet == NULL) {
		panic("no hpet available");
	}
	hpet_init(Hpet->Address.Address);

	ACPI_TABLE_MCFG *Mcfg = NULL;
	AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER**)&Mcfg);
	if (Mcfg == NULL) {
		panic("no mcfg available");
	}
	mcfg_parse(Mcfg);
}
