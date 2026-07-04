// fat16-sh.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fat16-sh.h"

static void print_help(void) {
    printf("Comandos disponibles:\n");
    printf("  info\n");
    printf("  ls\n");
    printf("  fileclusters archivo\n");
    printf("  cat archivo\n");          
    printf("  rm archivo\n");           
    printf("  help\n");
    printf("  quit\n");
}

static char *next_token(char **line) {
    char *token;

    token = strtok(*line, " \t\r\n");
    *line = NULL;

    return token;
}

/*
 * Program flow:
 *
 * 1. Open the FAT16 image and read the boot sector.
 * 2. Calculate the filesystem layout.
 * 3. Load the root directory and FAT #1 into memory once.
 * 4. Prompt for commands.
 * 5. Run commands using the in-memory metadata.
 *
 * This shell makes it easier to see the difference between startup I/O
 * and later commands that reuse already-loaded filesystem metadata.
 */
int main(int argc, char *argv[]) {
    FAT16_Shell sh;
    char line[256];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s imagen-fat16.img\n", argv[0]);
        return EXIT_FAILURE;
    }

    sh.fd = fat16_open_image(argv[1], &sh.bpb, &sh.fs);
    if (sh.fd == -1) {
        return EXIT_FAILURE;
    }

    sh.root_dir = fat16_load_root_dir(sh.fd, &sh.fs);
    if (sh.root_dir == NULL) {
        fprintf(stderr, "Error: no se pudo cargar el root directory en memoria.\n");
        close(sh.fd);
        return EXIT_FAILURE;
    }

    sh.fat = fat16_load_fat(sh.fd, &sh.fs);
    if (sh.fat == NULL) {
        fprintf(stderr, "Error: no se pudo cargar la FAT en memoria.\n");
        free(sh.root_dir);
        close(sh.fd);
        return EXIT_FAILURE;
    }

    sh.fat_entries = fat16_fat_entry_count(&sh.fs);

    printf("FAT16 shell. Escriba 'help' para ver comandos.\n");

    while (1) {
        char *cursor;
        char *command;
        char *arg;

        printf("fat16> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        cursor = line;
        command = next_token(&cursor);
        if (command == NULL) {
            continue;
        }

        if (strcmp(command, "info") == 0) {
            fat16_sh_info(&sh);
        } else if (strcmp(command, "ls") == 0) {
            fat16_sh_ls(&sh);
        } else if (strcmp(command, "fileclusters") == 0) {
            arg = next_token(&cursor);
            fat16_sh_fileclusters(&sh, arg);
        } else if (strcmp(command, "cat") == 0) {  // <-- AGREGADO: Caso para cat
            arg = next_token(&cursor);
            if (arg == NULL) {
                fprintf(stderr, "Error: se requiere el nombre del archivo. Uso: cat archivo\n");
            } else {
                fat16_sh_cat(&sh, arg);
            }
        } else if (strcmp(command, "rm") == 0) {   // <-- AGREGADO: Caso para rm
            arg = next_token(&cursor);
            if (arg == NULL) {
                fprintf(stderr, "Error: se requiere el nombre del archivo. Uso: rm archivo\n");
            } else {
                fat16_sh_rm(&sh, arg);
            }
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else {
            fprintf(stderr, "Comando desconocido: %s\n", command);
        }
    }

    free(sh.fat);
    free(sh.root_dir);
    close(sh.fd);
    return EXIT_SUCCESS;
}