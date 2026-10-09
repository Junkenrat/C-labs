#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_NUMBER,
    NUMBER_OVERFLOW,
    MEMORY_ERROR,
    FILE_ERROR,
    SAME_FILE,
    READ_ERROR,
    WRITE_ERROR,
    CLOSE_ERROR
};

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

int digit_value(const unsigned char symbol, int *result)
{
    if (result == NULL) return INVALID_ARGUMENT;

    if (symbol >= '0' && symbol <= '9') *result = symbol - '0';
    else if (symbol >= 'A' && symbol <= 'Z') *result = symbol - 'A' + 10;
    else if (symbol >= 'a' && symbol <= 'z') *result = symbol - 'a' + 10;
    else return INVALID_NUMBER;

    return OK;
}

int convert_number(const char *word, const size_t length, int *base,
                   long long *result, size_t *first_digit)
{
    size_t start = 0;
    size_t i;
    int negative = 0;
    int maximum = 0;
    int digit;
    int current_base;
    int status;
    unsigned long long value = 0;
    unsigned long long limit = LLONG_MAX;
    long long answer;

    if (word == NULL || base == NULL || result == NULL || first_digit == NULL)
        return INVALID_ARGUMENT;
    if (length == 0) return INVALID_NUMBER;

    if (word[0] == '-' || word[0] == '+') {
        if (word[0] == '-') negative = 1;
        start = 1;
    }
    if (start == length) return INVALID_NUMBER;

    for (i = start; i < length; i++) {
        status = digit_value((unsigned char)word[i], &digit);
        if (status != OK) return status;
        if (digit > maximum) maximum = digit;
    }

    current_base = maximum + 1;
    if (current_base < 2) current_base = 2;

    while (start < length - 1 && word[start] == '0') start++;
    if (negative == 1) limit = (unsigned long long)(-(LLONG_MIN + 1)) + 1;

    for (i = start; i < length; i++) {
        status = digit_value((unsigned char)word[i], &digit);
        if (status != OK) return status;
        if (value > (limit - digit) / current_base) return NUMBER_OVERFLOW;
        value = value * current_base + digit;
    }

    if (negative == 1) {
        if (value == limit) answer = LLONG_MIN;
        else answer = -(long long)value;
    } else {
        answer = (long long)value;
    }

    *base = current_base;
    *result = answer;
    *first_digit = start;
    return OK;
}

int write_result(FILE *file, const char *word, const size_t length,
                 const size_t first_digit, const int base, const long long value)
{
    if (file == NULL || word == NULL || first_digit >= length)
        return INVALID_ARGUMENT;
    if (base < 2 || base > 36) return INVALID_ARGUMENT;

    if (value < 0 && fputc('-', file) == EOF) return WRITE_ERROR;
    if (fwrite(word + first_digit, 1, length - first_digit, file) != length - first_digit)
        return WRITE_ERROR;
    if (fprintf(file, " %d %lld\n", base, value) < 0) return WRITE_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    FILE *input = NULL;
    FILE *output = NULL;
    char *word = NULL;
    size_t length = 0;
    size_t first_digit = 0;
    long long value = 0;
    int base = 0;
    int finished = 0;
    int status;

    if (argc != 3) {
        fprintf(stderr, "Использование: %s input.txt output.txt\n", argv[0]);
        return 1;
    }

    status = check_paths(argv[1], argv[2]);
    if (status == OK) {
        input = fopen(argv[1], "rb");
        if (input == NULL) status = FILE_ERROR;
    }
    if (status == OK) {
        output = fopen(argv[2], "wb");
        if (output == NULL) status = FILE_ERROR;
    }

    while (status == OK) {
        status = read_word(input, &word, &length, &finished);
        if (status != OK || finished == 1) break;

        status = convert_number(word, length, &base, &value, &first_digit);
        if (status == OK) {
            status = write_result(output, word, length, first_digit, base, value);
        }

        free(word);
        word = NULL;
    }

    free(word);
    if (input != NULL) {
        if (fclose(input) != 0 && status == OK) status = CLOSE_ERROR;
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
        case INVALID_NUMBER:
            fprintf(stderr, "Некорректное число: допустимы цифры, латинские буквы и знак в начале.\n");
            break;
        case NUMBER_OVERFLOW:
            fprintf(stderr, "Значение числа не помещается в long long.\n");
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память для числа.\n");
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
        default:
            fprintf(stderr, "Неизвестная ошибка.\n");
            break;
    }
    return 1;
}
