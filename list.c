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

/* From jobs.c */
extern int busy_slots;
extern int max_slots;

static const int col_width_id = 4;
static const int col_width_state = 10;
static const int col_width_output = 20;
static const int col_width_elevel = 8;
static const int col_width_times = 14;

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
            col_width_output,
            col_width_elevel,
            col_width_times);

    line = malloc(100);
    snprintf(line, 100, format_str,
            "ID",
            "State",
            "Output",
            "E-Level",
            "Times(r/u/s)",
            "Command",
            busy_slots,
            max_slots);

    free(format_str);
    return line;
}

static int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
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
    int maxlen;
    char * format_str;
    char * line;
    /* 18 chars should suffice for a string like "[int]&& " */
    char dependstr[18] = "";

    jobstate = jstate2string(p->state);
    output_filename = ofilename_shown(p);

    maxlen = col_width_id + 1
           + col_width_state + 1
           + max(col_width_output, strlen(output_filename)) + 1
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
        strcat(format_str, " %s[%s]%s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                output_filename,
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
                output_filename,
                "",
                "",
		        dependstr,
                p->command);
    }

    free(format_str);
    return line;
}

static char * print_result(const struct Job *p)
{
    const char * jobstate;
    int maxlen;
    char * format_str;
    char * line;
    const char * output_filename;
    /* 18 chars should suffice for a string like "[int]&& " */
    char dependstr[18] = "";

    jobstate = jstate2string(p->state);
    output_filename = ofilename_shown(p);

    maxlen = col_width_id + 1
           + col_width_state + 1
           + max(col_width_output, strlen(output_filename)) + 1
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
    snprintf(format_str, 100,
            "%%-%di %%-%ds %%-%ds %%-%di %%0.2f/%%0.2f/%%0.2f",
            col_width_id,
            col_width_state,
            col_width_output,
            col_width_elevel);

    if (p->label)
    {
        strcat(format_str, " %s[%s]%s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                output_filename,
                p->result.errorlevel,
                p->result.real_ms,
                p->result.user_ms,
                p->result.system_ms,
                dependstr,
                p->label,
                p->command);
    } else
    {
        strcat(format_str, " %s%s\n");
        snprintf(line, maxlen, format_str,
                p->jobid,
                jobstate,
                output_filename,
                p->result.errorlevel,
                p->result.real_ms,
                p->result.user_ms,
                p->result.system_ms,
                dependstr,
                p->command);
    }

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
