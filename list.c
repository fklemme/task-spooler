/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007-2019  Lluís Batlle i Rossell
    Edited by Florian Klemme <mail@florianklemme.de>

    Please find the license in the provided COPYING file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "main.h"

/* From jobs.c */
extern int busy_slots;
extern int max_slots;

/* Filename to show in joblist table (unchanged) */
static const char * ofilename_shown(const struct Job *p)
{
    const char * output_filename;

    if (p->state == SKIPPED)
    {
        output_filename = "(no output)";
    } else if (p->store_output)
    {
        if (p->state == QUEUED)
        {
            output_filename = "(file)";
        } else
        {
            if (p->output_filename == 0)
                /* This may happen due to concurrency problems */
                output_filename = "(...)";
            else
                output_filename = p->output_filename;
        }
    } else
        output_filename = "stdout";


    return output_filename;
}

/* This is an approach the make the printed table more readable, even if
 * there are long entries for output file, times, label and command. */
char **joblist_table(const struct Job **job_list, int job_list_size)
{
    /* Settings: column widths */
    const int table_width = 100;    /* fix */
    const int col_width_id = 4;     /* fix */
    const int col_width_state = 8;  /* fix, jstate2string() -> max length 8 */
    const int col_width_output_max = 24; /* upper limit */
    const int col_width_elevel = 4; /* fix */
    const int col_width_times = 14; /* fix */
    int col_width_command;          /* rest up to table_width, calculated */
    int col_width_output;           /* will be also calculated */
    /* Settings: column names */
    const char *const header_id = "ID";
    const char *const header_state = "State";
    const char *const header_output = "Output";
    const char *const header_elevel = "Err"; /* short to save space */
    const char *const header_times = "Times (r/u/s)";
    const char *const header_command_template = "Command [run=%d/%d]";
    char header_command[30]; /* rought estimation, enough */
    /* Other variables */
    const struct Job **job_pptr; /* job list iterator */
    const struct Job **const job_list_end = job_list + job_list_size;
    const struct Job *job_ptr; /* "current job pointer" in loop */
    int filepath_size = 0;
    int max_filepath_size = 0;
    char **table; /* printable table, result of this function */
    int line_count = 0;
    char *format_str; /* store format for printf */
    char *output_str = malloc(col_width_output_max + 1); /* upper limit */
    char elevel_str[10]; /* should be more than sufficient to render number */
    char *times_str = malloc(col_width_times + 11); /* space to compare */
    char depend_str[18]; /* 18 should suffice for a string like "[int]&& " */
    char *command_str; /* use malloc, might be long */

    if (output_str == NULL || times_str == NULL)
        error("Malloc failed.\n"); /* used malloc because of ISO C90 */

    /* Find suitable size for "col_width_output" */
    for (job_pptr = job_list; job_pptr != job_list_end; ++job_pptr)
    {
        filepath_size = strlen(ofilename_shown(*job_pptr));
        if (filepath_size > max_filepath_size)
            max_filepath_size = filepath_size;
    }
    col_width_output = max_filepath_size < col_width_output_max
        ? max_filepath_size : col_width_output_max;

    /* Initialize more variables */
    col_width_command = table_width - (col_width_id + col_width_state
        + col_width_output + col_width_elevel + col_width_times + 5);
    if (!(col_width_command > 0)) /* assert */
        error("Assert (col_width_command > 0) failed: %d!\n", col_width_command);

    snprintf(header_command, 30, header_command_template, busy_slots, max_slots);

    /* Estimate header plus two lines for each job (worst case) */
    table = malloc((1 + job_list_size * 2) * sizeof(char *));
    if (table == NULL)
        error("Malloc for %i failed.\n", (1 + job_list_size * 2) * sizeof(char *));
    format_str = malloc(100); /* rough upper limit */
    if (format_str == NULL)
        error("Malloc for %i failed.\n", 100);

    /* --- Print header --- */
    /* Prepare format string */
    snprintf(format_str, 100, "%%-%ds %%-%ds %%-%ds %%-%ds %%-%ds %%s\n",
        col_width_id, col_width_state, col_width_output,
        col_width_elevel, col_width_times); /* no limit for "command" */

    /* Allocate memory, assume upper limit of "table_width" + 20 chars */
    table[line_count] = malloc(table_width + 20);
    if (table[line_count] == NULL)
        error("Malloc for %i failed.\n", table_width + 20);

    /* Print header line */
    snprintf(table[line_count], table_width + 20, format_str,
        header_id, header_state, header_output,
        header_elevel, header_times, header_command);
    ++line_count;

    /* --- Print jobs --- */
    command_str = malloc(table_width + 10); /* space to compare length */
    if (command_str == NULL)
        error("Malloc for %i failed.\n", table_width + 10);

    for (job_pptr = job_list; job_pptr != job_list_end; ++job_pptr)
    {
        job_ptr = *job_pptr; /* current job pointer */

        /* Allocate memory, assume upper limit of "table_width" + 20 chars */
        table[line_count] = malloc(table_width + 20);
        if (table[line_count] == NULL)
            error("Malloc for %i failed.\n", table_width + 20);

        /* Prepare output string, trim if necessary */
        filepath_size = strlen(ofilename_shown(job_ptr));
        if (filepath_size > col_width_output)
            snprintf(output_str, col_width_output, "...%s",
                ofilename_shown(job_ptr) + filepath_size - col_width_output + 3);
        else
            strcpy(output_str, ofilename_shown(job_ptr));

        /* Prepare error level string */
        if (job_ptr->state == FINISHED)
            snprintf(elevel_str, 10, "%d", job_ptr->result.errorlevel);
        else
            strcpy(elevel_str, "");

        /* Prepare times string */
        if (job_ptr->state == FINISHED)
        {
            /* Try two decimal places <= col_width_times */
            snprintf(times_str, col_width_times + 10, "%0.2f/%0.2f/%0.2f",
                job_ptr->result.real_ms,
                job_ptr->result.user_ms,
                job_ptr->result.system_ms);
            if (strlen(times_str) > col_width_times)
                /* Try one decimal place <= col_width_times */
                snprintf(times_str, col_width_times + 10, "%0.1f/%0.1f/%0.1f",
                    job_ptr->result.real_ms,
                    job_ptr->result.user_ms,
                    job_ptr->result.system_ms);
            if (strlen(times_str) > col_width_times)
                /* Fallback: no decimal places */
                snprintf(times_str, col_width_times + 10, "%0.0f/%0.0f/%0.0f",
                    job_ptr->result.real_ms,
                    job_ptr->result.user_ms,
                    job_ptr->result.system_ms);
        } else
            strcpy(times_str, "");

        /* Prepare depend_str, will be included in command_str */
        if (!job_ptr->do_depend)
            strcpy(depend_str, "");
        else if (job_ptr->depend_on == -1)
            strcpy(depend_str, "&& ");
        else
            snprintf(depend_str, 18, "[%i]&& ", job_ptr->depend_on);

        /* Prepare command string */
        if (job_ptr->label)
            snprintf(command_str, table_width + 10, "%s[%s] %s",
                depend_str, job_ptr->label, job_ptr->command);
        else
            snprintf(command_str, table_width + 10, "%s%s",
                depend_str, job_ptr->command);

        /* Print line */
        if (strlen(command_str) <= col_width_command) /* all in one line */
        {
            /* Prepare format string */
            snprintf(format_str, 100, "%%-%dd %%-%ds %%-%ds %%-%ds %%-%ds %%s\n",
                col_width_id, col_width_state, col_width_output, col_width_elevel,
                col_width_times); /* no explicit limit for "command" */

            /* Print line */
            snprintf(table[line_count], table_width + 20, format_str,
                job_ptr->jobid, jstate2string(job_ptr->state),
                output_str, elevel_str, times_str, command_str);
            ++line_count;
        } else /* Print command to extra line */
        {
            /* Prepare format string */
            snprintf(format_str, 100, "%%-%dd %%-%ds %%-%ds %%-%ds %%-%ds %%s\n",
                col_width_id, col_width_state, col_width_output, col_width_elevel,
                col_width_times); /* no explicit limit for "command" */

            /* Print first line */
            snprintf(table[line_count], table_width + 20, format_str,
                job_ptr->jobid, jstate2string(job_ptr->state),
                output_str, elevel_str, times_str, "(next line)");
            ++line_count;

            /* Again, allocate memory for extra line... */
            table[line_count] = malloc(table_width + 20);
            if (table[line_count] == NULL)
                error("Malloc for %i failed.\n", table_width + 20);

            /* Trim command_str, if it's still too long */
            /* Also consider indention of "col_width_id + 1" */
            if (strlen(command_str) > table_width - (col_width_id + 1))
                strcpy(command_str + table_width - (col_width_id + 1) - 3, "...");

            /* Print second (command) line */
            snprintf(format_str, 100, "%%-%ds %%s\n", col_width_id);
            snprintf(table[line_count], table_width + 20, format_str,
                /* indention string: */ "", command_str);
            ++line_count;
        }
    }

    /* --- done --- */
    free(format_str);
    free(output_str); /* used malloc because of ISO C90 */
    free(times_str); /* used malloc because of ISO C90 */
    free(command_str);
    table[line_count] = NULL; /* indicate end */
    return table;
}

char * joblistdump_headers()
{
    char * line;

    line = malloc(600);
    snprintf(line, 600, "#!/bin/sh\n# - task spooler (ts) job dump\n"
            "# This file has been created because a SIGTERM killed\n"
            "# your queue server.\n"
            "# The finished commands are listed first.\n"
            "# The commands running or to be run are stored as you would\n"
            "# probably run them. Take care - some quotes may have got"
            " broken\n\n");

    return line;
}

char * joblistdump_torun(const struct Job *p)
{
    int maxlen;
    char * line;

    maxlen = 10 + strlen(p->command) + 20; /* 20 is the margin for errors */

    line = (char *) malloc(maxlen);
    if (line == NULL)
        error("Malloc for %i failed.\n", maxlen);

    snprintf(line, maxlen, "ts %s\n", p->command);

    return line;
}
