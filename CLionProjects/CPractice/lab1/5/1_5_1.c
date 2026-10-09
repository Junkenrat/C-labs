#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <errno.h>

enum Status {
    OK,
    NOT_READY,
    INVALID_ARGUMENT,
    INVALID_NUMBER,
    INVALID_EPSILON,
    DIVERGES,
    CALCULATION_ERROR,
    PRECISION_ERROR,
    ITERATION_LIMIT,
    OUTPUT_ERROR
};

int read_number(const char *text, double *result)
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

int check_arguments(const double eps, const double x, const double *result)
{
    if (result == NULL) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (!isfinite(x)) return INVALID_NUMBER;
    return OK;
}

int check_accuracy(const double sum, const double absolute_sum,
                   const double remainder, const int n,
                   const double eps, double *result)
{
    double rounding;

    if (result == NULL || n < 1) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (!isfinite(sum) || !isfinite(absolute_sum) || !isfinite(remainder))
        return CALCULATION_ERROR;
    if (absolute_sum < 0.0 || remainder < 0.0) return INVALID_ARGUMENT;

    rounding = (8.0 * n + 8.0) * DBL_EPSILON * absolute_sum;
    if (!isfinite(rounding)) return CALCULATION_ERROR;
    if (rounding > eps / 2.0) return PRECISION_ERROR;

    if (remainder <= eps / 2.0) {
        *result = sum;
        return OK;
    }
    return NOT_READY;
}

int sum_a(const double eps, const double x, double *result)
{
    double sum = 1.0;
    double term = 1.0;
    double absolute_sum = 1.0;
    double ratio;
    double remainder;
    int n;
    int status = check_arguments(eps, x, result);
    if (status != OK) return status;

    for (n = 1; n <= 10000000; n++) {
        term *= x / n;
        sum += term;
        absolute_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(absolute_sum)) return CALCULATION_ERROR;

        ratio = fabs(x) / (n + 1.0);
        if (ratio < 1.0) {
            remainder = fabs(term) * ratio / (1.0 - ratio);
            status = check_accuracy(sum, absolute_sum, remainder, n, eps, result);
            if (status != NOT_READY) return status;
        }
    }
    return ITERATION_LIMIT;
}

int sum_b(const double eps, const double x, double *result)
{
    double sum = 1.0;
    double term = 1.0;
    double absolute_sum = 1.0;
    double square;
    double ratio;
    double remainder;
    int n;
    int status = check_arguments(eps, x, result);
    if (status != OK) return status;

    square = x * x;
    if (!isfinite(square)) return CALCULATION_ERROR;

    for (n = 1; n <= 10000000; n++) {
        term *= -square / ((2.0 * n - 1.0) * (2.0 * n));
        sum += term;
        absolute_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(absolute_sum)) return CALCULATION_ERROR;

        ratio = square / ((2.0 * n + 1.0) * (2.0 * n + 2.0));
        if (ratio <= 1.0) {
            remainder = fabs(term) * ratio;
            status = check_accuracy(sum, absolute_sum, remainder, n, eps, result);
            if (status != NOT_READY) return status;
        }
    }
    return ITERATION_LIMIT;
}

int sum_c(const double eps, const double x, double *result)
{
    double sum = 1.0;
    double term = 1.0;
    double absolute_sum = 1.0;
    double square;
    double ratio;
    double remainder;
    double next;
    int n;
    int status = check_arguments(eps, x, result);
    if (status != OK) return status;
    if (fabs(x) >= 1.0) return DIVERGES;

    square = x * x;
    for (n = 1; n <= 10000000; n++) {
        ratio = 9.0 * n * n * square / ((3.0 * n - 1.0) * (3.0 * n - 2.0));
        term *= ratio;
        sum += term;
        absolute_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(absolute_sum)) return CALCULATION_ERROR;

        next = n + 1.0;
        ratio = 9.0 * next * next * square / ((3.0 * next - 1.0) * (3.0 * next - 2.0));
        if (ratio < 1.0) {
            remainder = fabs(term) * ratio / (1.0 - ratio);
            status = check_accuracy(sum, absolute_sum, remainder, n, eps, result);
            if (status != NOT_READY) return status;
        }
    }
    return ITERATION_LIMIT;
}

int sum_d(const double eps, const double x, double *result)
{
    double sum = 0.0;
    double term = 1.0;
    double absolute_sum = 0.0;
    double square;
    double remainder;
    int n;
    int status = check_arguments(eps, x, result);
    if (status != OK) return status;
    if (fabs(x) > 1.0) return DIVERGES;

    square = x * x;
    for (n = 1; n <= 10000000; n++) {
        term *= -square * (2.0 * n - 1.0) / (2.0 * n);
        sum += term;
        absolute_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(absolute_sum)) return CALCULATION_ERROR;

        remainder = fabs(term) * square * (2.0 * n + 1.0) / (2.0 * n + 2.0);
        status = check_accuracy(sum, absolute_sum, remainder, n, eps, result);
        if (status != NOT_READY) return status;
    }
    return ITERATION_LIMIT;
}

int print_result(const char name, const int status, const double value)
{
    int written;

    if (name < 'a' || name > 'd') return INVALID_ARGUMENT;

    if (status == OK) {
        if (!isfinite(value)) return INVALID_NUMBER;
        written = printf("%c: %.17g\n", name, value);
    } else if (status == DIVERGES) {
        written = printf("%c: ряд расходится при этом x\n", name);
    } else if (status == CALCULATION_ERROR) {
        written = printf("%c: переполнение при вычислении\n", name);
    } else if (status == PRECISION_ERROR) {
        written = printf("%c: недостаточно точности типа double\n", name);
    } else if (status == ITERATION_LIMIT) {
        written = printf("%c: превышено допустимое число итераций\n", name);
    } else if (status == INVALID_EPSILON) {
        written = printf("%c: неверное значение epsilon\n", name);
    } else if (status == INVALID_NUMBER) {
        written = printf("%c: неверное значение x\n", name);
    } else {
        return INVALID_ARGUMENT;
    }

    if (written < 0) return OUTPUT_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    double eps;
    double x;
    double results[4] = {0.0};
    int statuses[4];
    const char names[4] = {'a', 'b', 'c', 'd'};
    int status;
    int failed = 0;
    int i;

    if (argc != 3) {
        fprintf(stderr, "Использование: %s epsilon x\n", argv[0]);
        return 1;
    }

    status = read_number(argv[1], &eps);
    if (status != OK || eps <= 0.0) {
        fprintf(stderr, "Epsilon должен быть положительным конечным числом.\n");
        return 1;
    }

    status = read_number(argv[2], &x);
    if (status != OK) {
        fprintf(stderr, "x должен быть конечным вещественным числом.\n");
        return 1;
    }

    statuses[0] = sum_a(eps, x, &results[0]);
    statuses[1] = sum_b(eps, x, &results[1]);
    statuses[2] = sum_c(eps, x, &results[2]);
    statuses[3] = sum_d(eps, x, &results[3]);

    for (i = 0; i < 4; i++) {
        status = print_result(names[i], statuses[i], results[i]);
        if (status != OK) {
            fprintf(stderr, "Ошибка вывода результата.\n");
            return 1;
        }
        if (statuses[i] != OK) failed = 1;
    }

    if (fflush(stdout) == EOF) {
        fprintf(stderr, "Ошибка вывода результата.\n");
        return 1;
    }
    return failed;
}