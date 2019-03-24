/*****************************************************************************************
* FILENAME :        myConsole.h
*
* DESCRIPTION :
*       Header file for console parsing independent on the input channel
*
* Date: 24. January 2019
*
* NOTES :
*
* Copyright (c) [2019] [Stephan Wink]
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
vAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*****************************************************************************************/
#ifndef MYCONSOLE_H
#define MYCONSOLE_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************************/
/* Imported header files: */

#include <stddef.h>
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

/**
 * @brief Parameters for console initialization
 */
typedef struct {
    size_t max_cmdline_length;  //!< length of command line buffer, in bytes
    size_t max_cmdline_args;    //!< maximum number of command line arguments to parse
} myConsole_config_t;

/**
 * @brief Console command main function
 * @param argc number of arguments
 * @param argv array with argc entries, each pointing to a zero-terminated string argument
 * @return console command return code, 0 indicates "success"
 */
typedef int (*myConsole_cmdFunc_t)(int argc, char** argv);

/**
 * @brief Console command description
 */
typedef struct {
    /**
     * Command name. Must not be NULL, must not contain spaces.
     * The pointer must be valid until the call to esp_console_deinit.
     */
    const char* command;    //!< command name
    /**
     * Help text for the command, shown by help command.
     * If set, the pointer must be valid until the call to esp_console_deinit.
     * If not set, the command will not be listed in 'help' output.
     */
    const char* help;
    /**
     * Hint text, usually lists possible arguments.
     * If set to NULL, and 'argtable' field is non-NULL, hint will be generated
     * automatically
     */
    const char* hint;
    /**
     * Pointer to a function which implements the command.
     */
    myConsole_cmdFunc_t func;
    /**
     * Array or structure of pointers to arg_xxx structures, may be NULL.
     * Used to generate hint text if 'hint' is set to NULL.
     * Array/structure which this field points to must end with an arg_end.
     * Only used for the duration of esp_console_cmd_register call.
     */
    void* argtable;
} myConsole_cmd_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief   initialize console module, Call this once before using other console module
 *              features
 * @author  S. Wink
 * @date    24. Jan. 2019
 * @param   config_st    pointer to configuration structure
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_NO_MEM if out of memory
 *          - ESP_ERR_INVALID_STATE if already initialized
*//*------------------------------------------------------------------------------------*/
extern esp_err_t myConsole_Init_td(const myConsole_config_t *config_st);

/**---------------------------------------------------------------------------------------
 * @brief   de-initialize console module Call this once when done using console module
 *              functions
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_STATE if not initialized yet
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_DeInit_td();

/**---------------------------------------------------------------------------------------
 * @brief   Register console command
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param   command pointer to the command description; can point to a temporary value
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_NO_MEM if out of memory
*//*------------------------------------------------------------------------------------*/
extern esp_err_t myConsole_CmdRegister_td(const myConsole_cmd_t *cmd_stp);

/**---------------------------------------------------------------------------------------
 * @brief   Run command line
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param cmdline command line (command name followed by a number of arguments)
 * @param[out] cmd_ret return code from the command (set if command was run)
 * @return
 *      - ESP_OK, if command was run
 *      - ESP_ERR_INVALID_ARG, if the command line is empty, or only contained
 *        whitespace
 *      - ESP_ERR_NOT_FOUND, if command with given name wasn't registered
 *      - ESP_ERR_INVALID_STATE, if esp_console_init wasn't called
*//*------------------------------------------------------------------------------------*/
extern esp_err_t myConsole_Run_td(const char* cmdline_cpc, int* cmdReturn_ip);

/**---------------------------------------------------------------------------------------
 * @brief Split command line into arguments in place
 *
 * - This function finds whitespace-separated arguments in the given input line.
 *
 *     'abc def 1 20 .3' -> [ 'abc', 'def', '1', '20', '.3' ]
 *
 * - Argument which include spaces may be surrounded with quotes. In this case
 *   spaces are preserved and quotes are stripped.
 *
 *     'abc "123 456" def' -> [ 'abc', '123 456', 'def' ]
 *
 * - Escape sequences may be used to produce backslash, double quote, and space:
 *
 *     'a\ b\\c\"' -> [ 'a b\c"' ]
 *
 * Pointers to at most argv_size - 1 arguments are returned in argv array.
 * The pointer after the last one (i.e. argv[argc]) is set to NULL.
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param   line_cp pointer to buffer to parse; it is modified in place
 * @param   argv_cpp array where the pointers to arguments are written
 * @param   argvSize_st number of elements in argv_array (max. number of arguments)
 * @return  number of arguments found (argc)
*//*------------------------------------------------------------------------------------*/
extern size_t myConsole_SplitArgv(char *line_cp, char **argv_cpp, size_t argvSize_st);

/**---------------------------------------------------------------------------------------
 * @brief Register a 'help' command
 *          Default 'help' command prints the list of registered commands along with
 *          hints and help strings.
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE, if esp_console_init wasn't called
*//*------------------------------------------------------------------------------------*/
extern esp_err_t myConsole_RegisterHelpCommand();

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //MYCONSOLE_H
