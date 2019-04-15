/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007-2009  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "main.h"

#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

/* From jobs.c */
extern int busy_slots;
extern int max_slots;

/* This is an approach the make the printed table more readable, even if
 * there are long entries for output file, times, label and command. */
/* static const int target_table_width = 100; */
static const int col_width_id = 4;
static const int col_width_state = 8;
static const int col_width_output_max = 24; /* use function */
static const int col_width_elevel = 4;
static const int col_width_times = 14; /* < 20, or change code */

static int get_col_width_output()
{
    /* Copied from execute.c */
    const char *outfname = "/o.XXXXXX";
    const char *tmpdir = getenv("TMPDIR");
    int path_length;
    const char *col_title = "Output File";

    if (tmpdir == NULL)
        tmpdir = "/tmp";
    path_length = strlen(tmpdir) + strlen(outfname);

    /* At least as big as col_title but smaller than col_width_output_max */
    return max(min(path_length, col_width_output_max), strlen(col_title));
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

char * joblist_headers()
{
    char * format_str;
    char * line;

    format_str = malloc(100);
    snprintf(format_str, 100,
            "%%-%ds %%-%ds %%-%ds %%-%ds %%-%ds %%s [run=%%i/%%i]\n",
            col_width_id,
            col_width_state,
            get_col_width_output(),
            col_width_elevel,
            col_width_times);

    line = malloc(100);
    snprintf(line, 100, format_str,
            "ID",
            "State",
            "Output File",
            "Err", /* E-Level */
            "Times (r/u/s)",
            "Command",
            busy_slots,
            max_slots);

    free(format_str);
    return line;
}

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
                /* This may happen due to concurrency
                 * problems */
                output_filename = "(...)";
            else
                output_filename = p->output_filename;
        }
    } else
        output_filename = "stdout";


    return output_filename;
}

static char * print_noresult(const struct Job *p)
{
    const char * jobstate;
    const char * output_filename;
    char * trimmed_filename;
    const int col_width_output = get_col_width_output();
    int maxlen;
    char * format_str;
    char * line;
    /* 18 chars should suffice for a string like "[int]&& " */
    char dependstr[18] = "";

    jobstate = jstate2string(p->state); /* max. length: 8 */
    output_filename = ofilename_shown(p);

    trimmed_filename = malloc(col_width_output + 1);
    if (strlen(output_filename) > col_width_output)
    {
        strcpy(trimmed_filename, "...");
        strcat(trimmed_filename + 3,
            output_filename + strlen(output_filename) - col_width_output + 3);
    } else
        strcpy(trimmed_filename, output_filename);

    maxlen = col_width_id + 1
           + col_width_state + 1
           + col_width_output + 1
           + col_width_elevel + 1
           + col_width_times + 1
           + strlen(p->command) + 20; /* 20 is the margin for errors */

    if (p->label)
        maxlen += 3 + strlen(p->label);
    if (p->do_depend)
    {
        maxlen += sizeof(dependstr);
        if (p->depend_on == -1)
            snprintf(dependstr, sizeof(dependstr), "&& ");
        else
            snprintf(dependstr, sizeof(dependstr), "[%i]&& ", p->depend_on);
    }

    line = (char *) malloc(maxlen);
    if (line == NULL)
        error("Malloc for %i failed.\n", maxlen);

    format_str = malloc(100);
    snprintf(format_str, 100, "%%-%di %%-%ds %%-%ds %%-%ds %%%ds",
            col_width_id,
            col_width_state,
            col_width_output,
            col_width_elevel,
            col_width_times);

    if (p->label)
    {
        strcat(format_str, " %s[%s] %s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                trimmed_filename,
                "",
                "",
		        dependstr,
                p->label,
                p->command);
    }
    else
    {
        strcat(format_str, " %s%s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                trimmed_filename,
                "",
                "",
		        dependstr,
                p->command);
    }

    free(trimmed_filename);
    free(format_str);
    return line;
}

static char * print_result(const struct Job *p)
{
    const char * jobstate;
    const char * output_filename;
    char * trimmed_filename;
    const int col_width_output = get_col_width_output();
    int maxlen;
    char * time_str;
    char * format_str;
    char * line;

    /* 18 chars should suffice for a string like "[int]&& " */
    char dependstr[18] = "";

    jobstate = jstate2string(p->state); /* max. length: 8 */
    output_filename = ofilename_shown(p);

    trimmed_filename = malloc(col_width_output + 1);
    if (strlen(output_filename) > col_width_output)
    {
        strcpy(trimmed_filename, "...");
        strcat(trimmed_filename + 3,
            output_filename + strlen(output_filename) - col_width_output + 3);
    } else
        strcpy(trimmed_filename, output_filename);

    maxlen = col_width_id + 1
           + col_width_state + 1
           + col_width_output + 1
           + col_width_elevel + 1
           + col_width_times + 1
           + strlen(p->command) + 20; /* 20 is the margin for errors */

    if (p->label)
        maxlen += 3 + strlen(p->label);
    if (p->do_depend)
    {
        maxlen += sizeof(dependstr);
        if (p->depend_on == -1)
            snprintf(dependstr, sizeof(dependstr), "&& ");
        else
            snprintf(dependstr, sizeof(dependstr), "[%i]&& ", p->depend_on);
    }

    line = (char *) malloc(maxlen);
    if (line == NULL)
        error("Malloc for %i failed.\n", maxlen);

    /* Print times beforehand for easy allignment */
    time_str = malloc(20);
    snprintf(time_str, 20, "%0.2f/%0.2f/%0.2f",
            p->result.real_ms,
            p->result.user_ms,
            p->result.system_ms);

    /* If times are too long, try again with less trailing decimal places */
    if (strlen(time_str) > col_width_times)
        snprintf(time_str, 20, "%0.1f/%0.1f/%0.1f",
                p->result.real_ms,
                p->result.user_ms,
                p->result.system_ms);

    /* If times are still too long, print without trailing decimal places */
    if (strlen(time_str) > col_width_times)
        snprintf(time_str, 20, "%0.0f/%0.0f/%0.0f",
                p->result.real_ms,
                p->result.user_ms,
                p->result.system_ms);

    format_str = malloc(100);
    snprintf(format_str, 100,
            "%%-%di %%-%ds %%-%ds %%-%di %%-%ds",
            col_width_id,
            col_width_state,
            col_width_output,
            col_width_elevel,
            col_width_times);

    if (p->label)
    {
        strcat(format_str, " %s[%s] %s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                trimmed_filename,
                p->result.errorlevel,
                time_str,
                dependstr,
                p->label,
                p->command);
    } else
    {
        strcat(format_str, " %s%s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                trimmed_filename,
                p->result.errorlevel,
                time_str,
                dependstr,
                p->command);
    }

    free(time_str);
    free(trimmed_filename);
    free(format_str);
    return line;
}

char * joblist_line(const struct Job *p)
{
    char * line;

    if (p->state == FINISHED)
        line = print_result(p);
    else
        line = print_noresult(p);

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
