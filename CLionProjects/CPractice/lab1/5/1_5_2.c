#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <errno.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_EPSILON,
    CALCULATION_ERROR,
    PRECISION_ERROR,
    ITERATION_LIMIT,
    OUTPUT_ERROR
};

int read_epsilon(const char *text, double *result)
{
    char *end;
    double value;
    int i;

    if (text == NULL || result == NULL) return INVALID_ARGUMENT;
    if (text[0] == '\0') return INVALID_EPSILON;

    for (i = 0; text[i] != '\0'; i++) {
        if ((text[i] < '0' || text[i] > '9') && text[i] != '.' &&
            text[i] != '+' && text[i] != '-' &&
            text[i] != 'e' && text[i] != 'E') return INVALID_EPSILON;
    }

    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' ||
        !isfinite(value) || value <= 0.0) return INVALID_EPSILON;

    *result = value;
    return OK;
}

int function_value(const int number, const double x,
                   const double eps, double *result)
{
    double value;

    if (result == NULL || number < 0 || number > 3) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (!isfinite(x) || x <= 0.0 || x >= 1.0) return INVALID_ARGUMENT;

    if (number == 0) value = log(1.0 + x) / x;
    else if (number == 1) value = exp(-x * x / 2.0);
    else if (number == 2) value = log(1.0 / (1.0 - x));
    else value = pow(x, x);

    if (!isfinite(value)) return CALCULATION_ERROR;
    *result = value;
    return OK;
}

int calculate_integral(const int number, const double eps, double *result)
{
    double h;
    double x;
    double value;
    double sum = 0.0;
    double error;
    double rounding;
    int n = 1;
    int i;
    int status;

    if (result == NULL || number < 0 || number > 3) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (eps < 64.0 * DBL_EPSILON) return PRECISION_ERROR;

    while (1) {
        if (number == 0 || number == 1) error = 1.0 / (24.0 * n * n);
        else error = 1.0 / n;

        if (error <= eps / 2.0) break;
        if (n > 5000000) return ITERATION_LIMIT;
        n *= 2;
    }

    h = 1.0 / n;
    for (i = 0; i < n; i++) {
        x = (i + 0.5) * h;
        status = function_value(number, x, eps, &value);
        if (status != OK) return status;

        sum += value * h;
        if (!isfinite(sum)) return CALCULATION_ERROR;
    }

    rounding = (2.0 * n + 16.0) * DBL_EPSILON * fabs(sum);
    if (!isfinite(rounding)) return CALCULATION_ERROR;
    if (rounding > eps / 2.0) return PRECISION_ERROR;

    *result = sum;
    return OK;
}

int print_result(const char name, const int status, const double value)
{
    int written;

    if (name < 'a' || name > 'd') return INVALID_ARGUMENT;

    if (status == OK) {
        if (!isfinite(value)) return INVALID_ARGUMENT;
        written = printf("%c: %.17g\n", name, value);
    } else if (status == PRECISION_ERROR) {
        written = printf("%c: недостаточно точности типа double\n", name);
    } else if (status == ITERATION_LIMIT) {
        written = printf("%c: требуется слишком много разбиений\n", name);
    } else if (status == CALCULATION_ERROR) {
        written = printf("%c: ошибка вычисления\n", name);
    } else if (status == INVALID_EPSILON) {
        written = printf("%c: неверное значение epsilon\n", name);
    } else {
        return INVALID_ARGUMENT;
    }

    if (written < 0) return OUTPUT_ERROR;
    return OK;
}

int main(int argc, char *argv[])
{
    double eps;
    double results[4] = {0.0};
    int statuses[4];
    const char names[4] = {'a', 'b', 'c', 'd'};
    int status;
    int failed = 0;
    int i;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s epsilon\n", argv[0]);
        return 1;
    }

    status = read_epsilon(argv[1], &eps);
    if (status != OK) {
        fprintf(stderr, "Epsilon должен быть положительным конечным числом.\n");
        return 1;
    }

    for (i = 0; i < 4; i++) {
        statuses[i] = calculate_integral(i, eps, &results[i]);
    }

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