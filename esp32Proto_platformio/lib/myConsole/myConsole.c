                                /*****************************************************************************************
* FILENAME :        myConsole.c
*
* DESCRIPTION :
*       This module implements console line command execution
*
* AUTHOR :    Stephan Wink        CREATED ON :    31.01.2019
*
* PUBLIC FUNCTIONS :
*
* Copyright (c) [2017] [Stephan Wink]
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
*
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */

#include "myConsole.h"


#include <ctype.h>
//#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "rom/queue.h"

/****************************************************************************************/
/* Include Interfaces */

/****************************************************************************************/
/* Local constant defines */

#define SS_FLAG_ESCAPE 0x8

/****************************************************************************************/
/* Local function like makros */

/* helper macro, called when done with an argument */
#define END_ARG() do { \
    char_out = 0; \
    argv_cpp[argc++] = next_arg_start; \
    state = SS_SPACE; \
} while(0)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef struct cmdItem_tag 
{
    /**
     * Command name (statically allocated by application)
     */
    const char *command;
    /**
     * Help text (statically allocated by application), may be NULL.
     */
    const char *help;
    /**
     * Hint text, usually lists possible arguments, dynamically allocated.
     * May be NULL.
     */
    char *hint;
    myConsole_cmdFunc_t func;       //!< pointer to the command handler
    myConsole_cmdFunc2_t func2;     //!< pointer to the extended command handler
    void *argtable;                 //!< optional pointer to arg table
    SLIST_ENTRY(cmdItem_tag) next;  //!< next command in the list
} cmdItem_t;

typedef enum {
    /* parsing the space between arguments */
    SS_SPACE = 0x0,
    /* parsing an argument which isn't quoted */
    SS_ARG = 0x1,
    /* parsing a quoted argument */
    SS_QUOTED_ARG = 0x2,
    /* parsing an escape sequence within unquoted argument */
    SS_ARG_ESCAPED = SS_ARG | SS_FLAG_ESCAPE,
    /* parsing an escape sequence within a quoted argument */
    SS_QUOTED_ARG_ESCAPED = SS_QUOTED_ARG | SS_FLAG_ESCAPE,
} splitState_t;

/****************************************************************************************/
/* Local functions prototypes: */
static int HelpCommand_i(int argc, char **argv);
static int HelpCommand2_i(int argc, char **argv, FILE *retStream_xp, uint16_t *length_u16p);
static const cmdItem_t *FindCommandByName_stp(const char *name_cpc);

/****************************************************************************************/
/* Local variables: */

/** linked list of command structures */
static SLIST_HEAD(cmd_list_, cmdItem_tag) cmdList_sts;
/** run-time configuration options */
static myConsole_config_t config_sts;
/** temporary buffer used for command line parsing */
static char *tmpLineBuf_cps;
//static const char *TAG = "myConsole";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief   initialize console module, Call this once before using other console module
 *              features
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_Init_td(const myConsole_config_t *config_st)
{
    if (tmpLineBuf_cps)
    {
        return ESP_ERR_INVALID_STATE;
    }
    memcpy(&config_sts, config_st, sizeof(config_sts));

    tmpLineBuf_cps = calloc(config_st->max_cmdline_length, 1);
    if (tmpLineBuf_cps == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief   de-initialize console module Call this once when done using console module
 *              functions
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_DeInit_td()
{
    if (!tmpLineBuf_cps) {
        return ESP_ERR_INVALID_STATE;
    }
    free(tmpLineBuf_cps);
    cmdItem_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &cmdList_sts, next, tmp) {
        free(it->hint);
        free(it);
    }
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief   Initializes the console command to defaults
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_CmdInit_td(const myConsole_cmd_t *cmd_stp)
{
    return(ESP_FAIL);
}

/**---------------------------------------------------------------------------------------
 * @brief   Register console command
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_CmdRegister_td(const myConsole_cmd_t *cmd_stp)
{
    cmdItem_t *item_stp = (cmdItem_t *) calloc(1, sizeof(*item_stp));
    if (item_stp == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    if (cmd_stp->command == NULL) 
    {
        free(item_stp);
        return ESP_ERR_INVALID_ARG;
    }
    if (strchr(cmd_stp->command, ' ') != NULL) 
    {
        free(item_stp);
        return ESP_ERR_INVALID_ARG;
    }
    item_stp->command = cmd_stp->command;
    item_stp->help = cmd_stp->help;
    if (cmd_stp->hint) 
    {
        /* Prepend a space before the hint. It separates command name and
         * the hint. arg_print_syntax below adds this space as well.
         */
        int unused __attribute__((unused));
        unused = asprintf(&item_stp->hint, " %s", cmd_stp->hint);
    } else if (cmd_stp->argtable) 
    {
        /* Generate hint based on cmd_stp->argtable */
        char *buf = NULL;
        size_t buf_size = 0;
        FILE *f = open_memstream(&buf, &buf_size);
        if (f != NULL) 
        {
            arg_print_syntax(f, cmd_stp->argtable, NULL);
            fclose(f);
        }
        item_stp->hint = buf;
    }
    item_stp->argtable = cmd_stp->argtable;
    item_stp->func = cmd_stp->func;
    item_stp->func2 = cmd_stp->func2;
    cmdItem_t *last = SLIST_FIRST(&cmdList_sts);
    if (last == NULL) 
    {
        SLIST_INSERT_HEAD(&cmdList_sts, item_stp, next);
    } else 
    {
        cmdItem_t *it;
        while ((it = SLIST_NEXT(last, next)) != NULL) {
            last = it;
        }
        SLIST_INSERT_AFTER(last, item_stp, next);
    }
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief   Run command line
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_Run_td(const char *cmdline_cpc, int *cmdRet_ip)
{
    if (tmpLineBuf_cps == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    char **argv = (char **) calloc(config_sts.max_cmdline_args, sizeof(char *));
    if (argv == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    strlcpy(tmpLineBuf_cps, cmdline_cpc, config_sts.max_cmdline_length);

    size_t argc = myConsole_SplitArgv(tmpLineBuf_cps, argv,
                                         config_sts.max_cmdline_args);
    if (argc == 0)
    {
        free(argv);
        return ESP_ERR_INVALID_ARG;
    }
    const cmdItem_t *cmd_stp = FindCommandByName_stp(argv[0]);
    if (cmd_stp == NULL)
    {
        free(argv);
        return ESP_ERR_NOT_FOUND;
    }
    *cmdRet_ip = (*cmd_stp->func)(argc, argv);
    free(argv);
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief   Run command line
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_Run2_td(const char *cmdline_cpc, int *cmdRet_ip, FILE *retStream_xp, uint16_t *length_u16p)
{
    if (tmpLineBuf_cps == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    char **argv = (char **) calloc(config_sts.max_cmdline_args, sizeof(char *));
    if (argv == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    strlcpy(tmpLineBuf_cps, cmdline_cpc, config_sts.max_cmdline_length);

    size_t argc = myConsole_SplitArgv(tmpLineBuf_cps, argv,
                                         config_sts.max_cmdline_args);
    if (argc == 0)
    {
        free(argv);
        return ESP_ERR_INVALID_ARG;
    }
    const cmdItem_t *cmd_stp = FindCommandByName_stp(argv[0]);
    if (cmd_stp == NULL)
    {
        free(argv);
        return ESP_ERR_NOT_FOUND;
    }

    if(NULL != cmd_stp->func2)
    {
        if(NULL != retStream_xp)
        {
            *cmdRet_ip = (cmd_stp->func2)(argc, argv, retStream_xp, length_u16p);
        }
    }
    else
    {
        /* code */
        *cmdRet_ip = (*cmd_stp->func)(argc, argv);
    }
    
    free(argv);
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief Register a 'help' command
*//*------------------------------------------------------------------------------------*/
esp_err_t myConsole_RegisterHelpCommand()
{
    myConsole_cmd_t command_st = {
        .command = "help",
        .help = "Print the list of registered commands",
        .func = &HelpCommand_i,
        .func2 = &HelpCommand2_i
    };
    return myConsole_CmdRegister_td(&command_st);
}

/**---------------------------------------------------------------------------------------
 * @brief   Split argument list
*//*------------------------------------------------------------------------------------*/
size_t myConsole_SplitArgv(char *line_cp, char **argv_cpp, size_t argvSize_st)
{
    const int QUOTE = '"';
    const int ESCAPE = '\\';
    const int SPACE = ' ';
    splitState_t state = SS_SPACE;
    int argc = 0;
    char *next_arg_start = line_cp;
    char *out_ptr = line_cp;
    for (char *in_ptr = line_cp; argc < argvSize_st - 1; ++in_ptr) {
        int char_in = (unsigned char) *in_ptr;
        if (char_in == 0) {
            break;
        }
        int char_out = -1;

        switch (state) {
        case SS_SPACE:
            if (char_in == SPACE) {
                /* skip space */
            } else if (char_in == QUOTE) {
                next_arg_start = out_ptr;
                state = SS_QUOTED_ARG;
            } else if (char_in == ESCAPE) {
                next_arg_start = out_ptr;
                state = SS_ARG_ESCAPED;
            } else {
                next_arg_start = out_ptr;
                state = SS_ARG;
                char_out = char_in;
            }
            break;

        case SS_QUOTED_ARG:
            if (char_in == QUOTE) {
                END_ARG();
            } else if (char_in == ESCAPE) {
                state = SS_QUOTED_ARG_ESCAPED;
            } else {
                char_out = char_in;
            }
            break;

        case SS_ARG_ESCAPED:
        case SS_QUOTED_ARG_ESCAPED:
            if (char_in == ESCAPE || char_in == QUOTE || char_in == SPACE) {
                char_out = char_in;
            } else {
                /* unrecognized escape character, skip */
            }
            state = (splitState_t) (state & (~SS_FLAG_ESCAPE));
            break;

        case SS_ARG:
            if (char_in == SPACE) {
                END_ARG();
            } else if (char_in == ESCAPE) {
                state = SS_ARG_ESCAPED;
            } else {
                char_out = char_in;
            }
            break;
        }
        /* need to output anything? */
        if (char_out >= 0) {
            *out_ptr = char_out;
            ++out_ptr;
        }
    }
    /* make sure the final argument is terminated */
    *out_ptr = 0;
    /* finalize the last argument */
    if (state != SS_SPACE && argc < argvSize_st - 1) {
        argv_cpp[argc++] = next_arg_start;
    }
    /* add a NULL at the end of argv */
    argv_cpp[argc] = NULL;

    return argc;
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief   Help command function, prints all commands registered to console
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param[in]   argc  number of arguments
 * @param[in]   argv list of arguments
 * @return      0U
*//*------------------------------------------------------------------------------------*/
static int HelpCommand_i(int argc, char **argv)
{
    cmdItem_t *it_stp;
    
    /* Print summary of each command */
    SLIST_FOREACH(it_stp, &cmdList_sts, next)
    {
        if (it_stp->help == NULL)
        {
            continue;
        }
        /* First line: command name and hint
         * Pad all the hints to the same column
         */
        const char *hint = (it_stp->hint) ? it_stp->hint : "";
        printf("%-s %s\n", it_stp->command, hint);
        /* Second line: print help.
         * Argtable has a nice helper function for this which does line
         * wrapping.
         */
        printf("  "); // arg_print_formatted does not indent the first line
        arg_print_formatted(stdout, 2, 78, it_stp->help);
        /* Finally, print the list of arguments */
        if (it_stp->argtable)
        {
            arg_print_glossary(stdout, (void **) it_stp->argtable, "  %12s  %s\n");
        }
        printf("\n");
    }
    return 0U;
}

/**---------------------------------------------------------------------------------------
 * @brief   Help command function, prints all commands registered to console
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param[in]   argc            number of arguments
 * @param[in]   argv            list of arguments
 * @param[out]  retStream_xp    stream for return data
 * @param[out]  length_u16p     bytes send to out stream
 * @return      0U
*//*------------------------------------------------------------------------------------*/
static int HelpCommand2_i(int argc, char **argv, FILE *retStream_xp, uint16_t *length_u16p)
{
    cmdItem_t *it_stp;

    if(NULL != retStream_xp)
    {   
        /* Print summary of each command */
        SLIST_FOREACH(it_stp, &cmdList_sts, next)
        {
            if (it_stp->help == NULL)
            {
                continue;
            }
            /* First line: command name and hint
            * Pad all the hints to the same column
            */
            const char *hint = (it_stp->hint) ? it_stp->hint : "";
            fprintf(retStream_xp, "%-s %s\n", it_stp->command, hint);
            /* Second line: print help.
            * Argtable has a nice helper function for this which does line
            * wrapping.
            */
            fprintf(retStream_xp, "  "); // arg_print_formatted does not indent the first line
            arg_print_formatted(retStream_xp, 2, 78, it_stp->help);
            /* Finally, print the list of arguments */
            if (it_stp->argtable)
            {
                arg_print_glossary(retStream_xp, (void **) it_stp->argtable, "  %12s  %s\n");
            }
            fprintf(retStream_xp, "\n");
        }
    }

    return 0U;
}

/**---------------------------------------------------------------------------------------
 * @brief   Search in the command list and find the command by key "name"
 * @author  S. Wink
 * @date    31. Jan. 2019
 * @param[in]   name_cpc command name
 * @return      pointer to the command item, else NULL
*//*------------------------------------------------------------------------------------*/
static const cmdItem_t *FindCommandByName_stp(const char *name_cpc)
{
    const cmdItem_t *cmd_stp = NULL;
    cmdItem_t *it;
    SLIST_FOREACH(it, &cmdList_sts, next)
    {
        if (strcmp(name_cpc, it->command) == 0)
        {
            cmd_stp = it;
            break;
        }
    }
    return cmd_stp;
}
