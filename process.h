#ifndef PROCESS_H
#define PROCESS_H

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libproc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <ctype.h>
#include <signal.h>

#define MAX_PROCESSES 2048
#define PROCESS_NAME_LEN 256

struct ProcessInfo {
    pid_t pid;
    char name[PROCESS_NAME_LEN];
    bool system_process;
    char *username;
};

void get_processes();

int terminate_pid(pid_t pid);
extern struct ProcessInfo processes[MAX_PROCESSES];
extern int process_count;
extern char ERROR_MSG[256];
extern bool no_username;
extern bool dont_get_fancy;
extern bool ignore_errors;
extern bool hide_system_processes;

#endif