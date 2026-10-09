#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_BASE,
    INVALID_NUMBER,
    NUMBER_OVERFLOW,
    SUM_OVERFLOW,
    MEMORY_ERROR,
    READ_ERROR,
    END_OF_INPUT,
    EMPTY_INPUT,
    BUFFER_TOO_SMALL,
    OUTPUT_ERROR
};

int read_word(FILE *file, char **word, size_t *length)
{
    char *buffer;
    char *new_buffer;
    size_t capacity = 64;
    size_t size = 0;
    int symbol;

    if (file == NULL || word == NULL || length == NULL) return INVALID_ARGUMENT;
    *word = NULL;
    *length = 0;

    symbol = fgetc(file);
    while (symbol != EOF && isspace((unsigned char)symbol)) symbol = fgetc(file);

    if (symbol == EOF) {
        if (ferror(file)) return READ_ERROR;
        return END_OF_INPUT;
    }

    buffer = malloc(capacity);
    if (buffer == NULL) return MEMORY_ERROR;

    while (symbol != EOF && !isspace((unsigned char)symbol)) {
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

int parse_number(const char *word, const size_t length, const int base,
                 long long *result)
{
    size_t start = 0;
    size_t i;
    int negative = 0;
    int digit;
    unsigned long long value = 0;
    unsigned long long limit = LLONG_MAX;

    if (word == NULL || result == NULL) return INVALID_ARGUMENT;
    if (base < 2 || base > 36) return INVALID_BASE;
    if (length == 0) return INVALID_NUMBER;

    if (word[0] == '-' || word[0] == '+') {
        if (word[0] == '-') negative = 1;
        start = 1;
    }
    if (start == length) return INVALID_NUMBER;
    if (negative == 1) limit = (unsigned long long)(-(LLONG_MIN + 1)) + 1;

    for (i = start; i < length; i++) {
        if (word[i] >= '0' && word[i] <= '9') digit = word[i] - '0';
        else if (word[i] >= 'A' && word[i] <= 'Z') digit = word[i] - 'A' + 10;
        else return INVALID_NUMBER;

        if (digit >= base) return INVALID_NUMBER;
        if (value > (limit - digit) / base) return NUMBER_OVERFLOW;
        value = value * base + digit;
    }

    if (negative == 1) {
        if (value == limit) *result = LLONG_MIN;
        else *result = -(long long)value;
    } else {
        *result = (long long)value;
    }
    return OK;
}

int update_values(const long long number, const int has_values,
                  long long *maximum, long long *sum)
{
    unsigned long long magnitude;
    unsigned long long maximum_magnitude;

    if (maximum == NULL || sum == NULL || maximum == sum) return INVALID_ARGUMENT;
    if (has_values != 0 && has_values != 1) return INVALID_ARGUMENT;

    if (has_values == 0) {
        *maximum = number;
        *sum = number;
        return OK;
    }

    if (number > 0 && *sum > LLONG_MAX - number) return SUM_OVERFLOW;
    if (number < 0 && *sum < LLONG_MIN - number) return SUM_OVERFLOW;

    if (number < 0) magnitude = (unsigned long long)(-(number + 1)) + 1;
    else magnitude = (unsigned long long)number;

    if (*maximum < 0) maximum_magnitude = (unsigned long long)(-(*maximum + 1)) + 1;
    else maximum_magnitude = (unsigned long long)*maximum;

    if (magnitude > maximum_magnitude) *maximum = number;
    *sum += number;
    return OK;
}

int convert_to_base(const long long number, const int base,
                    char *result, const size_t capacity)
{
    const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char reversed[sizeof(long long) * CHAR_BIT];
    unsigned long long value;
    size_t count = 0;
    size_t position = 0;
    size_t i;
    int negative = 0;

    if (result == NULL) return INVALID_ARGUMENT;
    if (base < 2 || base > 36) return INVALID_BASE;

    if (number < 0) {
        negative = 1;
        value = (unsigned long long)(-(number + 1)) + 1;
    } else {
        value = (unsigned long long)number;
    }

    do {
        reversed[count] = alphabet[value % base];
        count++;
        value /= base;
    } while (value != 0);

    if (capacity < count + negative + 1) return BUFFER_TOO_SMALL;

    if (negative == 1) {
        result[position] = '-';
        position++;
    }
    for (i = count; i > 0; i--) {
        result[position] = reversed[i - 1];
        position++;
    }
    result[position] = '\0';
    return OK;
}

int print_results(const long long maximum, const long long sum, const int base)
{
    int bases[5] = {base, 9, 18, 27, 36};
    char maximum_text[sizeof(long long) * CHAR_BIT + 2];
    char sum_text[sizeof(long long) * CHAR_BIT + 2];
    int i;
    int status;

    if (base < 2 || base > 36) return INVALID_BASE;
    if (printf("Основание | Максимальное по модулю | Сумма\n") < 0) return OUTPUT_ERROR;

    for (i = 0; i < 5; i++) {
        status = convert_to_base(maximum, bases[i], maximum_text, sizeof maximum_text);
        if (status != OK) return status;
        status = convert_to_base(sum, bases[i], sum_text, sizeof sum_text);
        if (status != OK) return status;

        if (printf("%d | %s | %s\n", bases[i], maximum_text, sum_text) < 0)
            return OUTPUT_ERROR;
    }
    return OK;
}

int main(const int argc, char *argv[])
{
    char *word = NULL;
    size_t length = 0;
    long long base_value = 0;
    long long number = 0;
    long long maximum = 0;
    long long sum = 0;
    int base = 0;
    int has_values = 0;
    int status = OK;

    if (argc != 1) {
        fprintf(stderr, "Использование: %s (без аргументов)\n", argv[0]);
        return 1;
    }

    if (printf("Введите основание системы счисления [2..36]:\n") < 0)
        status = OUTPUT_ERROR;
    if (fflush(stdout) == EOF) status = OUTPUT_ERROR;

    if (status == OK) status = read_word(stdin, &word, &length);
    if (status == OK) status = parse_number(word, length, 10, &base_value);
    if (status == OK) {
        if (base_value < 2 || base_value > 36) status = INVALID_BASE;
        else base = (int)base_value;
    }
    free(word);
    word = NULL;

    if (status == OK) {
        if (printf("Введите целые числа. Для окончания ввода введите Stop:\n") < 0)
            status = OUTPUT_ERROR;
        if (fflush(stdout) == EOF) status = OUTPUT_ERROR;
    }

    while (status == OK) {
        status = read_word(stdin, &word, &length);
        if (status != OK) break;

        if (length == 4 && strcmp(word, "Stop") == 0) break;

        status = parse_number(word, length, base, &number);
        if (status == OK) status = update_values(number, has_values, &maximum, &sum);
        if (status == OK) has_values = 1;

        free(word);
        word = NULL;
    }

    free(word);

    if (status == OK && has_values == 0) status = EMPTY_INPUT;
    if (status == OK) status = print_results(maximum, sum, base);
    if (fflush(stdout) == EOF) status = OUTPUT_ERROR;

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Неверные параметры функции.\n");
            break;
        case INVALID_BASE:
            fprintf(stderr, "Основание должно быть целым числом от 2 до 36.\n");
            break;
        case INVALID_NUMBER:
            fprintf(stderr, "Некорректное число. Допустимы знак, цифры и прописные латинские буквы в выбранной системе.\n");
            break;
        case NUMBER_OVERFLOW:
            fprintf(stderr, "Введённое число не помещается в long long.\n");
            break;
        case SUM_OVERFLOW:
            fprintf(stderr, "Сумма не помещается в long long.\n");
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память для ввода.\n");
            break;
        case READ_ERROR:
            fprintf(stderr, "Ошибка чтения.\n");
            break;
        case END_OF_INPUT:
            fprintf(stderr, "Ввод завершён до получения основания или команды Stop.\n");
            break;
        case EMPTY_INPUT:
            fprintf(stderr, "Не введено ни одного числа.\n");
            break;
        case BUFFER_TOO_SMALL:
            fprintf(stderr, "Недостаточно места для записи результата.\n");
            break;
        case OUTPUT_ERROR:
            fprintf(stderr, "Ошибка вывода.\n");
            break;
        default:
            fprintf(stderr, "Неизвестная ошибка.\n");
            break;
    }
    return 1;
}
