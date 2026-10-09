#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
    CLOSE_ERROR
};

int read_flag(const char *text, char *flag)
{
    if (text == NULL || flag == NULL) return INVALID_ARGUMENT;
    if (strlen(text) != 2) return INVALID_FLAG;
    if (text[0] != '-' && text[0] != '/') return INVALID_FLAG;
    if (text[1] != 'r' && text[1] != 'a') return INVALID_FLAG;

    *flag = text[1];
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

int read_word(FILE *file, char **word, size_t *length, int *finished)
{
    char *buffer;
    char *new_buffer;
    size_t capacity = 64;
    size_t size = 0;
    int symbol;

    if (file == NULL || word == NULL || length == NULL || finished == NULL)
        return INVALID_ARGUMENT;

    *word = NULL;
    *length = 0;
    *finished = 0;

    symbol = fgetc(file);
    while (symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r') {
        symbol = fgetc(file);
    }

    if (symbol == EOF) {
        if (ferror(file)) return READ_ERROR;
        *finished = 1;
        return OK;
    }

    buffer = malloc(capacity);
    if (buffer == NULL) return MEMORY_ERROR;

    while (symbol != EOF && symbol != ' ' && symbol != '\t' &&
           symbol != '\n' && symbol != '\r') {
        if (size + 1 >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return MEMORY_ERROR;
            }
            new_buffer = realloc(buffer, capacity * 2);
            if (new_buffer == NULL) {
                free(buffer);
                return MEMORY_ERROR;
            }
            buffer = new_buffer;
            capacity *= 2;
        }

        buffer[size] = (char)symbol;
        size++;
        symbol = fgetc(file);
    }

    if (ferror(file)) {
        free(buffer);
        return READ_ERROR;
    }

    buffer[size] = '\0';
    *word = buffer;
    *length = size;
    return OK;
}

int transform_word(const char *word, const size_t length, const int position,
                   char **result, size_t *result_length)
{
    char *buffer;
    char digits[4];
    size_t i;
    size_t size = 0;
    int symbol;
    int base = 0;
    int count;
    int j;

    if (word == NULL || result == NULL || result_length == NULL || length == 0)
        return INVALID_ARGUMENT;
    if (position < 1 || position > 10) return INVALID_ARGUMENT;
    if (length > (SIZE_MAX - 1) / 4) return MEMORY_ERROR;

    buffer = malloc(length * 4 + 1);
    if (buffer == NULL) return MEMORY_ERROR;

    if (position == 10) base = 4;
    else if (position == 5) base = 8;

    for (i = 0; i < length; i++) {
        symbol = (unsigned char)word[i];

        if (position % 2 == 0 && symbol >= 'A' && symbol <= 'Z') {
            symbol = symbol - 'A' + 'a';
        }

        if (base == 0) {
            buffer[size] = (char)symbol;
            size++;
        } else {
            count = 0;
            do {
                digits[count] = (char)('0' + symbol % base);
                count++;
                symbol /= base;
            } while (symbol != 0);

            for (j = count - 1; j >= 0; j--) {
                buffer[size] = digits[j];
                size++;
            }
        }
    }

    buffer[size] = '\0';
    *result = buffer;
    *result_length = size;
    return OK;
}

int write_word(FILE *file, const char *word, const size_t length,
               const int prepend_space)
{
    if (file == NULL || word == NULL || length == 0) return INVALID_ARGUMENT;
    if (prepend_space != 0 && prepend_space != 1) return INVALID_ARGUMENT;

    if (prepend_space == 1 && fputc(' ', file) == EOF) return WRITE_ERROR;
    if (fwrite(word, 1, length, file) != length) return WRITE_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    char flag = '\0';
    const char *output_path = NULL;
    FILE *input1 = NULL;
    FILE *input2 = NULL;
    FILE *output = NULL;
    char *word = NULL;
    char *changed = NULL;
    size_t length = 0;
    size_t changed_length = 0;
    int end1 = 0;
    int end2 = 0;
    int finished = 0;
    int turn = 0;
    int position = 1;
    int prepend_space = 0;
    int status = OK;

    if (argc < 2) status = INVALID_COUNT;
    else status = read_flag(argv[1], &flag);

    if (status == OK) {
        if (flag == 'r' && argc != 5) status = INVALID_COUNT;
        if (flag == 'a' && argc != 4) status = INVALID_COUNT;
    }

    if (status == OK) {
        output_path = argv[argc - 1];
        status = check_paths(argv[2], output_path);
        if (status == OK && flag == 'r') status = check_paths(argv[3], output_path);
    }

    if (status == OK) {
        input1 = fopen(argv[2], "rb");
        if (input1 == NULL) status = FILE_ERROR;
    }
    if (status == OK && flag == 'r') {
        input2 = fopen(argv[3], "rb");
        if (input2 == NULL) status = FILE_ERROR;
    }
    if (status == OK) {
        output = fopen(output_path, "wb");
        if (output == NULL) status = FILE_ERROR;
    }

    if (flag == 'r') {
        while (status == OK && (end1 == 0 || end2 == 0)) {
            if ((turn == 0 && end1 == 0) || end2 == 1) {
                status = read_word(input1, &word, &length, &finished);
                end1 = finished;
            } else {
                status = read_word(input2, &word, &length, &finished);
                end2 = finished;
            }

            if (status == OK && finished == 0) {
                status = write_word(output, word, length, prepend_space);
                prepend_space = 1;
            }

            free(word);
            word = NULL;
            turn = 1 - turn;
        }
    } else if (flag == 'a') {
        while (status == OK) {
            status = read_word(input1, &word, &length, &finished);
            if (status != OK || finished == 1) break;

            status = transform_word(word, length, position, &changed, &changed_length);
            if (status == OK) {
                status = write_word(output, changed, changed_length, prepend_space);
                prepend_space = 1;
            }

            free(word);
            word = NULL;
            free(changed);
            changed = NULL;

            position++;
            if (position > 10) position = 1;
        }
    }

    free(word);
    free(changed);

    if (input1 != NULL) {
        if (fclose(input1) != 0 && status == OK) status = CLOSE_ERROR;
    }
    if (input2 != NULL) {
        if (fclose(input2) != 0 && status == OK) status = CLOSE_ERROR;
    }
    if (output != NULL) {
        if (fclose(output) != 0 && status == OK) status = WRITE_ERROR;
    }

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Неверные параметры или пустой путь к файлу.\n");
            break;
        case INVALID_FLAG:
            fprintf(stderr, "Нужен флаг -r, /r, -a или /a.\n");
            break;
        case INVALID_COUNT:
            fprintf(stderr, "Использование:\n");
            fprintf(stderr, "  %s -r file1.txt file2.txt output.txt\n", argv[0]);
            fprintf(stderr, "  %s -a input.txt output.txt\n", argv[0]);
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память для лексемы.\n");
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть файл или проверить путь к нему.\n");
            break;
        case SAME_FILE:
            fprintf(stderr, "Выходной файл не должен совпадать с входным.\n");
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
        default:
            fprintf(stderr, "Неизвестная ошибка.\n");
            break;
    }
    return 1;
}
