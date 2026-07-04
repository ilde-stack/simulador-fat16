// fat16-ls.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "fat16.h"

/*
 * Program flow:
 *
 * 1. Open the FAT16 image and read the boot sector.
 * 2. Calculate the filesystem layout.
 * 3. Load the fixed-size FAT16 root directory into memory.
 * 4. Loop over the root directory entries.
 * 5. Skip unused, deleted, and long-name entries.
 * 6. Print each remaining entry with its name, attributes, first cluster,
 *    and file size.
 *
 * This program only lists the root directory. It does not follow subdirectories.
 */
int main(int argc, char *argv[]) {
    int fd;
    FAT16_BootSector bpb;
    FAT16_FS fs;
    FAT16_DirEntry *root_dir;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s imagen-fat16.img\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = fat16_open_image(argv[1], &bpb, &fs);
    if (fd == -1) {
        return EXIT_FAILURE;
    }

    root_dir = fat16_load_root_dir(fd, &fs);
    if (root_dir == NULL) {
        fprintf(stderr, "Error: no se pudo cargar el root directory en memoria.\n");
        close(fd);
        return EXIT_FAILURE;
    }

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

    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        FAT16_DirEntry entry = root_dir[i];
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

    free(root_dir);
    close(fd);
    return EXIT_SUCCESS;
}
