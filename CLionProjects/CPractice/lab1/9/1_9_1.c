#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_NUMBER,
    INVALID_RANGE,
    OUTPUT_ERROR
};

enum { ARRAY_SIZE = 10 };

int read_integer(const char *text, int *result)
{
    char *end;
    long value;
    int i = 0;

    if (text == NULL || result == NULL) return INVALID_ARGUMENT;
    if (text[0] == '-' || text[0] == '+') i++;
    if (text[i] == '\0') return INVALID_NUMBER;

    for (; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') return INVALID_NUMBER;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        value < INT_MIN || value > INT_MAX) return INVALID_NUMBER;

    *result = (int)value;
    return OK;
}

int fill_array(int array[], const int size, const int a, const int b)
{
    unsigned long long range;
    unsigned long long random_value;
    int i;

    if (array == NULL || size <= 0) return INVALID_ARGUMENT;
    if (a > b) return INVALID_RANGE;

    range = (unsigned long long)((long long)b - a) + 1;
    for (i = 0; i < size; i++) {
        random_value = (unsigned long long)rand() * ((unsigned long long)RAND_MAX + 1);
        random_value += rand();
        array[i] = (int)((long long)a + (long long)(random_value % range));
    }
    return OK;
}

int swap_min_max(int array[], const int size)
{
    int min_index = 0;
    int max_index = 0;
    int temp;
    int i;

    if (array == NULL || size <= 0) return INVALID_ARGUMENT;

    for (i = 1; i < size; i++) {
        if (array[i] < array[min_index]) min_index = i;
        if (array[i] > array[max_index]) max_index = i;
    }

    temp = array[min_index];
    array[min_index] = array[max_index];
    array[max_index] = temp;
    return OK;
}

int print_array(const int array[], const int size)
{
    int i;

    if (array == NULL || size <= 0) return INVALID_ARGUMENT;

    for (i = 0; i < size; i++) {
        if (i > 0 && putchar(' ') == EOF) return OUTPUT_ERROR;
        if (printf("%d", array[i]) < 0) return OUTPUT_ERROR;
    }
    if (putchar('\n') == EOF) return OUTPUT_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    int array[ARRAY_SIZE];
    int a = 0;
    int b = 0;
    int status;
    time_t seed;

    if (argc != 3) {
        fprintf(stderr, "Использование: %s a b\n", argv[0]);
        return 1;
    }

    status = read_integer(argv[1], &a);
    if (status == OK) status = read_integer(argv[2], &b);
    if (status == OK && a > b) status = INVALID_RANGE;

    if (status == OK) {
        seed = time(NULL);
        if (seed == (time_t)-1) {
            fprintf(stderr, "Не удалось получить время для инициализации генератора.\n");
            return 1;
        }
        srand((unsigned int)seed);
        status = fill_array(array, ARRAY_SIZE, a, b);
    }

    if (status == OK) {
        if (printf("Исходный массив:\n") < 0) status = OUTPUT_ERROR;
        if (status == OK) status = print_array(array, ARRAY_SIZE);
    }

    if (status == OK) status = swap_min_max(array, ARRAY_SIZE);

    if (status == OK) {
        if (printf("После обмена:\n") < 0) status = OUTPUT_ERROR;
        if (status == OK) status = print_array(array, ARRAY_SIZE);
    }

    if (fflush(stdout) == EOF) status = OUTPUT_ERROR;

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Неверные параметры функции.\n");
            break;
        case INVALID_NUMBER:
            fprintf(stderr, "a и b должны быть целыми числами в диапазоне int.\n");
            break;
        case INVALID_RANGE:
            fprintf(stderr, "Должно выполняться a <= b.\n");
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
