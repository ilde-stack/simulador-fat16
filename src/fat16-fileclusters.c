// fat16-fileclusters.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "fat16.h"

/*
 * Program flow:
 *
 * 1. Convert the user filename into the FAT 8.3 directory-entry format.
 * 2. Open the FAT16 image and read the boot sector.
 * 3. Search the root directory for that filename.
 * 4. Reject the entry if it is a directory.
 * 5. Load FAT #1 into memory.
 * 6. Start from the file's first cluster.
 * 7. Follow the FAT entries in order:
 *
 *        current cluster -> FAT[current cluster] -> next cluster
 *
 * 8. Print each cluster until an end-of-chain marker is found.
 *
 * This program shows how FAT represents a file as a chain of clusters.
 */
int main(int argc, char *argv[]) {
    int fd;
    char fat_name[11];
    uint16_t *fat;
    uint32_t fat_entries;
    FAT16_BootSector bpb;
    FAT16_FS fs;
    FAT16_DirEntry entry;
    int find_result;
    char name[14];
    char attrs[7];
    uint16_t cluster;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s imagen-fat16.img archivo\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!fat16_make_fat83_name(argv[2], fat_name)) {
        fprintf(stderr, "Error: use un nombre 8.3 simple, sin directorios.\n");
        return EXIT_FAILURE;
    }

    fd = fat16_open_image(argv[1], &bpb, &fs);
    if (fd == -1) {
        return EXIT_FAILURE;
    }

    find_result = fat16_find_root_entry(fd, &fs, fat_name, &entry);
    if (find_result == -1) {
        perror("No se pudo leer el root directory");
        close(fd);
        return EXIT_FAILURE;
    }

    if (find_result == 0) {
        fprintf(stderr, "Error: archivo no encontrado en el root directory.\n");
        close(fd);
        return EXIT_FAILURE;
    }

    if (entry.attributes & FAT16_ATTR_DIRECTORY) {
        fprintf(stderr, "Error: '%s' es un directorio; solo se aceptan archivos.\n", argv[2]);
        close(fd);
        return EXIT_FAILURE;
    }

    fat = fat16_load_fat(fd, &fs);
    if (fat == NULL) {
        fprintf(stderr, "Error: no se pudo cargar la FAT en memoria.\n");
        close(fd);
        return EXIT_FAILURE;
    }

    fat_entries = fat16_fat_entry_count(&fs);

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
        free(fat);
        close(fd);
        return EXIT_SUCCESS;
    }

    cluster = entry.first_cluster_low;
    for (uint32_t count = 0; count < fs.cluster_count; count++) {
        uint16_t next;

        if (cluster < FAT16_FIRST_DATA_CLUSTER ||
            cluster >= fs.cluster_count + FAT16_FIRST_DATA_CLUSTER ||
            cluster >= fat_entries) {
            printf("\nError: cluster fuera de rango: %u\n", cluster);
            free(fat);
            close(fd);
            return EXIT_FAILURE;
        }

        printf(" %u", cluster);
        next = fat[cluster];

        if (next >= FAT16_EOC_MIN) {
            printf("\n");
            free(fat);
            close(fd);
            return EXIT_SUCCESS;
        }

        if (next == FAT16_FREE_CLUSTER) {
            printf("\nError: cadena de clusters apunta a un cluster libre.\n");
            free(fat);
            close(fd);
            return EXIT_FAILURE;
        }

        if (next == FAT16_BAD_CLUSTER) {
            printf("\nError: cadena de clusters apunta a un cluster defectuoso.\n");
            free(fat);
            close(fd);
            return EXIT_FAILURE;
        }

        if (next >= fat_entries) {
            printf("\nError: la FAT apunta fuera de la tabla: %u\n", next);
            free(fat);
            close(fd);
            return EXIT_FAILURE;
        }

        cluster = next;
    }

    printf("\nError: cadena de clusters demasiado larga o circular.\n");
    free(fat);
    close(fd);
    return EXIT_FAILURE;
}
