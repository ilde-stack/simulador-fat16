// fat16.c

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#include "fat16.h"

/*
 * Removes spaces from the end of a null-terminated buffer.
 *
 * Arguments:
 * - buffer: string buffer to modify in place.
 * - len: number of useful characters originally copied into buffer.
 *
 * Return value:
 * - none. The buffer is modified directly.
 */
static void trim_trailing_spaces(char *buffer, size_t len) {
    for (int i = (int)len - 1; i >= 0; i--) {
        if (buffer[i] == ' ') {
            buffer[i] = '\0';
        } else {
            break;
        }
    }
}

/*
 * Copies a fixed-width FAT text field into a normal C string.
 *
 * Arguments:
 * - dst: destination buffer.
 * - dst_size: size of the destination buffer.
 * - src: source bytes from the FAT structure.
 * - src_size: number of source bytes to copy at most.
 *
 * Return value:
 * - none. The destination buffer is filled, null-terminated, and trimmed.
 */
static void copy_trimmed(char *dst, size_t dst_size, const char *src, size_t src_size) {
    if (src_size >= dst_size) {
        src_size = dst_size - 1;
    }

    memcpy(dst, src, src_size);
    dst[src_size] = '\0';

    trim_trailing_spaces(dst, src_size);
}

/*
 * Prints a debug message to stderr.
 *
 * If stderr is connected to a terminal, the message is printed in yellow so
 * it is easier to distinguish from the program's normal stdout output. If
 * stderr is redirected to a file, no color codes are printed.
 *
 * Arguments:
 * - format: printf-style format string.
 * - ...: values used by the format string.
 *
 * Return value:
 * - none.
 */
static void debug_print(const char *format, ...) {
    va_list args;
    int use_color;

    use_color = isatty(STDERR_FILENO);

    if (use_color) {
        fprintf(stderr, "\033[33m");
    }

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    if (use_color) {
        fprintf(stderr, "\033[0m");
    }
}

/*
 * fat16_print_fixed_string prints a fixed-width text field from the FAT boot
 * sector.
 *
 * FAT text fields are not necessarily null-terminated and are padded
 * with trailing spaces, so this copies them into a local buffer,
 * adds '\0', trims the padding, and prints the cleaned value.
 *
 * Arguments:
 * - label: text printed before the value.
 * - s: fixed-width FAT text field.
 * - len: number of bytes in the FAT text field.
 *
 * Return value:
 * - none.
 */
void fat16_print_fixed_string(const char *label, const char *s, size_t len) {

    /*
     * 32 bytes is enough for the fixed FAT text fields printed here
     * (OEM name: 8, volume label: 11, filesystem type: 8), plus '\0'.
     */
    char buffer[32];

    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }

    memcpy(buffer, s, len);
    buffer[len] = '\0';

    trim_trailing_spaces(buffer, len);

    printf("%-30s: %s\n", label, buffer);
}

/*
 * Reads one logical block from the image.
 *
 * The stderr message is intentional: in this exercise, "block" means one
 * fixed-size unit transferred from the block-storage image. Counting these
 * messages helps visualize how much disk I/O a filesystem operation causes.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - block: block number to read, starting at block 0.
 * - block_size: size of one block in bytes.
 * - buffer: destination buffer. It must have room for block_size bytes.
 *
 * Return value:
 * - 1 if the complete block was read.
 * - 0 if the image ended before a complete block was read.
 * - -1 on seek/read error.
 */
int fat16_read_block(int fd, uint32_t block, uint32_t block_size, void *buffer) {
    ssize_t bytes_read;

    debug_print("I/O: READ  block %u (%u bytes)\n", block, block_size);

    if (lseek(fd, (off_t)block * block_size, SEEK_SET) == -1) {
        return -1;
    }

    bytes_read = read(fd, buffer, block_size);
    if (bytes_read == -1) {
        return -1;
    }

    if (bytes_read != (ssize_t)block_size) {
        return 0;
    }

    return 1;
}

/*
 * Writes one logical block to the image.
 *
 * The stderr message mirrors fat16_read_block(): each line represents one
 * block-storage write caused by the filesystem code.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - block: block number to write, starting at block 0.
 * - block_size: size of one block in bytes.
 * - buffer: source buffer containing block_size bytes.
 *
 * Return value:
 * - 1 if the complete block was written.
 * - 0 if write() wrote fewer bytes than requested.
 * - -1 on seek/write error.
 */
int fat16_write_block(int fd, uint32_t block, uint32_t block_size, const void *buffer) {
    ssize_t bytes_written;

    debug_print("I/O: WRITE block %u (%u bytes)\n", block, block_size);

    if (lseek(fd, (off_t)block * block_size, SEEK_SET) == -1) {
        return -1;
    }

    bytes_written = write(fd, buffer, block_size);
    if (bytes_written == -1) {
        return -1;
    }

    if (bytes_written != (ssize_t)block_size) {
        return 0;
    }

    return 1;
}

/*
 * Converts a FAT directory entry name into a printable C string.
 *
 * Arguments:
 * - entry: directory entry to format.
 * - name: destination buffer for the printable name.
 * - size: size of the destination buffer.
 *
 * Return value:
 * - none. The formatted name is written to name.
 */
void fat16_format_dir_name(const FAT16_DirEntry *entry, char *name, size_t size) {
    char base[9];
    char ext[4];

    copy_trimmed(base, sizeof(base), entry->name, sizeof(entry->name));
    copy_trimmed(ext, sizeof(ext), entry->ext, sizeof(entry->ext));

    if ((entry->attributes & FAT16_ATTR_VOLUME_ID) || ext[0] == '\0') {
        snprintf(name, size, "%s", base);
    } else {
        snprintf(name, size, "%s.%s", base, ext);
    }
}

/*
 * Converts the FAT attributes byte into a six-character string.
 *
 * The output positions mean:
 * R = read-only, H = hidden, S = system, V = volume label,
 * D = directory, A = archive. Missing attributes are printed as '-'.
 *
 * Arguments:
 * - attributes: raw FAT attributes byte from a directory entry.
 * - text: destination buffer. It should have room for 7 bytes, including '\0'.
 * - size: size of the destination buffer.
 *
 * Return value:
 * - none. The formatted attributes are written to text.
 */
void fat16_format_attributes(uint8_t attributes, char *text, size_t size) {
    snprintf(text,
             size,
             "%c%c%c%c%c%c",
             (attributes & FAT16_ATTR_READ_ONLY) ? 'R' : '-',
             (attributes & FAT16_ATTR_HIDDEN) ? 'H' : '-',
             (attributes & FAT16_ATTR_SYSTEM) ? 'S' : '-',
             (attributes & FAT16_ATTR_VOLUME_ID) ? 'V' : '-',
             (attributes & FAT16_ATTR_DIRECTORY) ? 'D' : '-',
             (attributes & FAT16_ATTR_ARCHIVE) ? 'A' : '-');
}

/*
 * Converts a user filename such as "hello.txt" into the 11-byte FAT 8.3 name
 * used inside directory entries.
 *
 * Arguments:
 * - name: filename typed by the user. It must be a single 8.3 name, with no
 * directories.
 * - fat_name: output buffer. It must have room for exactly 11 bytes.
 *
 * Return value:
 * - 1 if the name was accepted and fat_name was filled.
 * - 0 if the name is not a simple 8.3 filename.
 */
int fat16_make_fat83_name(const char *name, char fat_name[11]) {
    const char *dot;
    size_t base_len;
    size_t ext_len;

    if (name[0] == '\0' || strchr(name, '/') != NULL || strchr(name, '\\') != NULL) {
        return 0;
    }

    dot = strchr(name, '.');
    if (dot == NULL) {
        base_len = strlen(name);
        ext_len = 0;
    } else {
        if (strchr(dot + 1, '.') != NULL) {
            return 0;
        }

        base_len = (size_t)(dot - name);
        ext_len = strlen(dot + 1);
    }

    if (base_len == 0 || base_len > 8 || ext_len > 3) {
        return 0;
    }

    memset(fat_name, ' ', 11);

    for (size_t i = 0; i < base_len; i++) {
        fat_name[i] = (char)toupper((unsigned char)name[i]);
    }

    for (size_t i = 0; i < ext_len; i++) {
        fat_name[8 + i] = (char)toupper((unsigned char)dot[1 + i]);
    }

    return 1;
}

/*
 * Returns the total number of blocks in the FAT16 image.
 *
 * Arguments:
 * - bpb: boot sector / BIOS Parameter Block.
 *
 * Return value:
 * - total blocks from total_sectors_16, or total_sectors_32 when the 16-bit
 * field is zero.
 */
uint32_t fat16_get_total_sectors(const FAT16_BootSector *bpb) {
    if (bpb->total_sectors_16 != 0) {
        return bpb->total_sectors_16;
    }

    return bpb->total_sectors_32;
}

/*
 * Calculates how many blocks the fixed-size FAT16 root directory uses.
 *
 * Arguments:
 * - bpb: boot sector / BIOS Parameter Block.
 *
 * Return value:
 * - number of blocks occupied by the root directory.
 */
uint32_t fat16_get_root_dir_sectors(const FAT16_BootSector *bpb) {
    uint32_t root_dir_bytes;

    root_dir_bytes = bpb->root_entry_count * 32;

    return (root_dir_bytes + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;
}

/*
 * Performs basic checks to decide whether the boot sector looks like FAT16.
 *
 * Arguments:
 * - bpb: boot sector / BIOS Parameter Block already read from the image.
 *
 * Return value:
 * - 1 if the fields look reasonable for this exercise.
 * - 0 otherwise.
 */
int fat16_is_probably_fat16(const FAT16_BootSector *bpb) {
    if (bpb->boot_sector_signature != 0xAA55) {
        return 0;
    }

    if (bpb->bytes_per_sector == 0) {
        return 0;
    }

    if (bpb->sectors_per_cluster == 0) {
        return 0;
    }

    if (bpb->fat_count == 0) {
        return 0;
    }

    if (bpb->sectors_per_fat == 0) {
        return 0;
    }

    if (bpb->root_entry_count == 0) {
        return 0;
    }

    /*
     * This text field is informational, but it is useful as one more
     * simple check for this exercise.
     */
    if (memcmp(bpb->filesystem_type, "FAT16", 5) != 0) {
        return 0;
    }

    return 1;
}

/*
 * Reads block 0 as a FAT16 boot sector.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - bpb: destination boot-sector structure.
 *
 * Return value:
 * - 1 if the boot sector was read completely.
 * - 0 if the image ended before a complete boot sector was read.
 * - -1 on seek/read error.
 */
int fat16_read_boot_sector(int fd, FAT16_BootSector *bpb) {
    int read_result;

    /*
     * FAT stores multi-byte numbers in little-endian order. This educational
     * code reads directly into a C struct, so it assumes the host machine is
     * also little-endian. On big-endian hardware, numeric fields would need
     * explicit conversion after reading.
     */

    read_result = fat16_read_block(fd, 0, sizeof(FAT16_BootSector), bpb);
    if (read_result == -1) {
        return -1;
    }

    if (read_result == 0) {
        return 0;
    }

    return 1;
}

/*
 * Calculates the important FAT16 layout positions from the boot sector.
 *
 * Arguments:
 * - fs: output structure that receives calculated offsets and sizes.
 * - bpb: boot sector / BIOS Parameter Block.
 *
 * Return value:
 * - 1 if the layout is valid enough for the exercise.
 * - 0 if the calculated layout is impossible.
 */
int fat16_calculate_layout(FAT16_FS *fs, const FAT16_BootSector *bpb) {
    fs->bpb = *bpb;

    fs->total_sectors = fat16_get_total_sectors(bpb);
    fs->root_dir_sectors = fat16_get_root_dir_sectors(bpb);

    fs->first_fat_sector = bpb->reserved_sectors;
    fs->first_root_dir_sector =
        bpb->reserved_sectors +
        bpb->fat_count * bpb->sectors_per_fat;

    fs->first_data_sector =
        fs->first_root_dir_sector +
        fs->root_dir_sectors;

    if (fs->total_sectors <= fs->first_data_sector) {
        return 0;
    }

    fs->fat_offset = fs->first_fat_sector * bpb->bytes_per_sector;
    fs->root_dir_offset = fs->first_root_dir_sector * bpb->bytes_per_sector;
    fs->data_area_offset = fs->first_data_sector * bpb->bytes_per_sector;

    fs->cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    fs->cluster_count =
        (fs->total_sectors - fs->first_data_sector) / bpb->sectors_per_cluster;

    return 1;
}

/*
 * Opens a FAT16 image and fills the boot-sector and layout structures.
 *
 * Arguments:
 * - path: image file path.
 * - bpb: output boot-sector structure.
 * - fs: output structure with useful calculated offsets and sizes.
 *
 * Return value:
 * - a file descriptor opened read-only on success.
 * - -1 on error. This function prints a short error message before returning.
 */
int fat16_open_image(const char *path, FAT16_BootSector *bpb, FAT16_FS *fs) {
    int fd;
    int read_result;

    fd = open(path, O_RDWR);
    if (fd == -1) {
        perror("No se pudo abrir la imagen");
        return -1;
    }

    read_result = fat16_read_boot_sector(fd, bpb);
    if (read_result == -1) {
        perror("No se pudo leer el boot sector");
        close(fd);
        return -1;
    }

    if (read_result == 0) {
        fprintf(stderr, "Error: boot sector incompleto.\n");
        close(fd);
        return -1;
    }

    if (!fat16_is_probably_fat16(bpb)) {
        fprintf(stderr, "Error: la imagen no parece ser FAT16 valido.\n");
        close(fd);
        return -1;
    }

    if (!fat16_calculate_layout(fs, bpb)) {
        fprintf(stderr, "Error: layout FAT16 invalido.\n");
        close(fd);
        return -1;
    }

    return fd;
}

/*
 * Searches the root directory for one FAT 8.3 filename.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - fs: calculated FAT16 layout.
 * - fat_name: 11-byte FAT name, usually created with fat16_make_fat83_name().
 * - found: output directory entry, filled only when the file is found.
 *
 * Return value:
 * - 1 if the entry was found.
 * - 0 if the entry was not found.
 * - -1 on read/seek error.
 */
int fat16_find_root_entry(int fd,
                          const FAT16_FS *fs,
                          const char fat_name[11],
                          FAT16_DirEntry *found) {
    FAT16_DirEntry *root_dir;
    int result;

    root_dir = fat16_load_root_dir(fd, fs);
    if (root_dir == NULL) {
        return -1;
    }

    result = fat16_find_root_entry_in_memory(fs, root_dir, fat_name, found);
    free(root_dir);
    return result;
}

/*
 * Searches an already-loaded root directory for one FAT 8.3 filename.
 *
 * Arguments:
 * - fs: calculated FAT16 layout.
 * - root_dir: in-memory root directory array.
 * - fat_name: 11-byte FAT name, usually created with fat16_make_fat83_name().
 * - found: output directory entry, filled only when the file is found.
 *
 * Return value:
 * - 1 if the entry was found.
 * - 0 if the entry was not found.
 */
int fat16_find_root_entry_in_memory(const FAT16_FS *fs,
                                    const FAT16_DirEntry *root_dir,
                                    const char fat_name[11],
                                    FAT16_DirEntry *found) {
    for (uint32_t i = 0; i < fs->bpb.root_entry_count; i++) {
        FAT16_DirEntry entry = root_dir[i];

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_FREE) {
            break;
        }

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_DELETED) {
            continue;
        }

        if ((entry.attributes & FAT16_ATTR_LONG_NAME) == FAT16_ATTR_LONG_NAME) {
            continue;
        }

        if (entry.attributes & FAT16_ATTR_VOLUME_ID) {
            continue;
        }

        if (memcmp(entry.name, fat_name, 8) == 0 &&
            memcmp(entry.ext, fat_name + 8, 3) == 0) {
            *found = entry;
            return 1;
        }
    }

    return 0;
}

/*
 * Loads the fixed-size FAT16 root directory into memory.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - fs: calculated FAT16 layout.
 *
 * Return value:
 * - pointer to a malloc-allocated array of FAT16_DirEntry values. The caller
 * must free it.
 * - NULL on allocation or block-read error.
 */
FAT16_DirEntry *fat16_load_root_dir(int fd, const FAT16_FS *fs) {
    uint32_t block_size = fs->bpb.bytes_per_sector;
    uint32_t first_block = fs->first_root_dir_sector;
    size_t bytes = fs->root_dir_sectors * block_size;
    uint8_t *root_dir;

    root_dir = malloc(bytes);
    if (root_dir == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < fs->root_dir_sectors; i++) {
        int read_result;

        read_result = fat16_read_block(fd,
                                       first_block + i,
                                       block_size,
                                       root_dir + i * block_size);
        if (read_result != 1) {
            free(root_dir);
            return NULL;
        }
    }

    return (FAT16_DirEntry *)root_dir;
}

/*
 * Returns how many 16-bit entries fit in one FAT copy.
 *
 * Arguments:
 * - fs: calculated FAT16 layout.
 *
 * Return value:
 * - number of uint16_t entries in FAT #1.
 */
uint32_t fat16_fat_entry_count(const FAT16_FS *fs) {
    return fs->bpb.sectors_per_fat * fs->bpb.bytes_per_sector / sizeof(uint16_t);
}

/*
 * Loads FAT #1 into memory as an array of uint16_t entries.
 *
 * Arguments:
 * - fd: open image file descriptor.
 * - fs: calculated FAT16 layout.
 *
 * Return value:
 * - pointer to a malloc-allocated FAT array on success. The caller must free it.
 * - NULL on seek, allocation, or read error.
 */
uint16_t *fat16_load_fat(int fd, const FAT16_FS *fs) {
    uint32_t entries;
    uint32_t block_size;
    size_t bytes;
    uint8_t *fat;

    entries = fat16_fat_entry_count(fs);
    block_size = fs->bpb.bytes_per_sector;
    bytes = entries * sizeof(uint16_t);

    fat = malloc(bytes);
    if (fat == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < fs->bpb.sectors_per_fat; i++) {
        int read_result;

        read_result = fat16_read_block(fd,
                                       fs->first_fat_sector + i,
                                       block_size,
                                       fat + i * block_size);
        if (read_result != 1) {
            free(fat);
            return NULL;
        }
    }

    return (uint16_t *)fat;
}

/*
 * Reads one complete FAT16 cluster into memory.
 *
 * Implemented to iterate through all the sectors/blocks contained within
 * a single cluster, performing multi-block reading sequentially.
 */
int fat16_read_cluster(int fd, const FAT16_FS *fs, uint16_t cluster, void *buffer) {
    uint32_t first_block = fs->first_data_sector +
                           (cluster - FAT16_FIRST_DATA_CLUSTER) *
                           fs->bpb.sectors_per_cluster;

    uint8_t *byte_buffer = (uint8_t *)buffer;
    uint32_t block_size = fs->bpb.bytes_per_sector;

    for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++) {
        uint8_t *block_destination = byte_buffer + (i * block_size);
        
        int res = fat16_read_block(fd, first_block + i, block_size, block_destination);
        if (res != 1) {
            return res; 
        }
    }

    return 1; 
}

int fat16_write_root_dir(int fd, const FAT16_FS *fs, const FAT16_DirEntry *root_dir) {
    uint32_t block_size = fs->bpb.bytes_per_sector;
    uint32_t first_block = fs->first_root_dir_sector;
    const uint8_t *byte_ptr = (const uint8_t *)root_dir;

    for (uint32_t i = 0; i < fs->root_dir_sectors; i++) {
        int res = fat16_write_block(fd, first_block + i, block_size, byte_ptr + i * block_size);
        if (res != 1) {
            return res; 
        }
    }
    return 1; 
}

int fat16_write_fat(int fd, const FAT16_FS *fs, const uint16_t *fat) {
    uint32_t block_size = fs->bpb.bytes_per_sector;
    const uint8_t *byte_ptr = (const uint8_t *)fat;

    for (uint8_t k = 0; k < fs->bpb.fat_count; k++) {

        uint32_t fat_start_block = fs->first_fat_sector + (k * fs->bpb.sectors_per_fat);

        for (uint32_t i = 0; i < fs->bpb.sectors_per_fat; i++) {
            int res = fat16_write_block(fd, fat_start_block + i, block_size, byte_ptr + i * block_size);
            if (res != 1) {
                return res;
            }
        }
    }
    return 1;
}

int fat16_find_root_entry_index(const FAT16_FS *fs, const FAT16_DirEntry *root_dir, const char fat_name[11], uint32_t *index) {
    for (uint32_t i = 0; i < fs->bpb.root_entry_count; i++) {
        FAT16_DirEntry entry = root_dir[i];

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_FREE) {
            break;
        }

        if ((uint8_t)entry.name[0] == FAT16_DIR_ENTRY_DELETED) {
            continue;
        }
        if ((entry.attributes & FAT16_ATTR_LONG_NAME) == FAT16_ATTR_LONG_NAME) {
            continue;
        }
        if (entry.attributes & FAT16_ATTR_VOLUME_ID) {
            continue;
        }

        if (memcmp(entry.name, fat_name, 8) == 0 &&
            memcmp(entry.ext, fat_name + 8, 3) == 0) {
            *index = i;
            return 1;
        }
    }
    return 0; 
}