#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_FLAG,
    INVALID_COUNT,
    MEMORY_ERROR,
    FILE_ERROR,
    SAME_FILE,
    READ_ERROR,
    WRITE_ERROR,
    CLOSE_ERROR,
    COUNT_OVERFLOW
};

int read_flag(const char *text, char *operation, int *explicit_output)
{
    size_t length;
    int index;

    if (text == NULL || operation == NULL || explicit_output == NULL)
        return INVALID_ARGUMENT;

    length = strlen(text);
    if (length != 2 && length != 3) return INVALID_FLAG;
    if (text[0] != '-' && text[0] != '/') return INVALID_FLAG;

    if (length == 3) {
        if (text[1] != 'n') return INVALID_FLAG;
        index = 2;
    } else {
        index = 1;
    }

    if (text[index] != 'd' && text[index] != 'i' &&
        text[index] != 's' && text[index] != 'a') return INVALID_FLAG;

    *operation = text[index];
    *explicit_output = index - 1;
    return OK;
}

int make_output_path(const char *input_path, char **output_path)
{
    size_t length;
    size_t start = 0;
    size_t i;
    char *path;

    if (input_path == NULL || output_path == NULL) return INVALID_ARGUMENT;
    length = strlen(input_path);
    if (length == 0) return INVALID_ARGUMENT;
    if (length > SIZE_MAX - 5) return MEMORY_ERROR;

    for (i = 0; i < length; i++) {
        if (input_path[i] == '/') start = i + 1;
    }
    if (start == length) return INVALID_ARGUMENT;

    path = malloc(length + 5);
    if (path == NULL) return MEMORY_ERROR;

    for (i = 0; i < start; i++) path[i] = input_path[i];
    path[start] = 'o';
    path[start + 1] = 'u';
    path[start + 2] = 't';
    path[start + 3] = '_';
    for (i = start; i <= length; i++) path[i + 4] = input_path[i];

    *output_path = path;
    return OK;
}

int check_paths(const char *input_path, const char *output_path)
{
    struct stat input_info;
    struct stat output_info;

    if (input_path == NULL || output_path == NULL) return INVALID_ARGUMENT;
    if (input_path[0] == '\0' || output_path[0] == '\0') return INVALID_ARGUMENT;

    if (stat(input_path, &input_info) != 0) return FILE_ERROR;
    if (stat(output_path, &output_info) != 0) {
        if (errno == ENOENT) return OK;
        return FILE_ERROR;
    }

    if (input_info.st_dev == output_info.st_dev &&
        input_info.st_ino == output_info.st_ino) return SAME_FILE;

    return OK;
}

int transform_symbol(const int symbol, const char operation,
                     char output[2], int *length)
{
    const char digits[] = "0123456789ABCDEF";

    if (output == NULL || length == NULL) return INVALID_ARGUMENT;
    if (symbol < 0 || symbol > 255) return INVALID_ARGUMENT;
    if (operation != 'd' && operation != 'a') return INVALID_FLAG;

    if (operation == 'd') {
        if (symbol >= '0' && symbol <= '9') {
            *length = 0;
        } else {
            output[0] = (char)symbol;
            *length = 1;
        }
    } else {
        if (symbol >= '0' && symbol <= '9') {
            output[0] = (char)symbol;
            *length = 1;
        } else if (symbol < 16) {
            output[0] = digits[symbol];
            *length = 1;
        } else {
            output[0] = digits[symbol / 16];
            output[1] = digits[symbol % 16];
            *length = 2;
        }
    }
    return OK;
}

int count_symbol(const int symbol, const char operation,
                 unsigned long long *count)
{
    int letter;
    int digit;
    int add = 0;

    if (count == NULL) return INVALID_ARGUMENT;
    if (symbol < 0 || symbol > 255) return INVALID_ARGUMENT;
    if (operation != 'i' && operation != 's') return INVALID_FLAG;

    letter = (symbol >= 'A' && symbol <= 'Z') ||
             (symbol >= 'a' && symbol <= 'z');
    digit = symbol >= '0' && symbol <= '9';

    if (operation == 'i' && letter) add = 1;
    if (operation == 's' && !letter && !digit && symbol != ' ') add = 1;

    if (add == 1) {
        if (*count == ULLONG_MAX) return COUNT_OVERFLOW;
        (*count)++;
    }
    return OK;
}

int main(int argc, char *argv[])
{
    char operation = '\0';
    int explicit_output = 0;
    char *generated_path = NULL;
    const char *output_path = NULL;
    FILE *input = NULL;
    FILE *output = NULL;
    char buffer[2];
    unsigned long long count = 0;
    int length = 0;
    int has_line = 0;
    int symbol;
    int next;
    int status = OK;

    if (argc < 2) status = INVALID_COUNT;
    else status = read_flag(argv[1], &operation, &explicit_output);

    if (status == OK) {
        if (explicit_output == 0 && argc != 3) status = INVALID_COUNT;
        if (explicit_output == 1 && argc != 4) status = INVALID_COUNT;
    }

    if (status == OK) {
        if (explicit_output == 1) {
            output_path = argv[3];
        } else {
            status = make_output_path(argv[2], &generated_path);
            output_path = generated_path;
        }
    }

    if (status == OK) status = check_paths(argv[2], output_path);

    if (status == OK) {
        input = fopen(argv[2], "rb");
        if (input == NULL) status = FILE_ERROR;
    }
    if (status == OK) {
        output = fopen(output_path, "wb");
        if (output == NULL) status = FILE_ERROR;
    }

    while (status == OK) {
        symbol = fgetc(input);
        if (symbol == EOF) break;

        if (operation == 'd' || operation == 'a') {
            status = transform_symbol(symbol, operation, buffer, &length);
            if (status == OK && length > 0) {
                if (fwrite(buffer, 1, length, output) != (size_t)length)
                    status = WRITE_ERROR;
            }
        } else {
            if (symbol == '\r') {
                next = fgetc(input);
                if (next == '\n') symbol = '\n';
                else if (next != EOF && ungetc(next, input) == EOF) status = READ_ERROR;
            }

            if (status == OK) {
                if (symbol == '\n') {
                    if (fprintf(output, "%llu\n", count) < 0) status = WRITE_ERROR;
                    count = 0;
                    has_line = 0;
                } else {
                    has_line = 1;
                    status = count_symbol(symbol, operation, &count);
                }
            }
        }
    }

    if (input != NULL && ferror(input)) status = READ_ERROR;

    if (status == OK && (operation == 'i' || operation == 's') && has_line == 1) {
        if (fprintf(output, "%llu\n", count) < 0) status = WRITE_ERROR;
    }

    if (input != NULL) {
        if (fclose(input) != 0 && status == OK) status = CLOSE_ERROR;
    }
    if (output != NULL) {
        if (fclose(output) != 0 && status == OK) status = WRITE_ERROR;
    }
    free(generated_path);

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Неверные параметры или пустой путь к файлу.\n");
            break;
        case INVALID_FLAG:
            fprintf(stderr, "Нужен флаг -d, -i, -s, -a, -nd, -ni, -ns или -na.\n");
            fprintf(stderr, "Вместо '-' можно использовать '/'.\n");
            break;
        case INVALID_COUNT:
            fprintf(stderr, "Использование:\n");
            fprintf(stderr, "  %s -d input.txt\n", argv[0]);
            fprintf(stderr, "  %s -nd input.txt output.txt\n", argv[0]);
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память для пути.\n");
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть файл или проверить путь к нему.\n");
            break;
        case SAME_FILE:
            fprintf(stderr, "Входной и выходной файлы не должны совпадать.\n");
            break;
        case READ_ERROR:
            fprintf(stderr, "Ошибка чтения файла.\n");
            break;
        case WRITE_ERROR:
            fprintf(stderr, "Ошибка записи файла.\n");
            break;
        case CLOSE_ERROR:
            fprintf(stderr, "Ошибка закрытия файла.\n");
            break;
        case COUNT_OVERFLOW:
            fprintf(stderr, "Количество символов не помещается в unsigned long long.\n");
            break;
        default:
            fprintf(stderr, "Неизвестная ошибка.\n");
            break;
    }
    return 1;
}