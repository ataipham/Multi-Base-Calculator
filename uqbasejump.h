#ifndef UQBASEJUMP_H
#define UQBASEJUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>

/* Static variable to store original terminal settings */
static struct termios originalTermios;
static bool termiosInitialized = false;

/*
 * enable_line_buffering()
 * -----------------------
 * Restores canonical mode and echo on the terminal.
 * Typically used to undo the effects of disable_line_buffering().
 */
static inline void enable_line_buffering(void)
{
    if (termiosInitialized) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
    }
}

/*
 * disable_line_buffering()
 * ------------------------
 * Disables canonical mode and echo on the terminal to allow
 * character-by-character input. Registers enable_line_buffering()
 * to be called at exit to restore settings.
 */
static inline void disable_line_buffering(void)
{
    if (!isatty(STDIN_FILENO)) {
        return;
    }
    
    if (!termiosInitialized) {
        tcgetattr(STDIN_FILENO, &originalTermios);
        termiosInitialized = true;
        atexit(enable_line_buffering);
    }
    
    struct termios raw = originalTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

/*
 * clear_screen()
 * --------------
 * Uses ANSI escape codes to clear the screen and reset the cursor position.
 * If standard input is not from a terminal, this function will do nothing.
 */
static inline void clear_screen(void)
{
    if (!isatty(STDIN_FILENO)) {
        return;
    }
    printf("\033[2J\033[H");
    fflush(stdout);
}

/*
 * char_to_digit()
 * ---------------
 * Helper function to convert a character to its digit value.
 * Returns -1 if the character is not a valid digit.
 */
static inline int char_to_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

/*
 * digit_to_char()
 * ---------------
 * Helper function to convert a digit value to its character representation.
 * Uses uppercase letters for values 10-35.
 */
static inline char digit_to_char(int digit)
{
    if (digit >= 0 && digit <= 9) {
        return '0' + digit;
    } else if (digit >= 10 && digit <= 35) {
        return 'A' + (digit - 10);
    }
    return '?';
}

/*
 * convert_any_base_to_base_ten()
 * ------------------------------
 * Converts a string representing a number in a given base to base-10.
 *
 * input: A string representing the number (not NULL)
 * base: The base of the input number (2-36)
 *
 * Returns: A dynamically allocated string containing the base-10 representation.
 *          It is the caller's responsibility to free this buffer.
 *          Returns NULL on error.
 */
static inline char* convert_any_base_to_base_ten(const char* input, int base)
{
    if (!input || base < 2 || base > 36) {
        return NULL;
    }
    
    unsigned long long value = 0;
    const char* p = input;
    
    while (*p) {
        int digit = char_to_digit(*p);
        if (digit < 0 || digit >= base) {
            return NULL;
        }
        value = value * base + digit;
        p++;
    }
    
    // Allocate buffer for result (max 20 digits for unsigned long long + null)
    char* result = (char*)malloc(21);
    if (!result) {
        return NULL;
    }
    
    sprintf(result, "%llu", value);
    return result;
}

/*
 * convert_str_to_int_any_base()
 * -----------------------------
 * Parses a string representing a number in a given base and returns its numeric value.
 *
 * input: A string representing the number (not NULL)
 * base: The base of the input number (2-36)
 *
 * Returns: The numeric value as unsigned long long.
 *          If the input cannot be converted or base is invalid, returns 0.
 */
static inline unsigned long long convert_str_to_int_any_base(const char* input, int base)
{
    if (!input || base < 2 || base > 36) {
        return 0;
    }
    
    unsigned long long value = 0;
    const char* p = input;
    
    while (*p) {
        int digit = char_to_digit(*p);
        if (digit < 0 || digit >= base) {
            return 0;
        }
        value = value * base + digit;
        p++;
    }
    
    return value;
}

/*
 * convert_int_to_str_any_base()
 * -----------------------------
 * Converts a numeric value to a string representation in the specified base.
 *
 * value: The number to convert
 * outputBase: The base to convert to (2-36)
 *
 * Returns: A dynamically allocated string containing the number in the specified base.
 *          It is the caller's responsibility to free this buffer.
 */
static inline char* convert_int_to_str_any_base(unsigned long long value, int outputBase)
{
    if (outputBase < 2 || outputBase > 36) {
        return NULL;
    }
    
    // Handle zero case
    if (value == 0) {
        char* result = (char*)malloc(2);
        if (result) {
            result[0] = '0';
            result[1] = '\0';
        }
        return result;
    }
    
    // Buffer for digits (64 bits = max 64 binary digits + null)
    char buffer[65];
    int index = 64;
    buffer[index] = '\0';
    
    while (value > 0) {
        index--;
        buffer[index] = digit_to_char(value % outputBase);
        value /= outputBase;
    }
    
    char* result = strdup(&buffer[index]);
    return result;
}

/*
 * is_operator()
 * -------------
 * Helper function to check if a character is an operator.
 */
static inline bool is_operator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || 
           c == '%' || c == '(' || c == ')' || c == '^';
}

/*
 * convert_expression()
 * --------------------
 * Converts all numbers in a mathematical expression from one base to another.
 *
 * expression: The mathematical expression string to convert
 * inputBase: The base of numbers in the expression (2-36)
 * outputBase: The base to convert numbers to (2-36)
 *
 * Returns: A dynamically allocated string with converted numbers or NULL if error.
 *          It is the caller's responsibility to free this buffer.
 */
static inline char* convert_expression(const char* expression, int inputBase, int outputBase)
{
    if (!expression || inputBase < 2 || inputBase > 36 || 
        outputBase < 2 || outputBase > 36) {
        return NULL;
    }
    
    size_t len = strlen(expression);
    // Allocate generous buffer (conversion can expand size)
    size_t resultCapacity = len * 4 + 1;
    char* result = (char*)malloc(resultCapacity);
    if (!result) {
        return NULL;
    }
    
    size_t resultLen = 0;
    size_t i = 0;
    
    while (i < len) {
        char c = expression[i];
        
        // Check if it's the start of a number
        int digit = char_to_digit(c);
        if (digit >= 0 && digit < inputBase) {
            // Extract the full number
            size_t numStart = i;
            while (i < len) {
                digit = char_to_digit(expression[i]);
                if (digit < 0 || digit >= inputBase) {
                    break;
                }
                i++;
            }
            
            // Convert the number
            size_t numLen = i - numStart;
            char* numStr = (char*)malloc(numLen + 1);
            if (!numStr) {
                free(result);
                return NULL;
            }
            strncpy(numStr, expression + numStart, numLen);
            numStr[numLen] = '\0';
            
            // Convert to base 10 first
            char* baseTen = convert_any_base_to_base_ten(numStr, inputBase);
            free(numStr);
            
            if (!baseTen) {
                free(result);
                return NULL;
            }
            
            // Convert from base 10 to output base
            unsigned long long value = strtoull(baseTen, NULL, 10);
            free(baseTen);
            
            char* converted = convert_int_to_str_any_base(value, outputBase);
            if (!converted) {
                free(result);
                return NULL;
            }
            
            // Append converted number to result
            size_t convertedLen = strlen(converted);
            if (resultLen + convertedLen + 1 > resultCapacity) {
                resultCapacity = (resultLen + convertedLen + 1) * 2;
                char* newResult = (char*)realloc(result, resultCapacity);
                if (!newResult) {
                    free(converted);
                    free(result);
                    return NULL;
                }
                result = newResult;
            }
            
            strcpy(result + resultLen, converted);
            resultLen += convertedLen;
            free(converted);
        }
        else if (is_operator(c) || isspace(c)) {
            // Copy operator or whitespace directly
            if (resultLen + 2 > resultCapacity) {
                resultCapacity *= 2;
                char* newResult = (char*)realloc(result, resultCapacity);
                if (!newResult) {
                    free(result);
                    return NULL;
                }
                result = newResult;
            }
            result[resultLen++] = c;
            i++;
        }
        else {
            // Invalid character for the given base
            free(result);
            return NULL;
        }
    }
    
    result[resultLen] = '\0';
    return result;
}

/*
 * evaluate_expression()
 * ---------------------
 * Evaluates a mathematical expression in base 10.
 *
 * expression: The mathematical expression string (must be in base 10)
 * result: Pointer to store the result
 *
 * Returns: 0 if successful, 1 if the expression could not be evaluated or is NULL,
 *          or if the result is less than zero or >= 2^53.
 *
 * Note: This is a simple recursive descent parser for basic expressions.
 */

/* Forward declarations for recursive descent parser */
static inline int parse_expression(const char** expr, double* result);
static inline int parse_term(const char** expr, double* result);
static inline int parse_factor(const char** expr, double* result);
static inline int parse_power(const char** expr, double* result);
static inline int parse_number(const char** expr, double* result);

static inline void skip_whitespace(const char** expr)
{
    while (**expr && isspace(**expr)) {
        (*expr)++;
    }
}

static inline int parse_number(const char** expr, double* result)
{
    skip_whitespace(expr);
    
    if (!**expr) {
        return 1;
    }
    
    char* end;
    *result = strtod(*expr, &end);
    
    if (end == *expr) {
        return 1;
    }
    
    *expr = end;
    return 0;
}

static inline int parse_power(const char** expr, double* result)
{
    skip_whitespace(expr);
    
    // Handle parentheses
    if (**expr == '(') {
        (*expr)++;
        if (parse_expression(expr, result) != 0) {
            return 1;
        }
        skip_whitespace(expr);
        if (**expr != ')') {
            return 1;
        }
        (*expr)++;
    } else {
        if (parse_number(expr, result) != 0) {
            return 1;
        }
    }
    
    skip_whitespace(expr);
    
    // Handle exponentiation (right-associative)
    if (**expr == '^') {
        (*expr)++;
        double exponent;
        if (parse_power(expr, &exponent) != 0) {
            return 1;
        }
        *result = pow(*result, exponent);
    }
    
    return 0;
}

static inline int parse_factor(const char** expr, double* result)
{
    skip_whitespace(expr);
    
    // Handle unary minus
    bool negative = false;
    if (**expr == '-') {
        negative = true;
        (*expr)++;
    } else if (**expr == '+') {
        (*expr)++;
    }
    
    if (parse_power(expr, result) != 0) {
        return 1;
    }
    
    if (negative) {
        *result = -(*result);
    }
    
    return 0;
}

static inline int parse_term(const char** expr, double* result)
{
    if (parse_factor(expr, result) != 0) {
        return 1;
    }
    
    while (1) {
        skip_whitespace(expr);
        char op = **expr;
        
        if (op != '*' && op != '/' && op != '%') {
            break;
        }
        
        (*expr)++;
        double right;
        if (parse_factor(expr, &right) != 0) {
            return 1;
        }
        
        if (op == '*') {
            *result *= right;
        } else if (op == '/') {
            if (right == 0) {
                return 1;  // Division by zero
            }
            *result /= right;
        } else if (op == '%') {
            if (right == 0) {
                return 1;  // Modulo by zero
            }
            *result = fmod(*result, right);
        }
    }
    
    return 0;
}

static inline int parse_expression(const char** expr, double* result)
{
    if (parse_term(expr, result) != 0) {
        return 1;
    }
    
    while (1) {
        skip_whitespace(expr);
        char op = **expr;
        
        if (op != '+' && op != '-') {
            break;
        }
        
        (*expr)++;
        double right;
        if (parse_term(expr, &right) != 0) {
            return 1;
        }
        
        if (op == '+') {
            *result += right;
        } else {
            *result -= right;
        }
    }
    
    return 0;
}

static inline int evaluate_expression(const char* expression, unsigned long long* result)
{
    if (!expression || !result) {
        return 1;
    }
    
    const char* expr = expression;
    double evalResult;
    
    if (parse_expression(&expr, &evalResult) != 0) {
        return 1;
    }
    
    // Check for trailing characters
    skip_whitespace(&expr);
    if (*expr != '\0') {
        return 1;
    }
    
    // Check for negative result
    if (evalResult < 0) {
        return 1;
    }
    
    // Check if result exceeds 2^53 (largest exact integer in double)
    if (evalResult >= 9007199254740992.0) {  // 2^53
        return 1;
    }
    
    *result = (unsigned long long)evalResult;
    return 0;
}

#endif /* UQBASEJUMP_H */
