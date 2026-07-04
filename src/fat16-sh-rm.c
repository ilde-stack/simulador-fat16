// fat16-sh-rm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fat16.h"
#include "fat16-sh.h"

void fat16_sh_rm(FAT16_Shell *sh, const char *filename) {
    char fat_name[11];
    uint32_t index;

    if (filename == NULL) {
        fprintf(stderr, "Uso: rm archivo\n");
        return;
    }

    if (!fat16_make_fat83_name(filename, fat_name)) {
        fprintf(stderr, "Error: use un nombre 8.3 simple, sin directorios.\n");
        return;
    }

    int found = fat16_find_root_entry_index(&sh->fs, sh->root_dir, fat_name, &index);
    if (!found) {
        fprintf(stderr, "Error: archivo no encontrado en el root directory.\n");
        return;
    }

    if (sh->root_dir[index].attributes & FAT16_ATTR_DIRECTORY) {
        fprintf(stderr, "Error: '%s' es un directorio; solo se aceptan archivos.\n", filename);
        return;
    }

    uint16_t cluster = sh->root_dir[index].first_cluster_low;

    sh->root_dir[index].name[0] = FAT16_DIR_ENTRY_DELETED;

    if (cluster != FAT16_FREE_CLUSTER && sh->root_dir[index].file_size > 0) {
        uint32_t safety_counter = 0;

        while (cluster != FAT16_FREE_CLUSTER) {
            if (cluster < FAT16_FIRST_DATA_CLUSTER ||
                cluster >= sh->fs.cluster_count + FAT16_FIRST_DATA_CLUSTER ||
                cluster >= sh->fat_entries) {
                fprintf(stderr, "Error: cluster fuera de rango: %u\n", cluster);
                break;
            }

            uint16_t next = sh->fat[cluster];
            sh->fat[cluster] = FAT16_FREE_CLUSTER;

            if (next >= FAT16_EOC_MIN) {
                break;
            }

            if (next == FAT16_FREE_CLUSTER) {
                fprintf(stderr, "Error: cadena de clusters apunta a un cluster libre.\n");
                break;
            }
            if (next == FAT16_BAD_CLUSTER) {
                fprintf(stderr, "Error: cadena de clusters apunta a un cluster defectuoso.\n");
                break;
            }

            safety_counter++;
            if (safety_counter > sh->fs.cluster_count) {
                fprintf(stderr, "Error: cadena de clusters demasiado larga o circular.\n");
                break;
            }

            cluster = next;
        }
    }

    if (fat16_write_root_dir(sh->fd, &sh->fs, sh->root_dir) != 1) {
        return;
    }

    if (fat16_write_fat(sh->fd, &sh->fs, sh->fat) != 1) {
        return;
    }

   
}