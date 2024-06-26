/**
 * MIT License
 *
 * Copyright (c) 2024 Arisu Wonderland
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// A small stb-style library for handling the command line.
//
// To use a provided implementation, define CLI_IMPLEMENTATION before including
// the file:
//     #define CLI_IMPLEMENTATION
//     #include "cli.h"
//
// The behaviour and dependencies of this library can be configured with
// defining these macros:
//     CLI_NO_STDIO_H
//         Do not include <stdio.h> (but fprintf() is still expected to be
//         defined).
//     CLI_NO_STDBOOL_H
//         Do not use <stdbool.h>.
//     CLI_NO_STYLES
//         Do not use colors and other styles for formatting output.
//     CLI_NOHEAP
//     CLI_NOHEAP_IMPLEMENTATION
//         Do not allocate arguments on the heap. For more information, see
//         below.
//
//     CLI_DEFAULT_ARR_CAP = 5
//         Default capacity for dynamic arrays (e.g. CliArray).
//     CLI_ASSERT = assert
//         An assert function. If not defined, assert() from <assert.h> is
//         used.
//     CLI_MALLOC = malloc
//         A function for allocating memory. If not defined, malloc() from
//         <stdlib.h> is used.
//     CLI_REALLOC = realloc
//         A function for reallocating memory. If not defined, realloc() from
//         <stdlib.h> is used.
//     CLI_FREE = free
//         A function for freeing memory. If not defined, free() from
//         <stdlib.h> is used.
//     CLI_ERROR_SYM = "✖"
//         A symbol to use in error messages. By default, it is a Unicode
//         'x' that may be unsupported by a terminal font.
//     CLI_INFO_SYM = "●"
//         A symbol to use in information messages. By default, it is a Unicode
//         circle that may be unsupported by a terminal font.
//
// Like CLI_IMPLEMENTATION, all defines that how cli.h is processed should
// be placed before the #include directive.
// Order of #define directives are not known to cli.h, thus is not important.
//
// Example pseudo-code:
//
//     int main(int argc, char** argv) {
//         Cli cli;
//
//         int exit_code = cli_parse(argc, argv, &cli);
//         if (exit_code) {
//             return exit_code;
//         }
//         process_options(&cli); // <-- Your function to validate user input
//         cli_free(&cli);
//
//         // ...
//         if (--color == "yes") {
//             cli_toggle_styles();
//         }
//         // ...
//     }
//

// It is possible to use cli.h without allocating CliArray.data on the heap.
// For that, CLI_NOHEAP and/or CLI_NOHEAP_IMPLEMENTATION should be used.
//
// Similarly to CLI_IMPLEMENTATION, CLI_NOHEAP_IMPLEMENTATION provides the
// implementation for CLI_NOHEAP.
// If CLI_NOHEAP_IMPLEMENTATION is defined, defining CLI_NOHEAP is optional and
// does not affect the behaviour.
//
// Changes that have to be made to your program:
// 1. Declare an array for arguments: CliStackNode stack[argc - 1]
// 2. Call cli_parse_noheap(argc, argv, &cli, stack) instead of cli_parse(...)
// 3. If no heap allocations are made, it is not possible for cli.h to
//    determine the capacity of the CliStackNode[] array. Thus, .capacity
//    of CliArray should not be used.
// 4. If you use cli_parse_noheap() instead of preparing variables manually,
//    .next_unused of CliStackNode cannot be used either. It points to a local
//    variable that becomes invalid after cli_parse_noheap() returns.

#ifndef __CLI_H_
#define __CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CLI_NOHEAP_IMPLEMENTATION) && !defined(CLI_NOHEAP)
#define CLI_NOHEAP
#endif

const char* CLI_RESET = "";
const char* CLI_BOLD = "";
const char* CLI_DIM = "";
const char* CLI_FORE_RED = "";
const char* CLI_FORE_BRBLUE = "";

#ifdef CLI_NOHEAP
struct CliArray {
    unsigned short length;
    const char** stack;
    const char** data;
    // A pointer to a global (between `CliArray` instances) variable that points
    // to a next unused `stack` item.
    //
    // Techinally speaking, it's a pointer to a pointer to an argument
    // (`const char*`).
    const char*** next_unused;
};
#else
struct CliArray {
    unsigned short length;
    unsigned short capacity;
    const char** data;
};
#endif // CLI_NOHEAP

typedef struct Cli {
    const char* execfile;
    struct CliArray args;
    struct CliArray cmd_options;
    struct CliArray program_options;
} Cli;

enum CliError {
    CliErrorOk,
    CliErrorUser,
    CliErrorFatal
};

/* Initialize variables for formatting output.
 *
 * If `CLI_RESET` contains an empty string, all variables are initalized with
 * escape sequences, according to their names.
 * Otherwise, variables are initalized with empty strings.
 *
 * If `CLI_NO_STYLES` is defined, does nothing.
 */
void cli_toggle_styles(void);

/*
 * Parse the command line and save results to `cli`.
 *
 * If `CLI_NOHEAP` or `CLI_NOHEAP_IMPLEMENTATION` is defined and used directly,
 * uses CLI_ASSERT to fail. In that case, cli_parse_noheap() should be used.
 */
enum CliError cli_parse(int argc, char** argv, Cli* cli);

#ifdef CLI_NOHEAP
/*
 * Parse the command line and save pointers to `stack` elements to `cli`.
 *
 * Internally, this helper function prepares `cli` fields with pointers to
 * `stack` and calls cli_parse().
 */
enum CliError cli_parse_noheap(int argc, char** argv, struct Cli* cli, const char** stack);
#endif

/* Free memory occupied by dynamic arrays.
 *
 * If either `CLI_NOHEAP` or `CLI_NOHEAP_IMPLEMENTATION` is defined, does
 * nothing.
 */
void cli_free(Cli* cli);

/********************************** =(^-x-^)= **********************************/

#ifdef CLI_IMPLEMENTATION

#define CLI_STR_(x) #x
#define CLI_STR(x)  CLI_STR_(x)

// Cannot be equal to 0. Otherwise, memory checks will be failed.
#define CLI_INVALID_PTR (const char**)0xCA10CAFE

#ifdef CLI_NO_STDBOOL_H
typedef unsigned char bool
#define true  (bool)1
#define false (bool)0
#else
#include <stdbool.h>
#endif // CLI_NO_STDBOOL_H

#ifndef CLI_NO_STDIO_H
#include <stdio.h>
#endif

#ifndef CLI_NOHEAP
#if !defined CLI_MALLOC || !defined CLI_REALLOC || !defined CLI_FREE
#include <stdlib.h>
#endif // !CLI_MALLOC || !CLI_REALLOC || !CLI_FREE
#endif

#ifndef CLI_MALLOC
#define CLI_MALLOC malloc
#endif

#ifndef CLI_REALLOC
#define CLI_REALLOC realloc
#endif

#ifndef CLI_FREE
#define CLI_FREE free
#endif

#ifndef CLI_ASSERT
#include <assert.h>
#define CLI_ASSERT assert
#endif

#ifndef CLI_DEFAULT_ARR_CAP
#define CLI_DEFAULT_ARR_CAP 5
#endif

#ifndef CLI_ERROR_SYM
#define CLI_ERROR_SYM "✖"
#endif

#ifndef CLI_INFO_SYM
#define CLI_INFO_SYM "●"
#endif

#ifndef cli_print_error
#define cli_print_error(title, msg)                                                        \
    fprintf(                                                                               \
        stderr, "%s" CLI_ERROR_SYM "%s%s " title "%s: " msg "\n", CLI_FORE_RED, CLI_RESET, \
        CLI_BOLD, CLI_RESET                                                                \
    )
#endif

#ifndef cli_printf_error
#define cli_printf_error(title, msg, ...)                                                  \
    fprintf(                                                                               \
        stderr, "%s" CLI_ERROR_SYM "%s%s " title "%s: " msg "\n", CLI_FORE_RED, CLI_RESET, \
        CLI_BOLD, CLI_RESET, __VA_ARGS__                                                   \
    )
#endif

#ifndef cli_print_info
#define cli_print_info(title, msg)                                                           \
    fprintf(                                                                                 \
        stderr, "%s" CLI_INFO_SYM "%s%s " title "%s: " msg "\n", CLI_FORE_BRBLUE, CLI_RESET, \
        CLI_BOLD, CLI_RESET                                                                  \
    )
#endif

#ifndef cli_printf_info
#define cli_printf_info(title, msg, ...)                                                     \
    fprintf(                                                                                 \
        stderr, "%s" CLI_INFO_SYM "%s%s " title "%s: " msg "\n", CLI_FORE_BRBLUE, CLI_RESET, \
        CLI_BOLD, CLI_RESET, __VA_ARGS__                                                     \
    )
#endif

#ifndef cli_printf_debug
#define cli_printf_debug(msg, ...)                                                                 \
    fprintf(                                                                                       \
        stderr, "%s" __FILE__ ":" CLI_STR(__LINE__) ":%s%sDebug%s: " msg "\n", CLI_DIM, CLI_RESET, \
        CLI_BOLD, CLI_RESET, __VA_ARGS__                                                           \
    )
#endif

    void
    cli_toggle_styles(void) {
#ifndef CLI_NO_STYLES
    if (CLI_RESET[0]) {
        CLI_RESET = "";
        CLI_BOLD = "";
        CLI_DIM = "";
        CLI_FORE_RED = "";
        CLI_FORE_BRBLUE = "";
    } else {
        CLI_RESET = "\033[0m";
        CLI_BOLD = "\033[1m";
        CLI_DIM = "\033[2m";
        CLI_FORE_RED = "\033[31m";
        CLI_FORE_BRBLUE = "\033[94m";
    }
#endif // CLI_NO_STYLES
}

#ifdef CLI_NOHEAP_IMPLEMENTATION
#define cli_da_init(array, _, __)                                                          \
    {                                                                                      \
        CLI_ASSERT(                                                                        \
            (array).stack                                                                  \
            && "(" #array                                                                  \
               ").stack is not initalized. Use cli_parse_noheap() instead of cli_parse()." \
        );                                                                                 \
        (array).data = CLI_INVALID_PTR;                                                    \
    }
#define cli_da_append(array, item)                  \
    {                                               \
        const char** node = *((array).next_unused); \
        if ((array).data == CLI_INVALID_PTR) {      \
            (array).data = node;                    \
        }                                           \
        *node = (item);                             \
        *(array).next_unused += 1;                  \
        (array).length++;                           \
    }

enum CliError cli_parse_noheap(int argc, char** argv, struct Cli* cli, const char** stack) {
    const char** unused = stack;

    cli->args = (struct CliArray) { .stack = stack, .next_unused = &unused };
    cli->cmd_options = (struct CliArray) { .stack = stack, .next_unused = &unused };
    cli->program_options = (struct CliArray) { .stack = stack, .next_unused = &unused };

    return cli_parse(argc, argv, cli);
}
#endif // CLI_NOHEAP_IMPLEMENTATION

#ifndef cli_da_init
#define cli_da_init(array, item_t, da_malloc)                                      \
    {                                                                              \
        (array).capacity = CLI_DEFAULT_ARR_CAP;                                    \
        (array).length = 0;                                                        \
        (array).data = (item_t*)(da_malloc)(CLI_DEFAULT_ARR_CAP * sizeof(item_t)); \
    }
#endif

#ifndef cli_da_append
#define cli_da_append(array, item)                                                           \
    {                                                                                        \
        if (++(array).length > (array).capacity) {                                           \
            (array).capacity *= 2;                                                           \
            (array).data                                                                     \
                = (typeof(item)*)CLI_REALLOC((array).data, (array).capacity * sizeof(item)); \
            if ((array).data == NULL) {                                                      \
                cli_print_error(                                                             \
                    "Memory error", "Unable to reallocate memory for more array items."      \
                );                                                                           \
                CLI_ASSERT(0 && "Memory error");                                             \
            }                                                                                \
        }                                                                                    \
        (array).data[(array).length - 1] = (item);                                           \
    }
#endif

static const char* cli_pop_argv(int* argc, char*** argv) {
    CLI_ASSERT(*argc);
    (*argc)--;
    return *((*argv)++);
}

enum CliError cli_parse(int argc, char** argv, Cli* cli) {
    const char* execfile = cli_pop_argv(&argc, &argv);
    if (argc > 0) {
        cli->execfile = execfile;
        cli_da_init(cli->args, const char*, CLI_MALLOC);
        if (cli->args.data == NULL) {
            cli_print_error("Memory error", "Unable to allocate memory for CLI arguments.");
            return CliErrorFatal;
        }
        cli_da_init(cli->cmd_options, const char*, CLI_MALLOC);
        if (cli->cmd_options.data == NULL) {
            cli_print_error("Memory error", "Unable to allocate memory for command options.");
            return CliErrorFatal;
        }
        cli_da_init(cli->program_options, const char*, CLI_MALLOC);
        if (cli->program_options.data == NULL) {
            cli_print_error("Memory error", "Unable to allocate memory for program options.");
            return CliErrorFatal;
        }

        const char* arg;
        bool is_cmd_option = false;
        while (argc) {
            arg = cli_pop_argv(&argc, &argv);
            if (arg[0] == '-') {
                struct CliArray* option_array
                    = is_cmd_option ? &cli->cmd_options : &cli->program_options;

                if (arg[1] == '-' && arg[2] == '\0') {
                    if (cli->args.length > 0) {
                        cli_printf_error(
                            "CLI error",
                            "Double dash ('%s') cannot be specified after the positional argument "
                            "('%s').",
                            arg, *(char**)(&cli->args.data[cli->args.length - 1])
                        );
                        return CliErrorUser;
                    }
                    is_cmd_option = true;
                } else {
                    cli_da_append(*option_array, arg);
                }
            } else {
                if (cli->cmd_options.length > 0) {
                    cli_printf_error(
                        "CLI error",
                        "Positional arguments ('%s') should be specified prior to command options.",
                        arg
                    );
                    return CliErrorUser;
                }
                cli_da_append(cli->args, arg);
                is_cmd_option = true;
            }
        }
    } else {
        *cli = (struct Cli) { 0 };
        cli->execfile = execfile;
    }
    return CliErrorOk;
}

inline void cli_free(Cli* cli) {
#ifdef CLI_NOHEAP
    (void)cli;
#else
    CLI_FREE(cli->args.data);
    CLI_FREE(cli->cmd_options.data);
    CLI_FREE(cli->program_options.data);
#endif // CLI_NOHEAP || CLI_NOHEAP_IMPLEMENTATION
}

#endif // CLI_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // __CLI_H_
