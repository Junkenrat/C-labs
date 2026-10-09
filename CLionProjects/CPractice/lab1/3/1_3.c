#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_FLAG,
    INVALID_COUNT,
    INVALID_NUMBER,
    INVALID_EPSILON,
    ZERO_INTEGER,
    INVALID_SIDE,
    CALCULATION_ERROR,
    OUTPUT_ERROR
};

enum Solution {
    NO_ROOTS,
    ONE_ROOT,
    TWO_ROOTS,
    INFINITE_ROOTS
};

int read_flag(const char *text, char *flag)
{
    if (text == NULL || flag == NULL) return INVALID_ARGUMENT;
    if (strlen(text) != 2) return INVALID_FLAG;
    if (text[0] != '-' && text[0] != '/') return INVALID_FLAG;
    if (text[1] != 'q' && text[1] != 'm' && text[1] != 't') return INVALID_FLAG;
    *flag = text[1];
    return OK;
}

int read_double(const char *text, double *result)
{
    char *end;
    double value;
    int i;

    if (text == NULL || result == NULL) return INVALID_ARGUMENT;
    if (text[0] == '\0') return INVALID_NUMBER;

    for (i = 0; text[i] != '\0'; i++) {
        if ((text[i] < '0' || text[i] > '9') && text[i] != '.' &&
            text[i] != '+' && text[i] != '-' &&
            text[i] != 'e' && text[i] != 'E') return INVALID_NUMBER;
    }

    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value))
        return INVALID_NUMBER;

    *result = value;
    return OK;
}

int read_integer(const char *text, long long *result)
{
    char *end;
    long long value;
    int i = 0;

    if (text == NULL || result == NULL) return INVALID_ARGUMENT;
    if (text[0] == '-' || text[0] == '+') i++;
    if (text[i] == '\0') return INVALID_NUMBER;

    for (; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') return INVALID_NUMBER;
    }

    errno = 0;
    value = strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') return INVALID_NUMBER;

    *result = value;
    return OK;
}

int make_permutations(const double numbers[3], const double eps,
                      double permutations[6][3], int *count)
{
    int i;
    int j;
    int k;
    int p;
    int duplicate;
    int size = 0;

    if (numbers == NULL || permutations == NULL || count == NULL)
        return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;

    for (i = 0; i < 3; i++) {
        if (!isfinite(numbers[i])) return INVALID_NUMBER;
    }

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 3; k++) {
                if (i == j || i == k || j == k) continue;

                duplicate = 0;
                for (p = 0; p < size; p++) {
                    if (fabs(numbers[i] - permutations[p][0]) <= eps &&
                        fabs(numbers[j] - permutations[p][1]) <= eps &&
                        fabs(numbers[k] - permutations[p][2]) <= eps) {
                        duplicate = 1;
                        break;
                    }
                }

                if (duplicate == 0) {
                    permutations[size][0] = numbers[i];
                    permutations[size][1] = numbers[j];
                    permutations[size][2] = numbers[k];
                    size++;
                }
            }
        }
    }

    *count = size;
    return OK;
}

int solve_equation(const double a, const double b, const double c,
                   const double eps, double roots[2], int *kind)
{
    double d;
    double b_squared;
    double ac;
    double q;
    double first;
    double second;

    if (roots == NULL || kind == NULL) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (!isfinite(a) || !isfinite(b) || !isfinite(c)) return INVALID_NUMBER;

    if (fabs(a) <= eps) {
        if (fabs(b) <= eps) {
            if (fabs(c) <= eps) *kind = INFINITE_ROOTS;
            else *kind = NO_ROOTS;
        } else {
            first = -c / b;
            if (!isfinite(first)) return CALCULATION_ERROR;
            roots[0] = first;
            *kind = ONE_ROOT;
        }
        return OK;
    }

    b_squared = b * b;
    ac = a * c;
    if (!isfinite(b_squared) || !isfinite(ac)) return CALCULATION_ERROR;

    ac *= 4.0;
    if (!isfinite(ac)) return CALCULATION_ERROR;

    d = b_squared - ac;
    if (!isfinite(d)) return CALCULATION_ERROR;

    if (d < -eps) {
        *kind = NO_ROOTS;
    } else if (fabs(d) <= eps) {
        first = (-b / a) / 2.0;
        if (!isfinite(first)) return CALCULATION_ERROR;
        roots[0] = first;
        *kind = ONE_ROOT;
    } else {
        if (signbit(b)) q = -b / 2.0 + sqrt(d) / 2.0;
        else q = -b / 2.0 - sqrt(d) / 2.0;

        first = q / a;
        second = c / q;
        if (!isfinite(first) || !isfinite(second)) return CALCULATION_ERROR;

        roots[0] = first;
        roots[1] = second;
        *kind = TWO_ROOTS;
    }
    return OK;
}

int check_multiple(const long long first, const long long second, int *result)
{
    if (result == NULL) return INVALID_ARGUMENT;
    if (first == 0 || second == 0) return ZERO_INTEGER;

    if (second == -1) {
        *result = 1;
    } else if (first % second == 0) {
        *result = 1;
    } else {
        *result = 0;
    }
    return OK;
}

int check_triangle(const double a, const double b, const double c,
                   const double eps, int *result)
{
    double first;
    double second;
    double third;
    double sum1;
    double sum2;
    double sum3;

    if (result == NULL) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (!isfinite(a) || !isfinite(b) || !isfinite(c)) return INVALID_NUMBER;
    if (a <= 0.0 || b <= 0.0 || c <= 0.0) return INVALID_SIDE;

    *result = 0;
    if (a <= eps || b <= eps || c <= eps) return OK;

    if (a <= c - b + eps || b <= a - c + eps || c <= b - a + eps)
        return OK;

    first = a * a;
    second = b * b;
    third = c * c;
    if (!isfinite(first) || !isfinite(second) || !isfinite(third))
        return CALCULATION_ERROR;

    sum1 = first + second;
    sum2 = first + third;
    sum3 = second + third;
    if (!isfinite(sum1) || !isfinite(sum2) || !isfinite(sum3))
        return CALCULATION_ERROR;

    if (fabs(sum1 - third) <= eps || fabs(sum2 - second) <= eps ||
        fabs(sum3 - first) <= eps) *result = 1;
    return OK;
}

int print_equation(const double coefficients[3], const double roots[2],
                   const int kind)
{
    int i;
    int written;

    if (coefficients == NULL || roots == NULL) return INVALID_ARGUMENT;
    if (kind < NO_ROOTS || kind > INFINITE_ROOTS) return INVALID_ARGUMENT;
    for (i = 0; i < 3; i++) {
        if (!isfinite(coefficients[i])) return INVALID_NUMBER;
    }
    if (kind == ONE_ROOT && !isfinite(roots[0])) return INVALID_NUMBER;
    if (kind == TWO_ROOTS && (!isfinite(roots[0]) || !isfinite(roots[1])))
        return INVALID_NUMBER;

    written = printf("a = %.17g, b = %.17g, c = %.17g: ",
                     coefficients[0], coefficients[1], coefficients[2]);
    if (written < 0) return OUTPUT_ERROR;

    if (kind == NO_ROOTS) written = printf("действительных корней нет\n");
    else if (kind == INFINITE_ROOTS) written = printf("любое действительное число\n");
    else if (kind == ONE_ROOT) written = printf("x = %.17g\n", roots[0]);
    else written = printf("x1 = %.17g, x2 = %.17g\n", roots[0], roots[1]);

    if (written < 0) return OUTPUT_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    char flag = '\0';
    double eps = 0.0;
    double numbers[3] = {0.0};
    double permutations[6][3];
    double roots[6][2] = {{0.0}};
    int kinds[6];
    long long first = 0;
    long long second = 0;
    int count = 0;
    int answer = 0;
    int status = OK;
    int i;

    if (argc < 2) status = INVALID_COUNT;
    else status = read_flag(argv[1], &flag);

    if (status == OK) {
        if (flag == 'm' && argc != 4) status = INVALID_COUNT;
        if ((flag == 'q' || flag == 't') && argc != 6) status = INVALID_COUNT;
    }

    if (status == OK && flag == 'm') {
        status = read_integer(argv[2], &first);
        if (status == OK) status = read_integer(argv[3], &second);
        if (status == OK) status = check_multiple(first, second, &answer);

        if (status == OK) {
            if (answer == 1) {
                if (printf("Первое число кратно второму.\n") < 0) status = OUTPUT_ERROR;
            } else {
                if (printf("Первое число не кратно второму.\n") < 0) status = OUTPUT_ERROR;
            }
        }
    } else if (status == OK) {
        status = read_double(argv[2], &eps);
        if (status == OK && eps <= 0.0) status = INVALID_EPSILON;

        for (i = 0; i < 3 && status == OK; i++) {
            status = read_double(argv[i + 3], &numbers[i]);
        }

        if (status == OK && flag == 'q') {
            status = make_permutations(numbers, eps, permutations, &count);
            for (i = 0; i < count && status == OK; i++) {
                status = solve_equation(permutations[i][0], permutations[i][1],
                                        permutations[i][2], eps, roots[i], &kinds[i]);
            }
            for (i = 0; i < count && status == OK; i++) {
                status = print_equation(permutations[i], roots[i], kinds[i]);
            }
        } else if (status == OK && flag == 't') {
            status = check_triangle(numbers[0], numbers[1], numbers[2], eps, &answer);
            if (status == OK) {
                if (answer == 1) {
                    if (printf("Это стороны прямоугольного треугольника.\n") < 0)
                        status = OUTPUT_ERROR;
                } else {
                    if (printf("Это не стороны прямоугольного треугольника.\n") < 0)
                        status = OUTPUT_ERROR;
                }
            }
        }
    }

    if (fflush(stdout) == EOF) status = OUTPUT_ERROR;

    switch (status) {
        case OK:
            return 0;
        case INVALID_ARGUMENT:
            fprintf(stderr, "Ошибка параметров функции.\n");
            break;
        case INVALID_FLAG:
            fprintf(stderr, "Нужен флаг -q, -m, -t, /q, /m или /t.\n");
            break;
        case INVALID_COUNT:
            fprintf(stderr, "Использование:\n");
            fprintf(stderr, "  %s -q epsilon a b c\n", argv[0]);
            fprintf(stderr, "  %s -m first second\n", argv[0]);
            fprintf(stderr, "  %s -t epsilon a b c\n", argv[0]);
            break;
        case INVALID_NUMBER:
            fprintf(stderr, "Неверная запись числа или число вне диапазона типа.\n");
            break;
        case INVALID_EPSILON:
            fprintf(stderr, "Эпсилон должен быть положительным конечным числом.\n");
            break;
        case ZERO_INTEGER:
            fprintf(stderr, "Для -m оба числа должны быть ненулевыми.\n");
            break;
        case INVALID_SIDE:
            fprintf(stderr, "Длины сторон должны быть положительными.\n");
            break;
        case CALCULATION_ERROR:
            fprintf(stderr, "Результат вычисления не помещается в double.\n");
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