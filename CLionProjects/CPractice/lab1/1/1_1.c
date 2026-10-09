#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
    STATUS_INVALID_FLAG,
    STATUS_DOMAIN_ERROR,
    STATUS_OUTPUT_ERROR
} Status;

typedef enum { PRIME, COMPOSITE, NONE } NumberKind;

static Status parse_number(const char *const text, uint64_t *const result)
{
    if (text == NULL || result == NULL) return STATUS_INVALID_ARGUMENT;
    if (*text == '\0') return STATUS_INVALID_NUMBER;

    uint64_t value = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return STATUS_INVALID_NUMBER;
        const unsigned digit = (unsigned)(*p - '0');
        if (value > (UINT64_MAX - digit) / 10) return STATUS_OVERFLOW;
        value = value * 10 + digit;
    }
    *result = value;
    return STATUS_OK;
}

static Status parse_flag(const char *const text, char *const flag)
{
    if (text == NULL || flag == NULL) return STATUS_INVALID_ARGUMENT;
    if (strlen(text) != 2 || (text[0] != '-' && text[0] != '/') ||
        strchr("hpseaf", text[1]) == NULL) return STATUS_INVALID_FLAG;
    *flag = text[1];
    return STATUS_OK;
}

static Status find_multiples(const uint64_t x, uint64_t *const values,
                            const size_t capacity, size_t *const count)
{
    if (values == NULL || count == NULL) return STATUS_INVALID_ARGUMENT;
    const size_t needed = (x == 0 || x > 100) ? 0 : (size_t)(100 / x);
    if (capacity < needed) return STATUS_INVALID_ARGUMENT;
    for (size_t i = 0; i < needed; ++i) values[i] = (i + 1) * x;
    *count = needed;
    return STATUS_OK;
}

static Status classify_number(const uint64_t x, NumberKind *const kind)
{
    if (kind == NULL) return STATUS_INVALID_ARGUMENT;
    if (x < 2) {
        *kind = NONE;
        return STATUS_OK;
    }
    if (x % 2 == 0) {
        *kind = x == 2 ? PRIME : COMPOSITE;
        return STATUS_OK;
    }
    for (uint64_t divisor = 3; divisor <= x / divisor; divisor += 2) {
        if (x % divisor == 0) {
            *kind = COMPOSITE;
            return STATUS_OK;
        }
    }
    *kind = PRIME;
    return STATUS_OK;
}

static Status hex_digits(const uint64_t x, char *const digits,
                         const size_t capacity, size_t *const count)
{
    if (digits == NULL || count == NULL) return STATUS_INVALID_ARGUMENT;
    const char alphabet[] = "0123456789ABCDEF";
    char reversed[16];
    size_t length = 0;
    uint64_t remaining = x;
    do {
        reversed[length++] = alphabet[remaining % 16];
        remaining /= 16;
    } while (remaining != 0);
    if (capacity < length) return STATUS_INVALID_ARGUMENT;
    for (size_t i = 0; i < length; ++i) digits[i] = reversed[length - i - 1];
    *count = length;
    return STATUS_OK;
}

static Status make_powers(const uint64_t x, uint64_t (*const table)[10])
{
    if (table == NULL) return STATUS_INVALID_ARGUMENT;
    if (x == 0 || x > 10) return STATUS_DOMAIN_ERROR;
    for (size_t row = 0; row < 10; ++row) {
        uint64_t value = 1;
        const uint64_t base = row + 1;
        for (size_t col = 0; col < (size_t)x; ++col) {
            if (value > UINT64_MAX / base) return STATUS_OVERFLOW;
            value *= base;
            table[row][col] = value;
        }
    }
    return STATUS_OK;
}

static Status natural_sum(const uint64_t x, uint64_t *const result)
{
    if (result == NULL) return STATUS_INVALID_ARGUMENT;
    if (x == 0) return STATUS_DOMAIN_ERROR;
    if (x == UINT64_MAX) return STATUS_OVERFLOW;
    uint64_t a = x, b = x + 1;
    if (a % 2 == 0) a /= 2;
    else b /= 2;
    if (a > UINT64_MAX / b) return STATUS_OVERFLOW;
    *result = a * b;
    return STATUS_OK;
}

static Status factorial(const uint64_t x, uint64_t *const result)
{
    if (result == NULL) return STATUS_INVALID_ARGUMENT;
    uint64_t value = 1;
    for (uint64_t i = 2; i <= x; ++i) {
        if (value > UINT64_MAX / i) return STATUS_OVERFLOW;
        value *= i;
    }
    *result = value;
    return STATUS_OK;
}

static Status print_values(const uint64_t *const values, const size_t count)
{
    if (values == NULL || count > 100) return STATUS_INVALID_ARGUMENT;
    if (count == 0) {
        return puts("Таких натуральных чисел нет.") == EOF
                   ? STATUS_OUTPUT_ERROR : STATUS_OK;
    }
    for (size_t i = 0; i < count; ++i) {
        if (printf("%" PRIu64 "%c", values[i], i + 1 == count ? '\n' : ' ') < 0)
            return STATUS_OUTPUT_ERROR;
    }
    return STATUS_OK;
}

static Status print_digits(const char *const digits, const size_t count)
{
    if (digits == NULL || count == 0 || count > 16) return STATUS_INVALID_ARGUMENT;
    for (size_t i = 0; i < count; ++i) {
        if (strchr("0123456789ABCDEF", digits[i]) == NULL || digits[i] == '\0')
            return STATUS_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < count; ++i) {
        if (printf("%c%c", digits[i], i + 1 == count ? '\n' : ' ') < 0)
            return STATUS_OUTPUT_ERROR;
    }
    return STATUS_OK;
}

int main(const int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <x> <-h|-p|-s|-e|-a|-f>\n", argv[0]);
        return 1;
    }

    uint64_t x = 0;
    char flag = '\0';
    Status status = parse_number(argv[1], &x);
    if (status == STATUS_OK) status = parse_flag(argv[2], &flag);

    if (status == STATUS_OK) {
        switch (flag) {
            case 'h': {
                uint64_t values[100];
                size_t count = 0;
                status = find_multiples(x, values, 100, &count);
                if (status == STATUS_OK) status = print_values(values, count);
                break;
            }
            case 'p': {
                NumberKind kind;
                status = classify_number(x, &kind);
                if (status == STATUS_OK) {
                    const char *const message = kind == PRIME ? "Число простое."
                        : kind == COMPOSITE ? "Число составное."
                        : "Число не является ни простым, ни составным.";
                    if (puts(message) == EOF) status = STATUS_OUTPUT_ERROR;
                }
                break;
            }
            case 's': {
                char digits[16];
                size_t count = 0;
                status = hex_digits(x, digits, sizeof digits, &count);
                if (status == STATUS_OK) status = print_digits(digits, count);
                break;
            }
            case 'e': {
                uint64_t table[10][10];
                status = make_powers(x, table);
                if (status != STATUS_OK) break;
                if (printf("%12s", "base/exp") < 0) status = STATUS_OUTPUT_ERROR;
                for (size_t col = 0; col < (size_t)x && status == STATUS_OK; ++col)
                    if (printf("%12zu", col + 1) < 0) status = STATUS_OUTPUT_ERROR;
                if (status == STATUS_OK && putchar('\n') == EOF) status = STATUS_OUTPUT_ERROR;
                for (size_t row = 0; row < 10 && status == STATUS_OK; ++row) {
                    if (printf("%12zu", row + 1) < 0) status = STATUS_OUTPUT_ERROR;
                    for (size_t col = 0; col < (size_t)x && status == STATUS_OK; ++col)
                        if (printf("%12" PRIu64, table[row][col]) < 0) status = STATUS_OUTPUT_ERROR;
                    if (status == STATUS_OK && putchar('\n') == EOF) status = STATUS_OUTPUT_ERROR;
                }
                break;
            }
            case 'a':
            case 'f': {
                uint64_t result = 0;
                status = flag == 'a' ? natural_sum(x, &result) : factorial(x, &result);
                if (status == STATUS_OK && printf("%" PRIu64 "\n", result) < 0)
                    status = STATUS_OUTPUT_ERROR;
                break;
            }
            default: status = STATUS_INVALID_FLAG; break;
        }
    }
    if (fflush(stdout) == EOF) status = STATUS_OUTPUT_ERROR;
    switch (status) {
        case STATUS_OK: return 0;
        case STATUS_INVALID_ARGUMENT: fprintf(stderr, "Ошибка параметров функции.\n"); break;
        case STATUS_INVALID_NUMBER: fprintf(stderr, "x должен состоять из десятичных цифр.\n"); break;
        case STATUS_OVERFLOW: fprintf(stderr, "Число или результат не помещается в uint64_t.\n"); break;
        case STATUS_INVALID_FLAG: fprintf(stderr, "Неизвестный флаг: нужны -h, -p, -s, -e, -a, -f (или /).\n"); break;
        case STATUS_DOMAIN_ERROR: fprintf(stderr, "Для -e требуется 1 <= x <= 10; для -a требуется x >= 1.\n"); break;
        case STATUS_OUTPUT_ERROR: fprintf(stderr, "Ошибка вывода.\n"); break;
        default: fprintf(stderr, "Неизвестная ошибка.\n"); break;
    }
    return 1;
}