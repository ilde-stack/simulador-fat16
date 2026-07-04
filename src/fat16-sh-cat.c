// fat16-sh-cat.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fat16.h"
#include "fat16-sh.h"

void fat16_sh_cat(const FAT16_Shell *sh, const char *filename) {
    char fat83_name[11];
    FAT16_DirEntry entry;

    if (filename == NULL) {
        fprintf(stderr, "Uso: cat archivo\n");
        return;
    }

    if (!fat16_make_fat83_name(filename, fat83_name)) {
        fprintf(stderr, "Error: use un nombre 8.3 simple, sin directorios.\n");
        return;
    }

    int find_result = fat16_find_root_entry_in_memory(&sh->fs, sh->root_dir, fat83_name, &entry);
    if (find_result == 0) {
        fprintf(stderr, "Error: archivo no encontrado en el root directory.\n");
        return;
    }

    if (entry.attributes & FAT16_ATTR_DIRECTORY) {
        fprintf(stderr, "Error: '%s' es un directorio; solo se aceptan archivos.\n", filename);
        return;
    }

    if (entry.file_size == 0) {
        return;
    }

    void *cluster_buffer = malloc(sh->fs.cluster_size);
    if (cluster_buffer == NULL) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el buffer del cluster.\n");
        return;
    }

    uint16_t cluster = entry.first_cluster_low;
    uint32_t remaining = entry.file_size;

    while (remaining > 0) {
        if (cluster < FAT16_FIRST_DATA_CLUSTER ||
            cluster >= sh->fs.cluster_count + FAT16_FIRST_DATA_CLUSTER ||
            cluster >= sh->fat_entries) {
            fprintf(stderr, "Error: cluster fuera de rango: %u\n", cluster);
            break;
        }

        int res = fat16_read_cluster(sh->fd, &sh->fs, cluster, cluster_buffer);
        if (res <= 0) {
            break;
        }

        uint32_t bytes_to_write = remaining;
        if (bytes_to_write > sh->fs.cluster_size) {
            bytes_to_write = sh->fs.cluster_size;
        }

        size_t written = fwrite(cluster_buffer, 1, bytes_to_write, stdout);
        if (written != bytes_to_write) {
            break;
        }

        remaining -= bytes_to_write;

        uint16_t next = sh->fat[cluster];
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

        cluster = next;
    }

    free(cluster_buffer);
}