// fat16-sh-fileclusters.c

#include <stdio.h>
#include <stdint.h>

#include "fat16-sh.h"

void fat16_sh_fileclusters(const FAT16_Shell *sh, const char *filename) {
    char fat_name[11];
    FAT16_DirEntry entry;
    int find_result;
    char name[14];
    char attrs[7];
    uint16_t cluster;

    if (filename == NULL) {
        fprintf(stderr, "Uso: fileclusters archivo\n");
        return;
    }

    if (!fat16_make_fat83_name(filename, fat_name)) {
        fprintf(stderr, "Error: use un nombre 8.3 simple, sin directorios.\n");
        return;
    }

    find_result = fat16_find_root_entry_in_memory(&sh->fs,
                                                  sh->root_dir,
                                                  fat_name,
                                                  &entry);
    if (find_result == 0) {
        fprintf(stderr, "Error: archivo no encontrado en el root directory.\n");
        return;
    }

    if (entry.attributes & FAT16_ATTR_DIRECTORY) {
        fprintf(stderr, "Error: '%s' es un directorio; solo se aceptan archivos.\n", filename);
        return;
    }

    fat16_format_dir_name(&entry, name, sizeof(name));
    fat16_format_attributes(entry.attributes, attrs, sizeof(attrs));

    printf("%-30s: %s\n", "Nombre", name);
    printf("%-30s: %s\n", "Atributos", attrs);
    printf("%-30s: 0x%02X\n", "Atributos hex", entry.attributes);
    printf("%-30s: %u\n", "Primer cluster", entry.first_cluster_low);
    printf("%-30s: %u\n", "Tamano", entry.file_size);
    printf("%-30s:", "Clusters");

    if (entry.file_size == 0 || entry.first_cluster_low == FAT16_FREE_CLUSTER) {
        printf(" (archivo vacio)\n");
        return;
    }

    cluster = entry.first_cluster_low;
    for (uint32_t count = 0; count < sh->fs.cluster_count; count++) {
        uint16_t next;

        if (cluster < FAT16_FIRST_DATA_CLUSTER ||
            cluster >= sh->fs.cluster_count + FAT16_FIRST_DATA_CLUSTER ||
            cluster >= sh->fat_entries) {
            printf("\nError: cluster fuera de rango: %u\n", cluster);
            return;
        }

        printf(" %u", cluster);
        next = sh->fat[cluster];

        if (next >= FAT16_EOC_MIN) {
            printf("\n");
            return;
        }

        if (next == FAT16_FREE_CLUSTER) {
            printf("\nError: cadena de clusters apunta a un cluster libre.\n");
            return;
        }

        if (next == FAT16_BAD_CLUSTER) {
            printf("\nError: cadena de clusters apunta a un cluster defectuoso.\n");
            return;
        }

        if (next >= sh->fat_entries) {
            printf("\nError: la FAT apunta fuera de la tabla: %u\n", next);
            return;
        }

        cluster = next;
    }

    printf("\nError: cadena de clusters demasiado larga o circular.\n");
}
