// fat16-sh-ls.c

#include <stdio.h>
#include <stdint.h>

#include "fat16-sh.h"

void fat16_sh_ls(const FAT16_Shell *sh) {
    printf("%-14s %-6s %-6s %-10s %s\n",
           "Nombre",
           "Attr",
           "Hex",
           "Cluster",
           "Tamano");
    printf("%-14s %-6s %-6s %-10s %s\n",
           "------",
           "------",
           "------",
           "----------",
           "------");

    for (uint32_t i = 0; i < sh->bpb.root_entry_count; i++) {
        FAT16_DirEntry entry = sh->root_dir[i];
        char name[14];
        char attrs[7];

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_FREE) {
            break;
        }

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_DELETED) {
            continue;
        }

        if ((entry.attributes & FAT16_ATTR_LONG_NAME) == FAT16_ATTR_LONG_NAME) {
            continue;
        }

        fat16_format_dir_name(&entry, name, sizeof(name));
        fat16_format_attributes(entry.attributes, attrs, sizeof(attrs));

        printf("%-14s %-6s 0x%02X   %-10u %u\n",
               name,
               attrs,
               entry.attributes,
               entry.first_cluster_low,
               entry.file_size);
    }
}
