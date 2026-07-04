#ifndef FAT16_SH_H
#define FAT16_SH_H

#include <stdint.h>

#include "fat16.h"

typedef struct FAT16_Shell {
    int fd;
    FAT16_BootSector bpb;
    FAT16_FS fs;
    FAT16_DirEntry *root_dir;
    uint16_t *fat;
    uint32_t fat_entries;
} FAT16_Shell;

void fat16_sh_info(const FAT16_Shell *sh);
void fat16_sh_ls(const FAT16_Shell *sh);
void fat16_sh_fileclusters(const FAT16_Shell *sh, const char *filename);
void fat16_sh_cat(const FAT16_Shell *sh, const char *filename);
void fat16_sh_rm(FAT16_Shell *sh, const char *filename);

#endif
