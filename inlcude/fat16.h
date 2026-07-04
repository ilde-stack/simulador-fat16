#ifndef FAT16_H
#define FAT16_H

/*
 * fat16.h
 *
 * Definiciones básicas para trabajar con una imagen FAT16.
 *
 * Este archivo está pensado con fines educativos. No pretende cubrir toda
 * la especificación FAT ni todos los casos especiales, sino brindar las
 * estructuras necesarias para inspeccionar y modificar una imagen FAT16
 * sencilla, como las creadas con:
 *
 * dd if=/dev/zero of=fat.img bs=1M count=10
 * mkfs.fat -F 16 fat.img
 *
 * Organización general de un volumen FAT16:
 *
 * +-----------------------------+
 * | Boot Sector / BPB           |
 * +-----------------------------+
 * | Sectores reservados         |
 * +-----------------------------+
 * | FAT #1                      |
 * +-----------------------------+
 * | FAT #2                      |
 * +-----------------------------+
 * | Root Directory              |
 * +-----------------------------+
 * | Data Area                   |
 * +-----------------------------+
 *
 * FAT16 usa valores little-endian en disco.
 *
 * En máquinas x86/x86_64 esto coincide con el orden nativo de la CPU.
 * En arquitecturas big-endian habría que convertir explícitamente los
 * campos de 16 y 32 bits al leerlos.
 */

#include <stdint.h>
#include <stddef.h>

/*
 * Valores especiales de FAT16.
 */

#define FAT16_FREE_CLUSTER      0x0000
#define FAT16_BAD_CLUSTER       0xFFF7
#define FAT16_EOC_MIN           0xFFF8
#define FAT16_EOC_MAX           0xFFFF
#define FAT16_EOC               0xFFFF

/*
 * Cluster inicial válido.
 *
 * En FAT, las entradas FAT[0] y FAT[1] están reservadas.
 * El primer cluster de datos es el cluster 2.
 */

#define FAT16_FIRST_DATA_CLUSTER 2

/*
 * Valores especiales en el primer byte de una entrada de directorio.
 */

#define FAT16_DIR_ENTRY_FREE      0x00  /* No hay más entradas usadas. */
#define FAT16_DIR_ENTRY_DELETED   0xE5  /* Entrada borrada. */

/*
 * Atributos de una entrada de directorio FAT.
 *
 * FAT no almacena permisos Unix rwx, usuario ni grupo.
 * Estos atributos son mucho más simples.
 */

#define FAT16_ATTR_READ_ONLY  0x01
#define FAT16_ATTR_HIDDEN     0x02
#define FAT16_ATTR_SYSTEM     0x04
#define FAT16_ATTR_VOLUME_ID  0x08
#define FAT16_ATTR_DIRECTORY  0x10
#define FAT16_ATTR_ARCHIVE    0x20

/*
 * Combinación usada por entradas de nombres largos VFAT.
 * Para el ejercicio podemos ignorarlas.
 */

#define FAT16_ATTR_LONG_NAME \
    (FAT16_ATTR_READ_ONLY | FAT16_ATTR_HIDDEN | FAT16_ATTR_SYSTEM | FAT16_ATTR_VOLUME_ID)

/*
 * Todas las estructuras que representan datos en disco deben estar
 * empaquetadas. El compilador normalmente agrega padding para alinear
 * campos; eso rompería la correspondencia exacta con los bytes del disco.
 */

#pragma pack(push, 1)

/*
 * BIOS Parameter Block + parte extendida FAT16.
 *
 * Esta estructura comienza en el byte 0 del volumen.
 *
 * Importante:
 * - Los campos numéricos están almacenados en little-endian.
 * - Los campos de texto no necesariamente terminan en '\0'.
 * - Esta estructura describe cómo interpretar el resto del filesystem.
 */

typedef struct FAT16_BootSector {
    /*
     * Offset 0x00, tamaño 3.
     *
     * Instrucción de salto usada históricamente en discos booteables.
     * En una imagen no booteable igual suele estar presente.
     */
    uint8_t jump[3];

    /*
     * Offset 0x03, tamaño 8.
     *
     * Nombre del sistema que formateó el volumen.
     * Ejemplo: "mkfs.fat".
     *
     * No necesariamente termina en '\0'.
     */
    char oem_name[8];

    /*
     * Offset 0x0B, tamaño 2.
     *
     * Cantidad de bytes por sector lógico.
     * Valor habitual: 512.
     *
     * Little-endian.
     */
    uint16_t bytes_per_sector;

    /*
     * Offset 0x0D, tamaño 1.
     *
     * Cantidad de sectores por cluster.
     *
     * Ejemplo:
     * bytes_per_sector = 512
     * sectors_per_cluster = 4
     * cluster_size = 2048 bytes
     */
    uint8_t sectors_per_cluster;

    /*
     * Offset 0x0E, tamaño 2.
     *
     * Cantidad de sectores reservados antes de la primera FAT.
     * Incluye el Boot Sector.
     *
     * Little-endian.
     */
    uint16_t reserved_sectors;

    /*
     * Offset 0x10, tamaño 1.
     *
     * Cantidad de copias de la FAT.
     * Normalmente vale 2.
     */
    uint8_t fat_count;

    /*
     * Offset 0x11, tamaño 2.
     *
     * Cantidad máxima de entradas en el directorio raíz.
     *
     * En FAT12/FAT16 el root directory tiene tamaño fijo.
     *
     * Little-endian.
     */
    uint16_t root_entry_count;

    /*
     * Offset 0x13, tamaño 2.
     *
     * Cantidad total de sectores si el volumen tiene menos de 65536 sectores.
     * Si vale 0, debe usarse total_sectors_32.
     *
     * Little-endian.
     */
    uint16_t total_sectors_16;

    /*
     * Offset 0x15, tamaño 1.
     *
     * Descriptor del medio.
     * Valor típico para disco duro o imagen: 0xF8.
     */
    uint8_t media_descriptor;

    /*
     * Offset 0x16, tamaño 2.
     *
     * Tamaño de cada FAT, medido en sectores.
     *
     * Little-endian.
     */
    uint16_t sectors_per_fat;

    /*
     * Offset 0x18, tamaño 2.
     *
     * Sectores por pista.
     * Campo heredado de la geometría CHS clásica.
     *
     * Little-endian.
     */
    uint16_t sectors_per_track;

    /*
     * Offset 0x1A, tamaño 2.
     *
     * Cantidad de cabezas.
     * Campo heredado de la geometría CHS clásica.
     *
     * Little-endian.
     */
    uint16_t head_count;

    /*
     * Offset 0x1C, tamaño 4.
     *
     * Sectores ocultos antes del volumen.
     * Usado cuando el filesystem está dentro de una partición.
     *
     * Little-endian.
     */
    uint32_t hidden_sectors;

    /*
     * Offset 0x20, tamaño 4.
     *
     * Cantidad total de sectores para volúmenes grandes.
     * Se usa si total_sectors_16 vale 0.
     *
     * Little-endian.
     */
    uint32_t total_sectors_32;

    /*
     * Offset 0x24, tamaño 1.
     *
     * Número de unidad BIOS.
     * Campo histórico.
     */
    uint8_t drive_number;

    /*
     * Offset 0x25, tamaño 1.
     *
     * Reservado.
     */
    uint8_t reserved1;

    /*
     * Offset 0x26, tamaño 1.
     *
     * Firma extendida.
     * Normalmente vale 0x29 si están presentes los siguientes campos.
     */
    uint8_t boot_signature;

    /*
     * Offset 0x27, tamaño 4.
     *
     * Número de serie del volumen.
     * mkfs.fat puede generarlo automáticamente o recibirlo con -i.
     *
     * Little-endian.
     */
    uint32_t volume_id;

    /*
     * Offset 0x2B, tamaño 11.
     *
     * Etiqueta del volumen.
     * Ejemplo: "NO NAME    ".
     *
     * No necesariamente termina en '\0'.
     */
    char volume_label[11];

    /*
     * Offset 0x36, tamaño 8.
     *
     * Texto descriptivo del tipo de filesystem.
     * Ejemplo: "FAT16   ".
     *
     * No debe usarse como única forma de validar el filesystem.
     */
    char filesystem_type[8];

    /*
     * Offset 0x3E, tamaño 448.
     *
     * Código de arranque o relleno.
     * Para este ejercicio no se interpreta.
     */
    uint8_t boot_code[448];

    /*
     * Offset 0x1FE, tamaño 2.
     *
     * Firma final del sector de arranque.
     * En disco aparece como bytes:
     *
     * 55 AA
     *
     * Leído como uint16_t little-endian vale 0xAA55.
     */
    uint16_t boot_sector_signature;

} FAT16_BootSector;

/*
 * Entrada de directorio FAT16.
 *
 * Cada entrada ocupa exactamente 32 bytes.
 *
 * En FAT16, el directorio raíz es un array fijo de entradas de este tipo.
 * Cada entrada describe un archivo, un directorio, una etiqueta de volumen
 * o una entrada especial.
 */

typedef struct FAT16_DirEntry {
    /*
     * Offset 0, tamaño 8.
     *
     * Nombre en formato 8.3, parte del nombre.
     *
     * Ejemplo:
     * "HOLA    "
     *
     * No termina en '\0'.
     * Se rellena con espacios.
     *
     * Valores especiales en name[0]:
     * 0x00 = no hay más entradas usadas
     * 0xE5 = entrada borrada
     */
    char name[8];

    /*
     * Offset 8, tamaño 3.
     *
     * Extensión en formato 8.3.
     *
     * Ejemplo:
     * "TXT"
     *
     * No termina en '\0'.
     * Se rellena con espacios.
     */
    char ext[3];

    /*
     * Offset 11, tamaño 1.
     *
     * Atributos del archivo.
     *
     * Bits habituales:
     * 0x01 read-only
     * 0x02 hidden
     * 0x04 system
     * 0x08 volume label
     * 0x10 directory
     * 0x20 archive
     */
    uint8_t attributes;

    /*
     * Offset 12, tamaño 1.
     *
     * Reservado por Windows NT.
     * Para este ejercicio se puede dejar en 0.
     */
    uint8_t nt_reserved;

    /*
     * Offset 13, tamaño 1.
     *
     * Décimas de segundo para la hora de creación.
     * Para este ejercicio se puede dejar en 0.
     */
    uint8_t creation_time_tenths;

    /*
     * Offset 14, tamaño 2.
     *
     * Hora de creación, codificada en formato FAT.
     * Para este ejercicio se puede dejar en 0.
     *
     * Little-endian.
     */
    uint16_t creation_time;

    /*
     * Offset 16, tamaño 2.
     *
     * Fecha de creación, codificada en formato FAT.
     * Para este ejercicio se puede dejar en 0.
     *
     * Little-endian.
     */
    uint16_t creation_date;

    /*
     * Offset 18, tamaño 2.
     *
     * Fecha de último acceso.
     * Para este ejercicio se puede dejar en 0.
     *
     * Little-endian.
     */
    uint16_t last_access_date;

    /*
     * Offset 20, tamaño 2.
     *
     * Parte alta del cluster inicial.
     *
     * En FAT32 se usa para completar un número de cluster de 32 bits.
     * En FAT16 normalmente vale 0.
     *
     * Little-endian.
     */
    uint16_t first_cluster_high;

    /*
     * Offset 22, tamaño 2.
     *
     * Hora de última modificación.
     *
     * Little-endian.
     */
    uint16_t write_time;

    /*
     * Offset 24, tamaño 2.
     *
     * Fecha de última modificación.
     *
     * Little-endian.
     */
    uint16_t write_date;

    /*
     * Offset 26, tamaño 2.
     *
     * Cluster inicial del archivo o directorio.
     *
     * En FAT16 este campo indica dónde empieza la cadena de clusters.
     *
     * Little-endian.
     */
    uint16_t first_cluster_low;

    /*
     * Offset 28, tamaño 4.
     *
     * Tamaño del archivo en bytes.
     *
     * Para directorios suele valer 0.
     *
     * Little-endian.
     */
    uint32_t file_size;

} FAT16_DirEntry;

#pragma pack(pop)

/*
 * Estructura en memoria para representar una imagen FAT16 abierta.
 *
 * Esta estructura NO existe en disco.
 * Es una comodidad para el programa.
 */

typedef struct FAT16_FS {
    FAT16_BootSector bpb;

    uint32_t total_sectors;
    uint32_t root_dir_sectors;
    uint32_t first_fat_sector;
    uint32_t first_root_dir_sector;
    uint32_t first_data_sector;

    uint32_t fat_offset;
    uint32_t root_dir_offset;
    uint32_t data_area_offset;

    uint32_t cluster_size;
    uint32_t cluster_count;
} FAT16_FS;

/*
 * Funciones auxiliares sugeridas.
 *
 * No todas son estrictamente necesarias para una primera versión, pero ayudan
 * a separar responsabilidades.
 */

void fat16_print_fixed_string(const char *label, const char *s, size_t len);
int fat16_read_block(int fd, uint32_t block, uint32_t block_size, void *buffer);
int fat16_write_block(int fd, uint32_t block, uint32_t block_size, const void *buffer);
uint32_t fat16_get_total_sectors(const FAT16_BootSector *bpb);
uint32_t fat16_get_root_dir_sectors(const FAT16_BootSector *bpb);
int fat16_is_probably_fat16(const FAT16_BootSector *bpb);
int fat16_read_boot_sector(int fd, FAT16_BootSector *bpb);
int fat16_calculate_layout(FAT16_FS *fs, const FAT16_BootSector *bpb);
void fat16_format_dir_name(const FAT16_DirEntry *entry, char *name, size_t size);
void fat16_format_attributes(uint8_t attributes, char *text, size_t size);
int fat16_make_fat83_name(const char *name, char fat_name[11]);
int fat16_open_image(const char *path, FAT16_BootSector *bpb, FAT16_FS *fs);
int fat16_find_root_entry(int fd,
                          const FAT16_FS *fs,
                          const char fat_name[11],
                          FAT16_DirEntry *found);
int fat16_find_root_entry_in_memory(const FAT16_FS *fs,
                                    const FAT16_DirEntry *root_dir,
                                    const char fat_name[11],
                                    FAT16_DirEntry *found);
FAT16_DirEntry *fat16_load_root_dir(int fd, const FAT16_FS *fs);
uint32_t fat16_fat_entry_count(const FAT16_FS *fs);
uint16_t *fat16_load_fat(int fd, const FAT16_FS *fs);

int fat16_read_cluster(int fd, const FAT16_FS *fs, uint16_t cluster, void *buffer);
int fat16_write_root_dir(int fd, const FAT16_FS *fs, const FAT16_DirEntry *root_dir);
int fat16_write_fat(int fd, const FAT16_FS *fs, const uint16_t *fat);
int fat16_find_root_entry_index(const FAT16_FS *fs, const FAT16_DirEntry *root_dir, const char fat_name[11], uint32_t *index);

#endif