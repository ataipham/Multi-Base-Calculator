#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "uqbasejump.h"

/* Program constants */
#define MAX_BASE 36               // Maximum supported base
#define MIN_BASE 2                // Minimum supported base
#define MAX_INPUT 64              // Maximum input buffer size
#define MAX_CMD_INPUT 128         // Maximum command buffer size
#define DEFAULT_NUMBER_OF_BASES 3 // Default output bases count
#define END_OF_TRANSMISSION 4     // ASCII code for EOT character

/* DefaultBase enumeration
 * ----------------------
 * Defines commonly used number bases for the calculator.
 */
enum DefaultBase
{
    DECIMAL = 10, // Base 10 (decimal)
    BINARY = 2,   // Base 2 (binary)
    HEX = 16      // Base 16 (hexadecimal)
};

/* Key enumeration
 * ---------------
 * Defines ASCII values for special keyboard input characters.
 */
enum Key
{
    BACK_SPACE = 127, // ASCII backspace character
    ESC = 27,         // ASCII escape character
    ENTER = '\n'      // ASCII newline character
};

/* Exit enumeration
 * ----------------
 * Defines exit codes for different error conditions.
 */
enum Exit
{
    EXIT_INV_COMM_ARGS = 17, // Invalid command line arguments
    EXIT_OPEN_FILE = 13      // Unable to open file
};

/* HistoryEntry struct
 * -------------------
 * Stores a single history entry containing an expression, its base, and result.
 */
typedef struct
{
    char *expression;          // The mathematical expression string
    int base;                  // The base used for the expression
    unsigned long long result; // The calculated result
} HistoryEntry;

/* Config struct
 * -------------
 * Contains all configuration settings and state for the calculator program.
 */
typedef struct
{
    bool haveInputBase;   // Whether input base was specified via command line
    int inputBase;        // Current input base (2-36)
    int oBases[MAX_BASE]; // Array of output bases to display
    int oBasesCount;      // Number of output bases
    bool haveFile;        // Whether file input was specified
    const char *fileName; // Name of input file (if any)

    // History storage
    HistoryEntry *history;  // Dynamic array of history entries
    size_t historyCount;    // Number of entries in history
    size_t historyCapacity; // Allocated capacity for history array
} Config;

void invalid_command_line_args();
bool in_range(int base);
bool digits_only(const char *s);
char *remove_trailings(char *s);
void file_checking(const char *fileName, FILE **inputFile);
void file_expr_evaluation_display(const char *expression, int inputBase,
                                  int oBasesCount, const int *oBases);
void initialize_config(Config *cfg);
void parse_arguments(int argc, char **argv, Config *cfg);
void handle_inputbase_arg(int argc, char **argv, int *i, Config *cfg);
void handle_obases_arg(int argc, char **argv, int *i, Config *cfg);
void handle_file_arg(int argc, char **argv, int *i, Config *cfg);
void output_bases_parse(const char *basesOfOutput, Config *cfg);
void program_startup(const Config *cfg);
void add_history(Config *cfg, const char *expression, int base,
                 unsigned long long result);
void free_history(Config *cfg);
bool is_in_base_range(int ch, int base, char *outputCharacter);
void append_string(char **expressionBuffer, size_t *expressionBufferLen,
                   size_t *expressionBufferCapacity, const char *inputBuffer,
                   size_t inputBufferLen);
void append_char(
    char **expressionBuffer, size_t *exprLen, size_t *capacity, char ch);
void handle_operators(Config *cfg, char **expressionBuffer,
                      size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                      const char *inputBuffer, size_t inputBufferLen);
void evaluate_and_display_result(
    Config *cfg, char *expressionBuffer, size_t *expressionBufferLen);
void stdrd_input_expr_display(const Config *cfg, const char *expressionBuffer,
                              const char *inputBuffer);
void stdrd_input_expr_evaluation(Config *cfg);
void handle_character_input(Config *cfg, int ch, char *inputBuffer,
                            size_t *inputBufferLen, char **expressionBuffer);
void handle_operator_input(Config *cfg, int ch, char **expressionBuffer,
                           size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                           char *inputBuffer, size_t *inputBufferLen);
void handle_special_keys(Config *cfg, int ch, char **expressionBuffer,
                         size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen);
void handle_enter_key(Config *cfg, char **expressionBuffer,
                      size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                      char *inputBuffer, size_t *inputBufferLen, bool *justDisplayedResult);
void handle_just_displayed_result(Config **cfg, int ch,
                                  bool *justDisplayedResult, char **expressionBuffer,
                                  const char *inputBuffer);
void handle_colon_command(
    bool *command, size_t *commandBufferLen, char *commandBuffer);
void handle_command_mode(Config *cfg, int ch, bool *command,
                         char *commandBuffer, size_t *commandBufferLen, char **expressionBuffer,
                         size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen);
void handle_input_base_command(Config *cfg, char *commandBuffer,
                               size_t commandBufferLen, char **expressionBuffer,
                               size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen);
void handle_output_base_command(Config *cfg, char *commandBuffer,
                                size_t commandBufferLen, char **expressionBuffer,
                                size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen);
void handle_history_command(Config *cfg);

/* invalid_command_line_args()
 * ---------------------------
 * Prints usage information to stderr and exits the program with an error code.
 * This function is called when invalid command line arguments are detected.
 *
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17)
 */
void invalid_command_line_args()
{
    fprintf(stderr,
            "Usage: ./uqbasejump [--obases 2..36] [--inputbase 2..36] [--file "
            "string]\n");
    exit(EXIT_INV_COMM_ARGS);
}

/* in_range()
 * ----------
 * Checks if a given base is within the valid range for number bases.
 *
 * base: The base to check (should be between MIN_BASE and MAX_BASE inclusive)
 *
 * Returns: true if base is valid (2-36), false otherwise
 */
bool in_range(int base)
{
    return base >= MIN_BASE && base <= MAX_BASE;
}

/* digits_only()
 * -------------
 * Checks if a string contains only digit characters (0-9).
 *
 * s: The string to check (must not be NULL)
 *
 * Returns: true if string contains only digits, false otherwise
 */
bool digits_only(const char *s)
{
    for (const char *p = s; *p != '\0'; p++)
    {
        if (!isdigit(*p))
        {
            return false;
        }
    }
    return true;
}

/* remove_trailings()
 * ------------------
 * Removes trailing newline and carriage return characters from a string.
 *
 * s: The string to modify (may be NULL)
 *
 * Returns: The modified string (same pointer as input)
 */
char *remove_trailings(char *s)
{
    if (!s)
    {
        return s;
    }
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
    {
        s[--n] = '\0';
    }
    return s;
}

/* file_checking()
 * ---------------
 * Opens a file for reading and checks if the operation was successful.
 *
 * fileName: Name of the file to open (must not be NULL)
 * inputFile: Pointer to FILE* where the opened file handle will be stored
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_OPEN_FILE (13) if file cannot be opened
 */
void file_checking(const char *fileName, FILE **inputFile)
{
    FILE *file = fopen(fileName, "r");
    if (!file)
    {
        fprintf(stderr, "uqbasejump: can't read from file \"%s\"\n", fileName);
        exit(EXIT_OPEN_FILE);
    }
    *inputFile = file;
}

/* file_expr_evaluation_display()
 * ------------------------------
 * Evaluates a mathematical expression and displays the result in multiple
 * bases. Used for file-based input processing.
 *
 * expression: The mathematical expression string to evaluate
 * inputBase: The base of the input expression (2-36)
 * oBasesCount: Number of output bases to display
 * oBases: Array of output bases to display results in
 *
 * Returns: Nothing (void)
 * Errors: Prints error message to stderr if expression cannot be evaluated
 * REF: This function was developed with assistance from Cursor for
 * REF: implementing file-based mathematical expression evaluation and
 * multi-base output display.
 */
void file_expr_evaluation_display(const char *expression, int inputBase,
                                  int oBasesCount, const int *oBases)
{
    unsigned long long result = 0;
    char *expressionInDecimal = convert_expression(expression, inputBase, DECIMAL);
    int evaluateSuccessful = 0;

    if (expressionInDecimal)
    {
        evaluateSuccessful = evaluate_expression(expressionInDecimal, &result);

        if (evaluateSuccessful != 0)
        { // != 0 means unsuccessful
            // Print error message to stderr
            fprintf(stderr, "Cannot evaluate the expression \"%s\"\n",
                    expression);
            free(expressionInDecimal);
            return;
        }
        printf("Expression (base %d): %s\n", inputBase, expression);
        free(expressionInDecimal);
    }
    else
    {
        // Failed to convert
        fprintf(stderr, "Cannot evaluate the expression \"%s\"\n", expression);
        return;
    }

    char *resultInInputBase = convert_int_to_str_any_base(result, inputBase);
    printf("Result (base %d): %s\n", inputBase,
           resultInInputBase ? resultInInputBase : "0");

    for (int i = 0; i < oBasesCount; i++)
    {
        int base = oBases[i];
        char *output = convert_int_to_str_any_base(result, base);
        printf("Base %d: %s\n", base, output ? output : "0");
        free(output);
    }

    free(resultInInputBase);
    fflush(stdout);
}

/* initialize_config()
 * -------------------
 * Initializes a Config structure with default values.
 *
 * cfg: Pointer to Config structure to initialize
 *
 * Global variables modified: None
 */
void initialize_config(Config *cfg)
{
    cfg->fileName = NULL;
    cfg->haveFile = false;
    cfg->haveInputBase = false;
    cfg->inputBase = DECIMAL;
    cfg->oBases[0] = BINARY;
    cfg->oBases[1] = DECIMAL;
    cfg->oBases[2] = HEX;
    cfg->oBasesCount = DEFAULT_NUMBER_OF_BASES;

    cfg->history = NULL;
    cfg->historyCapacity = 0;
    cfg->historyCount = 0;
}

/* parse_arguments()
 * -----------------
 * Parses command line arguments and populates the configuration structure.
 *
 * argc: Number of command line arguments
 * argv: Array of command line argument strings
 * cfg: Pointer to Config structure to populate
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17) if arguments are invalid
 */
void parse_arguments(int argc, char **argv, Config *cfg)
{
    initialize_config(cfg);
    // Track which arguments have been used to prevent duplicates
    bool usedInput = false, usedOutput = false, usedFile = false;

    // Parse each command line argument starting from index 1
    for (int i = 1; i < argc; i++)
    {
        const char *argument = argv[i];
        // All valid arguments must start with '--'
        if (argument[0] != '-')
        {
            invalid_command_line_args();
        }

        if (strcmp(argument, "--inputbase") == 0)
        {
            if (usedInput)
            {
                invalid_command_line_args(); // Duplicate argument
            }
            usedInput = true;
            handle_inputbase_arg(argc, argv, &i, cfg);
        }

        else if (strcmp(argument, "--obases") == 0)
        {
            if (usedOutput)
            {
                invalid_command_line_args(); // Duplicate argument
            }
            usedOutput = true;
            handle_obases_arg(argc, argv, &i, cfg);
        }

        else if (strcmp(argument, "--file") == 0)
        {
            if (usedFile)
            {
                invalid_command_line_args(); // Duplicate argument
            }
            usedFile = true;
            handle_file_arg(argc, argv, &i, cfg);
        }

        else
        {
            invalid_command_line_args(); // Unknown argument
        }
    }
}

/* handle_inputbase_arg()
 * ----------------------
 * Processes the --inputbase command line argument.
 *
 * argc: Total number of command line arguments
 * argv: Array of command line argument strings
 * i: Pointer to current argument index (will be incremented)
 * cfg: Pointer to Config structure to modify
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17) if argument is invalid
 */
void handle_inputbase_arg(int argc, char **argv, int *i, Config *cfg)
{
    (*i)++; // Move from "--inputbase" to its value
    if (*i >= argc)
    {
        invalid_command_line_args();
    }

    const char *baseOfInput = argv[*i];
    if (baseOfInput[0] == '\0' || !digits_only(baseOfInput))
    {
        invalid_command_line_args();
    }

    int base = (int)strtol(baseOfInput, NULL, DECIMAL);
    if (!in_range(base))
    {
        invalid_command_line_args();
    }

    cfg->haveInputBase = true;
    cfg->inputBase = base;
}

/* handle_obases_arg()
 * -------------------
 * Processes the --obases command line argument.
 *
 * argc: Total number of command line arguments
 * argv: Array of command line argument strings
 * i: Pointer to current argument index (will be incremented)
 * cfg: Pointer to Config structure to modify
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17) if argument is invalid
 * REF: This function was developed with assistance from Cursor for
 * REF: processing and validating command line output base arguments.
 */
void handle_obases_arg(int argc, char **argv, int *i, Config *cfg)
{
    (*i)++; // Move from "--obases" to its value
    if (*i >= argc)
    {
        invalid_command_line_args();
    }

    const char *basesOfOutput = argv[*i];
    if (basesOfOutput[0] == '\0')
    {
        invalid_command_line_args();
    }
    output_bases_parse(basesOfOutput, cfg);
}

/* handle_file_arg()
 * -----------------
 * Processes the --file command line argument.
 *
 * argc: Total number of command line arguments
 * argv: Array of command line argument strings
 * i: Pointer to current argument index (will be incremented)
 * cfg: Pointer to Config structure to modify
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17) if argument is invalid
 */
void handle_file_arg(int argc, char **argv, int *i, Config *cfg)
{
    (*i)++; // Move from "--file" to its value
    if (*i >= argc)
    {
        invalid_command_line_args();
    }

    const char *inputFileName = argv[*i];
    if (inputFileName[0] == '\0')
    {
        invalid_command_line_args();
    }

    cfg->fileName = inputFileName;
    cfg->haveFile = true;
}

/* output_bases_parse()
 * --------------------
 * Parses a comma-separated string of output bases and validates them.
 *
 * basesOfOutput: Comma-separated string of base numbers (e.g., "2,8,16")
 * cfg: Pointer to Config structure to populate with parsed bases
 *
 * Global variables modified: None
 * Errors: Program exits with EXIT_INV_COMM_ARGS (17) if bases are invalid
 * REF: This function was developed with assistance from Cursor for
 * REF: parsing and validating comma-separated output base specifications.
 */
void output_bases_parse(const char *basesOfOutput, Config *cfg)
{
    size_t n = strlen(basesOfOutput);
    // Check for empty string or trailing/leading commas
    if (n == 0 || basesOfOutput[n - 1] == ',' || basesOfOutput[0] == ',')
    {
        invalid_command_line_args();
    }

    // Check for consecutive commas which are invalid
    for (size_t i = 1; i < n; i++)
    {
        if (basesOfOutput[i] == ',' && basesOfOutput[i - 1] == ',')
        {
            invalid_command_line_args();
        }
    }

    // Parse comma-separated base values
    char *parsing = strdup(basesOfOutput);
    char *token = strtok(parsing, ",");
    cfg->oBasesCount = 0;

    // Track which bases we've seen to prevent duplicates
    bool seen[MAX_BASE + 1] = {false};
    while (token)
    {
        // Ensure token contains only digits
        if (digits_only(token) == false)
        {
            free(parsing);
            invalid_command_line_args();
        }

        int base = (int)strtol(token, NULL, DECIMAL);
        // Check for valid range, duplicates, and capacity limits
        if ((!in_range(base) || seen[base] || cfg->oBasesCount >= MAX_BASE))
        {
            free(parsing);
            invalid_command_line_args();
        }
        seen[base] = true;
        cfg->oBases[cfg->oBasesCount++] = base;
        token = strtok(NULL, ",");
    }
    free(parsing);
}

/* program_startup()
 * -----------------
 * Displays the program welcome message and current configuration.
 *
 * cfg: Pointer to Config structure containing current settings
 *
 * Returns: Nothing (void)
 */
void program_startup(const Config *cfg)
{
    if (!cfg->haveFile)
    {
        clear_screen();
    }
    printf("Welcome to uqbasejump.\n");
    printf("This program was written by s4983508.\n");
    printf("Input base: %d\n", cfg->inputBase);
    printf("Output bases: ");
    for (int i = 0; i < cfg->oBasesCount; i++)
    {
        if (i > 0)
        {
            printf(", ");
        }
        printf("%d", cfg->oBases[i]);
    }
    printf("\n");
    if (!cfg->haveFile)
    {
        printf("Please enter your numbers and expressions.\n");
        fflush(stdout);
    }
}

/* add_history()
 * -------------
 * Adds a new entry to the calculation history.
 *
 * cfg: Pointer to Config structure containing history
 * expression: The mathematical expression string (must not be NULL)
 * base: The base used for the expression
 * result: The calculated result value
 *
 * Global variables modified: cfg->history, cfg->historyCount,
 * cfg->historyCapacity Errors: Prints error message to stderr if memory
 * allocation fails
 */
void add_history(Config *cfg, const char *expression, int base,
                 unsigned long long result)
{
    if (!cfg || !expression)
    {
        return;
    }
    // Expand history array if at capacity
    if (cfg->historyCount >= cfg->historyCapacity)
    {
        size_t newCapacity = (cfg->historyCapacity == 0)
                                 ? MAX_INPUT
                                 : (cfg->historyCapacity * 2);
        HistoryEntry *newHistory = realloc(cfg->history, sizeof(HistoryEntry) * newCapacity);
        if (!newHistory)
        {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
        cfg->history = newHistory;
        cfg->historyCapacity = newCapacity;
    }

    // Add new entry to history
    HistoryEntry *entry = &cfg->history[cfg->historyCount];
    entry->expression = strdup(expression); // Make a copy of expression
    if (!entry->expression)
    { // Check if strdup failed
        return;
    }
    cfg->historyCount++;
    entry->base = base;
    entry->result = result;
}

/* free_history()
 * --------------
 * Frees all memory allocated for the calculation history.
 *
 * cfg: Pointer to Config structure containing history (may be NULL)
 *
 * Global variables modified: cfg->history, cfg->historyCount,
 * cfg->historyCapacity
 */
void free_history(Config *cfg)
{
    if (!cfg)
    {
        return;
    }

    for (size_t i = 0; i < cfg->historyCount; i++)
    {
        free(cfg->history[i].expression);
    }
    free(cfg->history);
    cfg->history = NULL;
    cfg->historyCount = 0;
    cfg->historyCapacity = 0;
}

/* is_in_base_range()
 * ------------------
 * Checks if a character is valid for a given number base.
 *
 * ch: The character to check
 * base: The number base to validate against (2-36)
 * outputCharacter: Pointer to store the validated character (may be NULL)
 *
 * Returns: true if character is valid for the base, false otherwise
 */
bool is_in_base_range(int ch, int base, char *outputCharacter)
{
    if (isdigit(ch))
    {
        int digit = ch - '0';
        if (digit < base)
        {
            if (outputCharacter)
            {
                *outputCharacter = (char)ch;
            }
            return true;
        }
    }
    else if (isalpha(ch))
    {
        // ch = toupper(ch);
        int digit = toupper(ch) - 'A' + DECIMAL;
        if (digit < base)
        {
            if (outputCharacter)
            {
                *outputCharacter = (char)ch;
                //*outputCharacter = (char)toupper(ch);
            }
            return true;
        }
    }

    return false;
}

/* append_string()
 * ---------------
 * Appends a string to a dynamically allocated buffer, expanding as needed.
 *
 * expressionBuffer: Pointer to the buffer pointer (may be reallocated)
 * expressionBufferLen: Pointer to current buffer length
 * expressionBufferCapacity: Pointer to current buffer capacity
 * inputBuffer: String to append (must not be NULL)
 * inputBufferLen: Length of string to append
 *
 * Global variables modified: None
 * Errors: Prints error message to stderr if memory allocation fails
 */
void append_string(char **expressionBuffer, size_t *expressionBufferLen,
                   size_t *expressionBufferCapacity, const char *inputBuffer,
                   size_t inputBufferLen)
{
    // Check if we need to expand the buffer capacity
    if ((*expressionBufferLen) + (inputBufferLen) + 1 > (*expressionBufferCapacity))
    {
        // Calculate new capacity (double current or start with MAX_INPUT)
        size_t newCap = (*expressionBufferCapacity == 0)
                            ? MAX_INPUT
                            : (*expressionBufferCapacity) * 2;
        // Keep doubling until we have enough space
        while (newCap < (*expressionBufferLen) + (inputBufferLen) + 1)
        {
            newCap *= 2;
        }
        // Reallocate buffer with new capacity
        char *newBuffer = realloc(*expressionBuffer, newCap);
        if (!newBuffer)
        {
            fprintf(stderr, "Memory allocation failed\n");
            free(*expressionBuffer);
            return;
        }
        *expressionBuffer = newBuffer;
        *expressionBufferCapacity = newCap;
    }
    // Allocate initial buffer if needed
    if (!*expressionBuffer)
    {
        *expressionBuffer = malloc(*expressionBufferCapacity);
        if (!*expressionBuffer)
        {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
    }
    // Copy the input string to the buffer and update length
    memcpy(*expressionBuffer + *expressionBufferLen, inputBuffer,
           inputBufferLen);
    (*expressionBufferLen) += (inputBufferLen);
    (*expressionBuffer)[*expressionBufferLen] = '\0';
}

/* append_char()
 * -------------
 * Appends a single character to a dynamically allocated buffer.
 *
 * expressionBuffer: Pointer to the buffer pointer (may be reallocated)
 * exprLen: Pointer to current buffer length
 * capacity: Pointer to current buffer capacity
 * ch: Character to append
 *
 * Global variables modified: None
 * Errors: Prints error message to stderr if memory allocation fails
 */
void append_char(
    char **expressionBuffer, size_t *exprLen, size_t *capacity, char ch)
{
    char tmp[2] = {ch, '\0'};
    size_t len = strlen(tmp);
    append_string(expressionBuffer, exprLen, capacity, tmp, len);
}

/* handle_operators()
 * ------------------
 * Processes operator input by converting current input buffer to expression
 * format.
 *
 * cfg: Pointer to Config structure containing current settings
 * expressionBuffer: Pointer to the expression buffer pointer
 * expressionBufferLen: Pointer to expression buffer length
 * expressionBufferCapacity: Pointer to expression buffer capacity
 * inputBuffer: Current input buffer content
 * inputBufferLen: Length of current input buffer
 *
 * Global variables modified: None
 * REF: This function was refactored and broken down from a larger function with
 * REF: assistance from GitHub Copilot to improve code modularity and
 * readability.
 */
void handle_operators(Config *cfg, char **expressionBuffer,
                      size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                      const char *inputBuffer, size_t inputBufferLen)
{
    if (inputBufferLen > 0)
    {

        char *convertInputIntoBaseTen = convert_any_base_to_base_ten(inputBuffer, cfg->inputBase);
        char *convertBackToInputBase = convert_expression(
            convertInputIntoBaseTen, DECIMAL, cfg->inputBase);

        // unsigned long long result = 0;
        if (convertBackToInputBase)
        {
            size_t tmpLen = strlen(convertBackToInputBase);
            append_string(expressionBuffer, expressionBufferLen,
                          expressionBufferCapacity, convertBackToInputBase, tmpLen);
            free(convertBackToInputBase);
        }
        free(convertInputIntoBaseTen);
    }
    else
    {
        append_char(expressionBuffer, expressionBufferLen,
                    expressionBufferCapacity, '0');
    }
}

/* evaluate_and_display_result()
 * -----------------------------
 * Evaluates the current expression and displays results in all configured
 * bases.
 *
 * cfg: Pointer to Config structure containing current settings
 * expressionBuffer: The expression string to evaluate
 * expressionBufferLen: Pointer to expression buffer length (will be reset)
 *
 * Global variables modified: cfg->history (via add_history)
 * Errors: Prints error message to stderr if expression cannot be evaluated
 * REF: This function was created by breaking down a larger function with
 * assistance REF: from GitHub Copilot to improve code modularity and meet
 * function size limits.
 */
void evaluate_and_display_result(
    Config *cfg, char *expressionBuffer, size_t *expressionBufferLen)
{
    char *expressionInBaseTen = convert_expression(expressionBuffer, cfg->inputBase, DECIMAL);
    unsigned long long result = 0;
    int evaluationSuccess = 0;

    if (expressionInBaseTen)
    {
        evaluationSuccess = evaluate_expression(expressionInBaseTen, &result);

        if (evaluationSuccess != 0)
        {
            fprintf(stderr, "Cannot evaluate the expression \"%s\"\n",
                    expressionBuffer);
            *expressionBufferLen = 0;
            expressionBuffer[0] = '\0';
            free(expressionInBaseTen);
            return;
        }

        add_history(cfg, expressionBuffer, cfg->inputBase, result);

        clear_screen();
        printf("Expression (base %d): %s\n", cfg->inputBase, expressionBuffer);
        char *resultOfTheExpression = convert_int_to_str_any_base(result, cfg->inputBase);
        printf("Result (base %d): %s\n", cfg->inputBase,
               resultOfTheExpression ? resultOfTheExpression : "0");

        for (int i = 0; i < cfg->oBasesCount; i++)
        {
            int base = cfg->oBases[i];
            char *outputInBases = convert_int_to_str_any_base(result, base);
            printf("Base %d: %s\n", base, outputInBases ? outputInBases : "0");
            free(outputInBases);
        }

        free(expressionInBaseTen);
        free(resultOfTheExpression);

        *expressionBufferLen = 0;
        expressionBuffer[0] = '\0';
    }
    else
    {
        fprintf(stderr, "Cannot evaluate the expression \"%s\"\n",
                expressionBuffer);
        *expressionBufferLen = 0;
        expressionBuffer[0] = '\0';
    }
}

/* stdrd_input_expr_display()
 * --------------------------
 * Displays the current expression and input state in the interactive interface.
 *
 * cfg: Pointer to Config structure containing current settings
 * expressionBuffer: Current expression string (may be NULL)
 * inputBuffer: Current input buffer string (may be NULL)
 *
 * Returns: Nothing (void)
 */
void stdrd_input_expr_display(const Config *cfg, const char *expressionBuffer,
                              const char *inputBuffer)
{
    clear_screen();

    // Display current expression being built
    printf("Expression (base %d): %s\n", cfg->inputBase,
           expressionBuffer ? expressionBuffer : "");

    // Display current input buffer
    printf("Input (base %d): %s\n", cfg->inputBase,
           (inputBuffer && inputBuffer[0]) ? inputBuffer : "");
    unsigned long long result = 0;

    // Convert and display input in all output bases if input exists
    if (inputBuffer && inputBuffer[0])
    {
        size_t size = strlen(inputBuffer);
        char tempCapitalized[MAX_INPUT + 1] = {'\0'};

        // Convert input to uppercase for consistent base conversion
        for (size_t i = 0; i < size && i < MAX_INPUT; i++)
        {
            tempCapitalized[i] = (isalpha(inputBuffer[i]))
                                     ? toupper(inputBuffer[i])
                                     : inputBuffer[i];
        }

        tempCapitalized[size] = '\0';
        result = convert_str_to_int_any_base(tempCapitalized, cfg->inputBase);
    }

    // Display result in all configured output bases
    for (int i = 0; i < cfg->oBasesCount; i++)
    {
        int base = cfg->oBases[i];
        char *resultExpression = convert_int_to_str_any_base(result, base);
        printf("Base %d: %s\n", base,
               resultExpression ? resultExpression : "0");
        free(resultExpression);
    }
    fflush(stdout);
}

/* stdrd_input_expr_evaluation()
 * -----------------------------
 * Main interactive input loop for processing user keyboard input.
 *
 * cfg: Pointer to Config structure containing current settings
 *
 * Global variables modified: cfg->history (via expression evaluation)
 * Errors: Program exits cleanly on EOF or EOT
 * REF: This function was developed with assistance from Cursor for
 * REF: implementing the main interactive calculator input processing loop.
 */
void stdrd_input_expr_evaluation(Config *cfg)
{
    disable_line_buffering();
    char *expressionBuffer = NULL;
    size_t expressionBufferLen = 0, expressionBufferCapacity = 0,
           inputBufferLen = 0, commandBufferLen = 0;
    char inputBuffer[MAX_INPUT + 1] = {'\0'},
                                 commandBuffer[MAX_CMD_INPUT] = {'\0'};
    bool justDisplayedResult = false, command = false;
    int ch;
    while (1)
    {
        ch = getchar();
        if (ch == EOF || ch == END_OF_TRANSMISSION)
        { // Handle end of input
          // (EOF or EOT)
            enable_line_buffering();
            printf("Thank you for using uqbasejump!\n");
            free(expressionBuffer);
            free_history(cfg);
            return;
        }
        if (justDisplayedResult)
        { // Handle state after displaying a result
            handle_just_displayed_result(&cfg, ch, &justDisplayedResult,
                                         &expressionBuffer, inputBuffer);
            if (justDisplayedResult)
            {
                continue;
            }
        }
        if (command)
        { // Process input based on current mode and character type
            handle_command_mode(cfg, ch, &command, commandBuffer,
                                &commandBufferLen, &expressionBuffer, &expressionBufferLen,
                                inputBuffer, &inputBufferLen);
            continue;
        }
        if (ch == ':')
        {
            handle_colon_command(&command, &commandBufferLen, commandBuffer);
        }
        else if (ch == ESC || ch == BACK_SPACE)
        {
            handle_special_keys(cfg, ch, &expressionBuffer,
                                &expressionBufferLen, inputBuffer, &inputBufferLen);
        }
        else if (ch == ENTER)
        {
            handle_enter_key(cfg, &expressionBuffer, &expressionBufferLen,
                             &expressionBufferCapacity, inputBuffer, &inputBufferLen,
                             &justDisplayedResult);
        }
        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
        {
            handle_operator_input(cfg, ch, &expressionBuffer,
                                  &expressionBufferLen, &expressionBufferCapacity,
                                  inputBuffer, &inputBufferLen);
        }
        else
        {
            handle_character_input(
                cfg, ch, inputBuffer, &inputBufferLen, &expressionBuffer);
        }
    }
}

/* handle_character_input()
 * ------------------------
 * Processes valid character input and updates the input buffer.
 *
 * cfg: Pointer to Config structure containing current settings
 * ch: The character input to process
 * inputBuffer: Buffer to store input characters
 * inputBufferLen: Pointer to current input buffer length
 * expressionBuffer: Pointer to the expression buffer pointer
 *
 * Global variables modified: None
 * REF: This function was created by extracting character handling logic with
 * REF: assistance from GitHub Copilot to modularize the main input processing
 * loop.
 */
void handle_character_input(Config *cfg, int ch, char *inputBuffer,
                            size_t *inputBufferLen, char **expressionBuffer)
{
    char validCharacter = 0;
    if (is_in_base_range(ch, cfg->inputBase, &validCharacter))
    {
        if (*inputBufferLen < MAX_INPUT)
        {
            inputBuffer[(*inputBufferLen)++] = validCharacter;
            inputBuffer[*inputBufferLen] = '\0';
        }
    }
    // Always update the display regardless of whether character was valid/added
    stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
}

/* handle_input_base_command()
 * ---------------------------
 * Processes the :i command to change the input base.
 *
 * cfg: Pointer to Config structure to modify
 * commandBuffer: Buffer containing the command string
 * commandBufferLen: Length of the command buffer
 * expressionBuffer: Pointer to the expression buffer pointer (will be cleared)
 * expressionBufferLen: Pointer to expression buffer length (will be reset)
 * inputBuffer: Input buffer (will be cleared)
 * inputBufferLen: Pointer to input buffer length (will be reset)
 *
 * Global variables modified: cfg->inputBase
 * REF: This function was extracted from a larger command handling function with
 * REF: assistance from GitHub Copilot to improve code organization and
 * maintainability.
 */
void handle_input_base_command(Config *cfg, char *commandBuffer,
                               size_t commandBufferLen, char **expressionBuffer,
                               size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen)
{
    if (commandBufferLen > 1 && digits_only(commandBuffer + 1))
    {
        int newBase = strtol(commandBuffer + 1, NULL, DECIMAL);
        if (in_range(newBase))
        {
            cfg->inputBase = newBase;
            *inputBufferLen = 0;
            *expressionBufferLen = 0;
            inputBuffer[0] = '\0';
            if (*expressionBuffer)
            {
                (*expressionBuffer)[0] = '\0';
            }
        }
    }
    stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
}

/* handle_output_base_command()
 * ----------------------------
 * Processes the :o command to change the output bases.
 *
 * cfg: Pointer to Config structure to modify
 * commandBuffer: Buffer containing the command string
 * commandBufferLen: Length of the command buffer
 * expressionBuffer: Pointer to the expression buffer pointer (will be cleared)
 * expressionBufferLen: Pointer to expression buffer length (will be reset)
 * inputBuffer: Input buffer (will be cleared)
 * inputBufferLen: Pointer to input buffer length (will be reset)
 *
 * Global variables modified: cfg->oBases, cfg->oBasesCount
 * REF: This function was extracted from a larger command handling function with
 * REF: assistance from GitHub Copilot to improve code organization and
 * maintainability.
 */
void handle_output_base_command(Config *cfg, char *commandBuffer,
                                size_t commandBufferLen, char **expressionBuffer,
                                size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen)
{
    if (commandBufferLen > 1)
    {
        Config tmpCfg = *cfg;
        output_bases_parse(commandBuffer + 1, &tmpCfg);
        cfg->oBasesCount = tmpCfg.oBasesCount;
        for (int i = 0; i < tmpCfg.oBasesCount; i++)
        {
            cfg->oBases[i] = tmpCfg.oBases[i];
        }
        *inputBufferLen = 0;
        *expressionBufferLen = 0;
        inputBuffer[0] = '\0';
        if (*expressionBuffer)
        {
            (*expressionBuffer)[0] = '\0';
        }
    }
    stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
}

/* handle_history_command()
 * ------------------------
 * Processes the :h command to display calculation history.
 *
 * cfg: Pointer to Config structure containing history
 *
 * Returns: Nothing (void)
 * REF: This function was extracted from a larger command handling function with
 * REF: assistance from GitHub Copilot to improve code organization and
 * maintainability.
 */
void handle_history_command(Config *cfg)
{
    clear_screen();
    for (size_t i = 0; i < cfg->historyCount; i++)
    {
        printf("Expression (base %d): %s\n", cfg->history[i].base,
               cfg->history[i].expression);
        char *resultOfExpression = convert_int_to_str_any_base(
            cfg->history[i].result, cfg->history[i].base);
        printf("Result (base %d): %s\n", cfg->history[i].base,
               resultOfExpression ? resultOfExpression : "0");
        free(resultOfExpression);
    }
}

/* handle_command_mode()
 * ---------------------
 * Processes input while in command mode (:i, :o, :h commands).
 *
 * cfg: Pointer to Config structure containing current settings
 * ch: The character input to process
 * command: Pointer to command mode flag
 * commandBuffer: Buffer to store command characters
 * commandBufferLen: Pointer to command buffer length
 * expressionBuffer: Pointer to the expression buffer pointer
 * expressionBufferLen: Pointer to expression buffer length
 * inputBuffer: Current input buffer
 * inputBufferLen: Pointer to current input buffer length
 *
 * Global variables modified: cfg->inputBase, cfg->oBases, cfg->oBasesCount (via
 * subcommands) REF: This function was created by breaking down a large function
 * with assistance REF: from GitHub Copilot to handle command mode processing in
 * a modular way.
 */
void handle_command_mode(Config *cfg, int ch, bool *command,
                         char *commandBuffer, size_t *commandBufferLen, char **expressionBuffer,
                         size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen)
{
    if (ch == '\n')
    {
        if (*commandBufferLen > 0)
        {
            char typeOfCommand = commandBuffer[0];

            if (typeOfCommand == 'i')
            {
                handle_input_base_command(cfg, commandBuffer, *commandBufferLen,
                                          expressionBuffer, expressionBufferLen, inputBuffer,
                                          inputBufferLen);
            }
            else if (typeOfCommand == 'o')
            {
                handle_output_base_command(cfg, commandBuffer,
                                           *commandBufferLen, expressionBuffer,
                                           expressionBufferLen, inputBuffer, inputBufferLen);
            }
            else if (typeOfCommand == 'h' && *commandBufferLen == 1)
            {
                handle_history_command(cfg);
            }
        }
        *command = false;
        *commandBufferLen = 0;
        commandBuffer[0] = '\0';
    }
    else
    {
        if (*commandBufferLen < MAX_CMD_INPUT - 1)
        {
            commandBuffer[(*commandBufferLen)++] = (char)ch;
            commandBuffer[*commandBufferLen] = '\0';
        }
    }
}

/* handle_enter_key()
 * ------------------
 * Processes Enter key input to evaluate the current expression.
 *
 * cfg: Pointer to Config structure containing current settings
 * expressionBuffer: Pointer to the expression buffer pointer
 * expressionBufferLen: Pointer to expression buffer length
 * expressionBufferCapacity: Pointer to expression buffer capacity
 * inputBuffer: Current input buffer content
 * inputBufferLen: Pointer to current input buffer length (will be reset)
 * justDisplayedResult: Pointer to flag indicating if result was just shown
 *
 * Global variables modified: cfg->history (via expression evaluation)
 * REF: This function was created by extracting enter key handling logic with
 * REF: assistance from GitHub Copilot to modularize the main input processing
 * loop.
 */
void handle_enter_key(Config *cfg, char **expressionBuffer,
                      size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                      char *inputBuffer, size_t *inputBufferLen, bool *justDisplayedResult)
{
    if (*inputBufferLen > 0)
    {
        char *convertInputIntoBaseTen = convert_any_base_to_base_ten(inputBuffer, cfg->inputBase);
        char *convertBackToInputBase = convert_expression(
            convertInputIntoBaseTen, DECIMAL, cfg->inputBase);

        if (convertBackToInputBase)
        {
            size_t tmpLen = strlen(convertBackToInputBase);
            append_string(expressionBuffer, expressionBufferLen,
                          expressionBufferCapacity, convertBackToInputBase, tmpLen);
            free(convertInputIntoBaseTen);
            free(convertBackToInputBase);
        }
    }

    if (*inputBufferLen == 0 && *expressionBufferLen == 0)
    {
        append_char(expressionBuffer, expressionBufferLen,
                    expressionBufferCapacity, '0');
    }

    *inputBufferLen = 0;
    inputBuffer[0] = '\0';
    *justDisplayedResult = true;

    if (*expressionBufferLen > 0)
    {
        evaluate_and_display_result(
            cfg, *expressionBuffer, expressionBufferLen);
    }
    else
    {
        stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
    }
}

/* handle_special_keys()
 * ---------------------
 * Processes special keyboard input (ESC, Backspace).
 *
 * cfg: Pointer to Config structure containing current settings
 * ch: The special key character (ESC or BACK_SPACE)
 * expressionBuffer: Pointer to the expression buffer pointer
 * expressionBufferLen: Pointer to expression buffer length
 * inputBuffer: Current input buffer
 * inputBufferLen: Pointer to current input buffer length
 *
 * Global variables modified: None
 * REF: This function was created by extracting special key handling logic with
 * REF: assistance from GitHub Copilot to modularize the main input processing
 * loop.
 */
void handle_special_keys(Config *cfg, int ch, char **expressionBuffer,
                         size_t *expressionBufferLen, char *inputBuffer, size_t *inputBufferLen)
{
    if (ch == ESC)
    {
        if (*expressionBuffer)
        {
            (*expressionBuffer)[0] = '\0';
        }
        *expressionBufferLen = 0;
        inputBuffer[0] = '\0';
        *inputBufferLen = 0;
        stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
    }
    else if (ch == BACK_SPACE)
    {
        if (*inputBufferLen > 0)
        {
            inputBuffer[--(*inputBufferLen)] = '\0';
        }
        stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
    }
}

/* handle_just_displayed_result()
 * ------------------------------
 * Manages state after displaying calculation results.
 *
 * cfg: Pointer to Config structure pointer
 * ch: The character input to check
 * justDisplayedResult: Pointer to flag indicating if result was just shown
 * expressionBuffer: Pointer to the expression buffer pointer
 * inputBuffer: Current input buffer content
 *
 * Global variables modified: None
 * REF: This function was created by extracting result state management logic
 * with REF: assistance from GitHub Copilot to handle post-calculation input
 * properly.
 */
void handle_just_displayed_result(Config **cfg, int ch,
                                  bool *justDisplayedResult, char **expressionBuffer,
                                  const char *inputBuffer)
{
    if ((*justDisplayedResult))
    {
        char validChar;
        // Check if the character is an action key or valid input
        bool isAction = (ch == ':' || ch == ESC || ch == BACK_SPACE || ch == ENTER || ch == '+' || ch == '-' || ch == '*' || ch == '/');
        bool isValidInput = is_in_base_range(ch, (*cfg)->inputBase, &validChar);

        // If invalid character after result, clear screen and ignore
        if (!isAction && !isValidInput)
        {
            // This is an invalid keypress after a result was shown.
            // Clear the screen to show the empty input prompt.
            stdrd_input_expr_display(*cfg, *expressionBuffer, inputBuffer);
            *justDisplayedResult = false; // The screen is clean now.
            return;                       // Ignore the invalid character.
        }
        // Reset flag for valid characters that will be processed normally
        *justDisplayedResult = false;
    }
}

/* handle_operator_input()
 * -----------------------
 * Processes mathematical operator input (+, -, *, /).
 *
 * cfg: Pointer to Config structure containing current settings
 * ch: The operator character to process
 * expressionBuffer: Pointer to the expression buffer pointer
 * expressionBufferLen: Pointer to expression buffer length
 * expressionBufferCapacity: Pointer to expression buffer capacity
 * inputBuffer: Current input buffer content
 * inputBufferLen: Pointer to current input buffer length (will be reset)
 *
 * Global variables modified: None
 * REF: This function was created by extracting operator handling logic with
 * REF: assistance from GitHub Copilot to modularize the main input processing
 * loop.
 */
void handle_operator_input(Config *cfg, int ch, char **expressionBuffer,
                           size_t *expressionBufferLen, size_t *expressionBufferCapacity,
                           char *inputBuffer, size_t *inputBufferLen)
{
    handle_operators(cfg, expressionBuffer, expressionBufferLen,
                     expressionBufferCapacity, inputBuffer, *inputBufferLen);
    *inputBufferLen = 0;
    inputBuffer[0] = '\0';
    append_char(expressionBuffer, expressionBufferLen, expressionBufferCapacity,
                (char)ch);
    stdrd_input_expr_display(cfg, *expressionBuffer, inputBuffer);
}

/* handle_colon_command()
 * ----------------------
 * Initializes command mode when colon (:) is pressed.
 *
 * command: Pointer to command mode flag (will be set to true)
 * commandBufferLen: Pointer to command buffer length (will be reset to 0)
 * commandBuffer: Command buffer array (will be cleared)
 *
 * Global variables modified: None
 * REF: This function was created by extracting colon command initialization
 * with REF: assistance from GitHub Copilot to modularize the main input
 * processing loop.
 */
void handle_colon_command(
    bool *command, size_t *commandBufferLen, char *commandBuffer)
{
    *command = true;
    *commandBufferLen = 0;
    commandBuffer[0] = '\0';
}

int main(int argc, char **argv)
{
    Config cfg;
    parse_arguments(argc, argv, &cfg);
    bool fileHasContent = false;

    // Handle file-based input mode
    if (cfg.haveFile)
    {
        FILE *inputFile = NULL;
        file_checking(cfg.fileName, &inputFile);
        program_startup(&cfg);
        char *line = NULL;
        size_t size = 0;
        ssize_t n;

        // Process each line from the input file
        while ((n = getline(&line, &size, inputFile)) != -1)
        {
            fileHasContent = true;
            remove_trailings(line);
            file_expr_evaluation_display(
                line, cfg.inputBase, cfg.oBasesCount, cfg.oBases);
        }

        // Handle empty file case
        if (!fileHasContent)
        {
            fprintf(stderr, "Cannot evaluate the expression \"\"\n");
        }

        free(line);
        fclose(inputFile);
        printf("Thank you for using uqbasejump!\n");
    }

    // Handle interactive input mode
    else
    {
        program_startup(&cfg);
        stdrd_input_expr_evaluation(&cfg);
    }

    return 0;
}
