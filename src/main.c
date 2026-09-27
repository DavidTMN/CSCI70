/* This main compiles and runs the code  */

/* Turns on the POSIX functions used to read folders. */
#define _POSIX_C_SOURCE 200809L

/* Standard libraries for folders, files, memory, and strings. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Our scanner and parser modules. */
#include "scanner.h"
#include "parser.h"

/* Longest file path the program can handle. */
#define MAX_PATH_LEN 1024

/* Most input files the program can process in one run. */
#define MAX_FILES 256

/* Inputs the file */
static int is_input_file(const char *name)
{
    size_t len = strlen(name);
    return strstr(name, "input") != NULL
        && len > 4 && strcmp(name + len - 4, ".txt") == 0;
}

/* Makes the output file name by replacing input in the name. */
static void output_name(char *dest, size_t size, const char *name, const char *replacement)
{
    const char *pos = strstr(name, "input");
    snprintf(dest, size, "%.*s%s%s", (int)(pos - name), name, replacement, pos + strlen("input"));
}

/* Writes all tokens to the scan file. Stops at the first lexical error. */
static int scan_file(FILE *in, FILE *out)
{
    Token t;

    scanner_init(in);
    do {
        t = gettoken();
        print_token(out, &t);
    } while (t.type != T_ENDOFFILE && t.type != T_ERROR);
    return t.type != T_ERROR;
}

/* Scans and parses one input file. */
static void process_file(const char *dir, const char *name)
{
    char scan_name[MAX_PATH_LEN], parse_name[MAX_PATH_LEN];
    char in_path[2 * MAX_PATH_LEN], scan_path[2 * MAX_PATH_LEN], parse_path[2 * MAX_PATH_LEN];
    FILE *in, *scan_out, *parse_out;
    int scanned_ok, valid = 0;

    output_name(scan_name, sizeof scan_name, name, "output_scan");
    output_name(parse_name, sizeof parse_name, name, "output_parse");
    snprintf(in_path, sizeof in_path, "%s/%s", dir, name);
    snprintf(scan_path, sizeof scan_path, "%s/%s", dir, scan_name);
    snprintf(parse_path, sizeof parse_path, "%s/%s", dir, parse_name);

    in = fopen(in_path, "r");
    if (in == NULL) {
        fprintf(stderr, "Cannot open %s\n", in_path);
        return;
    }
    scan_out = fopen(scan_path, "w");
    parse_out = fopen(parse_path, "w");
    if (scan_out == NULL || parse_out == NULL) {
        fprintf(stderr, "Cannot create output files for %s\n", in_path);
        fclose(in);
        if (scan_out) fclose(scan_out);
        if (parse_out) fclose(parse_out);
        return;
    }

    /* First pass runs the scanner. */
    scanned_ok = scan_file(in, scan_out);

    /* Second pass runs the parser only if scanning had no errors. */
    if (scanned_ok) {
        rewind(in);
        valid = parse(in, parse_out, name);
    } else {
        fprintf(parse_out, "Parsing skipped: lexical error found in %s\n", name);
    }

    printf("%-30s %s\n", name,
           !scanned_ok ? "lexical error" : valid ? "valid" : "syntax error");

    fclose(in);
    fclose(scan_out);
    fclose(parse_out);
}

/* Compares two file names for sorting. */
static int compare_names(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* Processes every input file in the given folder. */
int main(int argc, char *argv[])
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char *files[MAX_FILES];
    int count = 0, i;
    struct dirent *entry;
    DIR *d = opendir(dir);

    if (d == NULL) {
        fprintf(stderr, "Cannot open directory %s\n", dir);
        return 1;
    }
    /* Collects the input file names first. */
    while ((entry = readdir(d)) != NULL && count < MAX_FILES) {
        if (is_input_file(entry->d_name)) {
            files[count] = malloc(strlen(entry->d_name) + 1);
            strcpy(files[count], entry->d_name);
            count++;
        }
    }
    closedir(d);

    if (count == 0) {
        printf("No input files (*input*.txt) found in %s\n", dir);
        return 0;
    }

    qsort(files, count, sizeof files[0], compare_names);
    for (i = 0; i < count; i++) {
        process_file(dir, files[i]);
        free(files[i]);
    }
    return 0;
}
