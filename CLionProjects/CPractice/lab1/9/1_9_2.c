#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_ELEMENT,
    MEMORY_ERROR,
    OUTPUT_ERROR
};

int fill_array(int array[], const int size)
{
    int i;

    if (array == NULL || size <= 0 || size > 10000) return INVALID_ARGUMENT;

    for (i = 0; i < size; i++) {
        array[i] = rand() % 2001 - 1000;
    }
    return OK;
}

int create_c(const int a[], const int size_a, const int b[], const int size_b,
             int **result)
{
    int *c;
    int closest;
    int minimum_distance;
    int distance;
    int i;
    int j;

    if (a == NULL || b == NULL || result == NULL) return INVALID_ARGUMENT;
    if (size_a <= 0 || size_a > 10000 || size_b <= 0 || size_b > 10000)
        return INVALID_ARGUMENT;

    for (i = 0; i < size_a; i++) {
        if (a[i] < -1000 || a[i] > 1000) return INVALID_ELEMENT;
    }
    for (i = 0; i < size_b; i++) {
        if (b[i] < -1000 || b[i] > 1000) return INVALID_ELEMENT;
    }

    c = malloc(size_a * sizeof(int));
    if (c == NULL) return MEMORY_ERROR;

    for (i = 0; i < size_a; i++) {
        closest = b[0];
        minimum_distance = abs(a[i] - b[0]);

        for (j = 1; j < size_b; j++) {
            distance = abs(a[i] - b[j]);
            if (distance < minimum_distance) {
                minimum_distance = distance;
                closest = b[j];
            }
        }

        c[i] = a[i] + closest;
    }

    *result = c;
    return OK;
}

int print_array(const int array[], const int size)
{
    int i;

    if (array == NULL || size <= 0 || size > 10000) return INVALID_ARGUMENT;

    for (i = 0; i < size; i++) {
        if (i > 0 && putchar(' ') == EOF) return OUTPUT_ERROR;
        if (printf("%d", array[i]) < 0) return OUTPUT_ERROR;
    }
    if (putchar('\n') == EOF) return OUTPUT_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    int *a = NULL;
    int *b = NULL;
    int *c = NULL;
    int size_a;
    int size_b;
    int status = OK;
    time_t seed;

    if (argc != 1) {
        fprintf(stderr, "Использование: %s (без аргументов)\n", argv[0]);
        return 1;
    }

    seed = time(NULL);
    if (seed == (time_t)-1) {
        fprintf(stderr, "Не удалось получить время для инициализации генератора.\n");
        return 1;
    }
    srand((unsigned int)seed);

    size_a = rand() % 9991 + 10;
    size_b = rand() % 9991 + 10;

    a = malloc(size_a * sizeof(int));
    if (a == NULL) status = MEMORY_ERROR;

    if (status == OK) {
        b = malloc(size_b * sizeof(int));
        if (b == NULL) status = MEMORY_ERROR;
    }

    if (status == OK) status = fill_array(a, size_a);
    if (status == OK) status = fill_array(b, size_b);
    if (status == OK) status = create_c(a, size_a, b, size_b, &c);

    if (status == OK) {
        if (printf("A, размер %d:\n", size_a) < 0) status = OUTPUT_ERROR;
        if (status == OK) status = print_array(a, size_a);
    }
    if (status == OK) {
        if (printf("B, размер %d:\n", size_b) < 0) status = OUTPUT_ERROR;
        if (status == OK) status = print_array(b, size_b);
    }
    if (status == OK) {
        if (printf("C, размер %d:\n", size_a) < 0) status = OUTPUT_ERROR;
        if (status == OK) status = print_array(c, size_a);
    }

    free(a);
    free(b);
    free(c);

    if (fflush(stdout) == EOF) status = OUTPUT_ERROR;

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Неверные параметры функции.\n");
            break;
        case INVALID_ELEMENT:
            fprintf(stderr, "Элементы A и B должны находиться в диапазоне [-1000..1000].\n");
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память для массива.\n");
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
