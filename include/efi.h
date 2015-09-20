#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef size_t efi_status;

struct efi_guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
};

enum efi_memory_type {
	efi_reserved,
	efi_loader_code,
	efi_loader_data,
	efi_boot_code,
	efi_boot_data,
	efi_runtime_code,
	efi_runtime_data,
	efi_conventional,
	efi_unusable,
	efi_acpi_reclaim,
	efi_acpi_nvs,
	efi_memory_mapped_io,
	efi_memory_mapped_port,
	efi_pal,
};

// unfortunately this can't be an enum because enum literals are 'int' and this is 64-bit
static const uint64_t efi_memory_uc = 1 << 0;
static const uint64_t efi_memory_wc = 1 << 1;
static const uint64_t efi_memory_wt = 1 << 2;
static const uint64_t efi_memory_wb = 1 << 3;
static const uint64_t efi_memory_uce = 1 << 4;
static const uint64_t efi_memory_wp = 1 << 12;
static const uint64_t efi_memory_rp = 1 << 13;
static const uint64_t efi_memory_xp = 1 << 14;
static const uint64_t efi_memory_runtime = 1UL << 63;

struct efi_memory_descriptor {
	uint32_t type;
	uint64_t physical;
	uint64_t virtual;
	uint64_t pages;
	uint64_t flags;
};

struct efi_time {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t pad1;
	uint32_t nanosecond;
	uint16_t timezone;
	uint8_t daylight;
	uint8_t pad2;
};

struct efi_time_capabilities {
	uint32_t resolution;
	uint32_t accuracy;
	bool sets_to_zero;
};

enum efi_reset_type {
	efi_reset_cold,
	efi_reset_warm,
	efi_reset_shutdown,
};

struct efi_capsule_header {
	struct efi_guid capsule_guid;
	uint32_t header_size;
	uint32_t flags;
	uint32_t capsule_image_size;
};

struct efi_table_header {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
};

typedef efi_status efi_get_time(
	struct efi_time*, struct efi_time_capabilities *capabilities
) __attribute__((ms_abi));
typedef efi_status efi_set_time(struct efi_time*) __attribute__((ms_abi));
typedef efi_status efi_get_wakeup_time(bool*, bool*, struct efi_time*) __attribute__((ms_abi));
typedef efi_status efi_set_wakeup_time(bool*, struct efi_time*) __attribute__((ms_abi));
typedef efi_status efi_set_wakeup_time(bool*, struct efi_time*) __attribute__((ms_abi));
typedef efi_status efi_set_virtual_address_map(
	size_t, size_t, uint32_t, struct efi_memory_descriptor*
) __attribute__((ms_abi));
typedef efi_status efi_convert_pointer(size_t, void**) __attribute__((ms_abi));
typedef efi_status efi_get_variable(
	uint16_t*, struct efi_guid*, uint32_t*, size_t*, void*
) __attribute__((ms_abi));
typedef efi_status efi_get_next_variable_name(
	size_t*, uint16_t*, struct efi_guid*
) __attribute__((ms_abi));
typedef efi_status efi_set_variable(
	uint16_t*, struct efi_guid*, uint32_t*, size_t*, void*
) __attribute__((ms_abi));
typedef efi_status efi_get_next_high_monotonic_count(uint32_t*) __attribute__((ms_abi));
typedef efi_status efi_reset_system(
	enum efi_reset_type, efi_status, size_t, void*
) __attribute__((ms_abi));
typedef efi_status efi_update_capsule(
	struct efi_capsule_header**, size_t, uint64_t
) __attribute__((ms_abi));
typedef efi_status efi_query_capsule_capabilities(
	struct efi_capsule_header**, size_t, uint64_t*, enum efi_reset_type*
) __attribute__((ms_abi));
typedef efi_status efi_query_variable_info(
	uint32_t attributes, uint64_t*, uint64_t*, uint64_t*
) __attribute__((ms_abi));

struct efi_runtime_services {
	struct efi_table_header header;
	efi_get_time *get_time;
	efi_set_time *set_time;
	efi_get_wakeup_time *get_wakeup_time;
	efi_set_wakeup_time *set_wakeup_time;
	efi_set_virtual_address_map *set_virtual_address_map;
	efi_convert_pointer *convert_pointer;
	efi_get_variable *get_variable;
	efi_get_next_variable_name *get_next_variable_name;
	efi_set_variable *set_variable;
	efi_get_next_high_monotonic_count *get_next_high_monotonic_count;
	efi_reset_system *reset_system;
	efi_update_capsule *update_capsule;
	efi_query_capsule_capabilities *query_capsule_capabilities;
	efi_query_variable_info *query_variable_info;
};
