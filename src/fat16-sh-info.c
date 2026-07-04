// fat16-sh-info.c

#include <stdio.h>

#include "fat16-sh.h"

void fat16_sh_info(const FAT16_Shell *sh) {
    const FAT16_BootSector *bpb = &sh->bpb;
    const FAT16_FS *fs = &sh->fs;

    printf("Información FAT16\n");
    printf("=================\n\n");

    fat16_print_fixed_string("OEM name", bpb->oem_name, sizeof(bpb->oem_name));
    fat16_print_fixed_string("Volume label", bpb->volume_label, sizeof(bpb->volume_label));
    fat16_print_fixed_string("Filesystem type", bpb->filesystem_type, sizeof(bpb->filesystem_type));

    printf("\nBoot Sector / BPB\n");
    printf("-----------------\n");
    printf("%-30s: %u\n", "Bytes por sector", bpb->bytes_per_sector);
    printf("%-30s: %u\n", "Sectores por cluster", bpb->sectors_per_cluster);
    printf("%-30s: %u bytes\n", "Tamano de cluster", fs->cluster_size);
    printf("%-30s: %u\n", "Sectores reservados", bpb->reserved_sectors);
    printf("%-30s: %u\n", "Cantidad de FATs", bpb->fat_count);
    printf("%-30s: %u\n", "Entradas root directory", bpb->root_entry_count);
    printf("%-30s: %u\n", "Sectores totales", fs->total_sectors);
    printf("%-30s: 0x%02X\n", "Media descriptor", bpb->media_descriptor);
    printf("%-30s: %u\n", "Sectores por FAT", bpb->sectors_per_fat);
    printf("%-30s: %u\n", "Sectores por pista", bpb->sectors_per_track);
    printf("%-30s: %u\n", "Cabezas", bpb->head_count);
    printf("%-30s: %u\n", "Sectores ocultos", bpb->hidden_sectors);
    printf("%-30s: 0x%08X\n", "Volume ID", bpb->volume_id);
    printf("%-30s: 0x%04X\n", "Boot sector signature", bpb->boot_sector_signature);

    printf("\nLayout calculado\n");
    printf("----------------\n");
    printf("%-30s: sector %u, offset 0x%X\n",
           "FAT #1",
           fs->first_fat_sector,
           fs->fat_offset);

    printf("%-30s: sector %u, offset 0x%X\n",
           "Root directory",
           fs->first_root_dir_sector,
           fs->root_dir_offset);

    printf("%-30s: sector %u, offset 0x%X\n",
           "Data area",
           fs->first_data_sector,
           fs->data_area_offset);

    printf("%-30s: %u\n", "Sectores root directory", fs->root_dir_sectors);
    printf("%-30s: %u\n", "Sectores de datos", fs->total_sectors - fs->first_data_sector);
    printf("%-30s: %u\n", "Clusters de datos", fs->cluster_count);
    printf("%-30s: %u\n", "Primer cluster de datos", FAT16_FIRST_DATA_CLUSTER);
}
