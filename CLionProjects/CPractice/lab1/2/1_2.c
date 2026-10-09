#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <float.h>

enum Status {
    OK,
    INVALID_ARGUMENT,
    INVALID_EPSILON,
    PRECISION_ERROR,
    ITERATION_LIMIT,
    OUTPUT_ERROR
};

static int check_arguments(const double eps, const double *result)
{
    if (result == NULL) return INVALID_ARGUMENT;
    if (!isfinite(eps) || eps <= 0.0) return INVALID_EPSILON;
    if (eps < 1e-12) return PRECISION_ERROR;
    return OK;
}

static int read_epsilon(const char *text, double *eps)
{
    char *end;
    double value;

    if (text == NULL || eps == NULL) return INVALID_ARGUMENT;
    if (text[0] == '\0') return INVALID_EPSILON;

    for (int i = 0; text[i] != '\0'; i++) {
        if ((text[i] < '0' || text[i] > '9') && text[i] != '.' &&
            text[i] != '+' && text[i] != '-' &&
            text[i] != 'e' && text[i] != 'E') return INVALID_EPSILON;
    }

    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' ||
        !isfinite(value) || value <= 0.0) return INVALID_EPSILON;

    *eps = value;
    return OK;
}

static int e_limit(const double eps, double *result)
{
    double n = 1.0;
    double value;
    const int status = check_arguments(eps, result);
    if (status != OK) return status;

    while (3.0 / n > eps / 2.0) {
        n *= 2.0;
        if (32.0 * n * DBL_EPSILON > eps / 2.0) return PRECISION_ERROR;
    }

    value = pow(1.0 + 1.0 / n, n);
    if (!isfinite(value)) return PRECISION_ERROR;
    *result = value;
    return OK;
}

static int e_series(const double eps, double *result)
{
    double sum = 1.0;
    double term = 1.0;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    for (n = 1; n <= 1000; n++) {
        term /= n;
        sum += term;
        if (2.0 * term / (n + 1) <= eps / 2.0) {
            *result = sum;
            return OK;
        }
    }
    return ITERATION_LIMIT;
}

static int e_equation(const double eps, double *result)
{
    double left = 2.0;
    double right = 3.0;
    double middle;
    double value;
    const int status = check_arguments(eps, result);
    if (status != OK) return status;

    while (right - left > eps / 2.0) {
        middle = (left + right) / 2.0;
        value = log(middle) - 1.0;
        if (fabs(value) <= eps / 8.0) {
            *result = middle;
            return OK;
        }
        if (value < -eps / 8.0) left = middle;
        else right = middle;
    }
    *result = (left + right) / 2.0;
    return OK;
}

static int pi_limit(const double eps, double *result)
{
    double value = 4.0;
    double denominator;
    int n;
    const int status = check_arguments(eps, result);
    if (status != OK) return status;
    if (4.0 / 10000000.0 > eps / 2.0) return ITERATION_LIMIT;

    for (n = 1; n <= 10000000; n++) {
        if (4.0 / n <= eps / 2.0) {
            *result = value;
            return OK;
        }
        denominator = 2.0 * n + 1.0;
        value *= 4.0 * n * (n + 1.0) / (denominator * denominator);
    }
    return ITERATION_LIMIT;
}

static int pi_series(const double eps, double *result)
{
    double sum = 0.0;
    int sign = 1;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;
    if (4.0 / 20000001.0 > eps / 2.0) return ITERATION_LIMIT;

    for (n = 1; n <= 10000000; n++) {
        sum += sign * 4.0 / (2.0 * n - 1.0);
        sign = -sign;
        if (4.0 / (2.0 * n + 1.0) <= eps / 2.0) {
            *result = sum;
            return OK;
        }
    }
    return ITERATION_LIMIT;
}

static int pi_equation(const double eps, double *result)
{
    double x = 3.0;
    double value;
    double derivative;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    if (2.0 * sqrt(8.0 * DBL_EPSILON) > eps / 2.0) return PRECISION_ERROR;

    for (n = 0; n < 1000; n++) {
        value = cos(x) + 1.0;
        if (2.0 * sqrt(fabs(value) + 8.0 * DBL_EPSILON) <= eps / 2.0) {
            *result = x;
            return OK;
        }
        derivative = -sin(x);
        if (fabs(derivative) <= eps / 16.0) return PRECISION_ERROR;
        x -= value / derivative;
        if (!isfinite(x)) return PRECISION_ERROR;
    }
    return ITERATION_LIMIT;
}

static int ln2_limit(const double eps, double *result)
{
    double n = 1.0;
    double value;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    while (1.0 / n > eps / 2.0) {
        n *= 2.0;
        if (16.0 * n * DBL_EPSILON > eps / 2.0) return PRECISION_ERROR;
    }
    value = n * (pow(2.0, 1.0 / n) - 1.0);
    if (!isfinite(value)) return PRECISION_ERROR;
    *result = value;
    return OK;
}

static int ln2_series(const double eps, double *result)
{
    double sum = 0.0;
    int sign = 1;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;
    if (1.0 / 10000001.0 > eps / 2.0) return ITERATION_LIMIT;

    for (n = 1; n <= 10000000; n++) {
        sum += (double)sign / n;
        sign = -sign;
        if (1.0 / (n + 1.0) <= eps / 2.0) {
            *result = sum;
            return OK;
        }
    }
    return ITERATION_LIMIT;
}

static int ln2_equation(const double eps, double *result)
{
    double left = 0.0;
    double right = 1.0;
    double middle;
    double value;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    while (right - left > eps / 2.0) {
        middle = (left + right) / 2.0;
        value = exp(middle) - 2.0;
        if (fabs(value) <= eps / 8.0) {
            *result = middle;
            return OK;
        }
        if (value < -eps / 8.0) left = middle;
        else right = middle;
    }
    *result = (left + right) / 2.0;
    return OK;
}

static int sqrt2_limit(const double eps, double *result)
{
    double x = -0.5;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    for (n = 0; n < 1000; n++) {
        x = x - x * x / 2.0 + 1.0;
        if (fabs(x * x - 2.0) <= eps / 2.0) {
            *result = x;
            return OK;
        }
    }
    return ITERATION_LIMIT;
}

static int sqrt2_product(const double eps, double *result)
{
    double product = 1.0;
    double power = 0.25;
    int n;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    for (n = 0; n < 1000; n++) {
        product *= pow(2.0, power);
        if (2.0 * power <= eps / 2.0) {
            *result = product;
            return OK;
        }
        power /= 2.0;
    }
    return ITERATION_LIMIT;
}

static int sqrt2_equation(const double eps, double *result)
{
    double left = 1.0;
    double right = 2.0;
    double middle;
    double value;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    while (right - left > eps / 2.0) {
        middle = (left + right) / 2.0;
        value = middle * middle - 2.0;
        if (fabs(value) <= eps / 8.0) {
            *result = middle;
            return OK;
        }
        if (value < -eps / 8.0) left = middle;
        else right = middle;
    }
    *result = (left + right) / 2.0;
    return OK;
}

static int gamma_series(const double eps, double *result)
{
    double pi;
    double sum;
    int root = 1;
    int k;
    int status = check_arguments(eps, result);
    if (status != OK) return status;
    if (3.0 / 3161.0 > eps / 2.0) return ITERATION_LIMIT;

    status = pi_series(eps / 16.0, &pi);
    if (status != OK) return status;
    sum = -pi * pi / 6.0;

    for (k = 2; k <= 10000000; k++) {
        if (k == (root + 1) * (root + 1)) root++;
        sum += 1.0 / (root * root) - 1.0 / k;

        if (k == (root + 1) * (root + 1) - 1 && 3.0 / root <= eps / 2.0) {
            *result = sum;
            return OK;
        }
    }
    return ITERATION_LIMIT;
}

static int gamma_limit(const double eps, double *result)
{
    double reference;
    double sum;
    double combination;
    double log_factorial;
    int sign;
    int m;
    int k;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    status = gamma_series(eps / 4.0, &reference);
    if (status != OK) return status;

    for (m = 1; m <= 40; m++) {
        sum = 0.0;
        combination = 1.0;
        log_factorial = 0.0;
        sign = -1;

        for (k = 1; k <= m; k++) {
            combination *= (double)(m - k + 1) / k;
            log_factorial += log(k);
            sum += combination * sign * log_factorial / k;
            sign = -sign;
        }

        if (fabs(sum - reference) <= eps / 2.0) {
            *result = sum;
            return OK;
        }
    }
    return PRECISION_ERROR;
}

static int is_prime(const int number, int *result)
{
    int divisor;
    if (result == NULL || number < 0) return INVALID_ARGUMENT;

    *result = 0;
    if (number < 2) return OK;

    for (divisor = 2; divisor <= number / divisor; divisor++) {
        if (number % divisor == 0) return OK;
    }

    *result = 1;
    return OK;
}

static int gamma_equation(const double eps, double *result)
{
    double reference;
    double product = 1.0;
    double target;
    double left;
    double right;
    double middle;
    double value;
    double answer;
    int prime;
    int t;
    int checkpoint = 16;
    int status = check_arguments(eps, result);
    if (status != OK) return status;

    status = gamma_series(eps / 4.0, &reference);
    if (status != OK) return status;

    for (t = 2; t <= 1000000; t++) {
        status = is_prime(t, &prime);
        if (status != OK) return status;
        if (prime == 1) product *= (t - 1.0) / t;

        if (t == checkpoint) {
            target = log(t) * product;
            left = 0.0;
            right = 1.0;

            while (right - left > eps / 8.0) {
                middle = (left + right) / 2.0;
                value = exp(-middle) - target;
                if (fabs(value) <= eps / 64.0) {
                    left = middle;
                    right = middle;
                } else if (value > eps / 64.0) {
                    left = middle;
                } else {
                    right = middle;
                }
            }

            answer = (left + right) / 2.0;
            if (fabs(answer - reference) <= eps / 2.0) {
                *result = answer;
                return OK;
            }
            checkpoint *= 2;
        }
    }
    return ITERATION_LIMIT;
}

static int print_result(const int status, const double value)
{
    int written;
    if (status == OK) {
        if (!isfinite(value)) return INVALID_ARGUMENT;
        written = printf("%.12f\n", value);
    } else if (status == INVALID_ARGUMENT) {
        written = printf("ошибка параметров\n");
    } else if (status == INVALID_EPSILON) {
        written = printf("неверная точность\n");
    } else if (status == PRECISION_ERROR) {
        written = printf("не удалось достичь точности при вычислении в double\n");
    } else if (status == ITERATION_LIMIT) {
        written = printf("для этой точности превышен предел числа итераций\n");
    } else {
        return INVALID_ARGUMENT;
    }

    if (written < 0) return OUTPUT_ERROR;
    return OK;
}

int main(const int argc, char *argv[])
{
    double eps;
    double values[5][3] = {{0.0}};
    int statuses[5][3];
    const char *names[5] = {"e", "pi", "ln 2", "sqrt 2", "gamma"};
    const char *methods[3] = {"предел", "ряд/произведение", "уравнение"};
    int status;
    int failed = 0;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s <epsilon>\n", argv[0]);
        return 1;
    }

    status = read_epsilon(argv[1], &eps);
    if (status != OK) {
        fprintf(stderr, "Точность должна быть положительным конечным числом.\n");
        return 1;
    }

    statuses[0][0] = e_limit(eps, &values[0][0]);
    statuses[0][1] = e_series(eps, &values[0][1]);
    statuses[0][2] = e_equation(eps, &values[0][2]);
    statuses[1][0] = pi_limit(eps, &values[1][0]);
    statuses[1][1] = pi_series(eps, &values[1][1]);
    statuses[1][2] = pi_equation(eps, &values[1][2]);
    statuses[2][0] = ln2_limit(eps, &values[2][0]);
    statuses[2][1] = ln2_series(eps, &values[2][1]);
    statuses[2][2] = ln2_equation(eps, &values[2][2]);
    statuses[3][0] = sqrt2_limit(eps, &values[3][0]);
    statuses[3][1] = sqrt2_product(eps, &values[3][1]);
    statuses[3][2] = sqrt2_equation(eps, &values[3][2]);
    statuses[4][0] = gamma_limit(eps, &values[4][0]);
    statuses[4][1] = gamma_series(eps, &values[4][1]);
    statuses[4][2] = gamma_equation(eps, &values[4][2]);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            if (printf("%s, %s: ", names[i], methods[j]) < 0) return 1;
            status = print_result(statuses[i][j], values[i][j]);
            if (status != OK) return 1;
            if (statuses[i][j] != OK) failed = 1;
        }
    }

    if (fflush(stdout) == EOF) return 1;
    return failed;
}