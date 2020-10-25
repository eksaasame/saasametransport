#ifndef _CONFIG_
#define _CONFIG_

#include "type.h"
#include <ntddk.h>
#include "stdio.h"

void update_driver_version_info(IN PUNICODE_STRING RegistryPath);
bool is_disable_vcbt();
bool is_only_enable_umap(IN PUNICODE_STRING RegistryPath);
bool is_batch_update_journal_meta_data(IN PUNICODE_STRING RegistryPath);
bool is_umap_flush_disabled(IN PUNICODE_STRING RegistryPath);
bool is_verbose_debug(IN PUNICODE_STRING RegistryPath);
bool is_disk_copy_data_disabled(IN PUNICODE_STRING RegistryPath);

#endif