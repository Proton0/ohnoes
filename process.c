#include "process.h"
#include <pwd.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include "display.h"

// Define external variables
struct ProcessInfo processes[MAX_PROCESSES];
int process_count = 0;
char ERROR_MSG[256];
bool no_username = false;
bool dont_get_fancy = false;
bool ignore_errors = false;
bool hide_system_processes = false;

struct ProcessInfo processes[MAX_PROCESSES];
char ERROR_MSG[256];


void get_processes() {
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
  size_t size;
  struct kinfo_proc *procs;

  if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) {
    handle_error("Failed to get process list size");
    return;
  }

  procs = malloc(size);
  if (!procs) {
    handle_error("Failed to allocate memory!");
    return;
  }

  if (sysctl(mib, 4, procs, &size, NULL, 0) < 0) {
    free(procs);
    handle_error("Failed to get process list");
    return;
  }

  process_count = size / sizeof(struct kinfo_proc);
  if (process_count > MAX_PROCESSES) process_count = MAX_PROCESSES;

  for (int i = 0; i < process_count; i++) {
    processes[i].pid = procs[i].kp_proc.p_pid;
    proc_name(procs[i].kp_proc.p_pid, processes[i].name, sizeof(processes[i].name));

    struct passwd *pw = getpwuid(procs[i].kp_eproc.e_ucred.cr_uid);
    if (pw) {
      processes[i].username = strdup(pw->pw_name);
      if (strcmp(pw->pw_name, "root") == 0) {
        processes[i].system_process = true;
      } else {
        processes[i].system_process = false;
      }
    } else {
      processes[i].username = strdup("Unknown");
      processes[i].system_process = false;
    }

#ifndef __linux__
    if (!dont_get_fancy) {
      // In macOS, processes with blank names are most likely services or system processes
      // Most of the time, the username is the service name (i think) but there are just processes with no name but 100%
      // they're owned by root
      if (strlen(processes[i].name) == 0) {
        processes[i].system_process = true;
        if (strcmp(processes[i].username, "root") != 0) {
          strncpy(processes[i].name, processes[i].username, sizeof(processes[i].name) - 1);
          processes[i].name[sizeof(processes[i].name) - 1] = '\0'; // Ensure null-termination
          processes[i].username = strdup("root");
        }
        // If the process is owned by root and has no name, then we cant really anymore information about it
      }

      // for processes that don't have a name, we can just use the binary path
      if (strlen(processes[i].name) == 0) {
        // Get the process's binary
        char path[PROC_PIDPATHINFO_MAXSIZE];
        int ret = proc_pidpath(procs[i].kp_proc.p_pid, path, sizeof(path));
        if (ret <= 0) {
          // If we can't get the path, then we cant get the name
          // so we skip unless PID is 0
          if (procs[i].kp_proc.p_pid == 0) { // Only kernel_task has PID 0
            strncpy(processes[i].name, "kernel_task", sizeof(processes[i].name) - 1);
            processes[i].name[sizeof(processes[i].name) - 1] = '\0'; // Ensure null-termination
          }

          continue;
        }
        // Get the name of the binary
        char *name = strrchr(path, '/');
        if (name) {
          name++;
          strncpy(processes[i].name, name, sizeof(processes[i].name) - 1);
          processes[i].name[sizeof(processes[i].name) - 1] = '\0'; // Ensure null-termination
        }
      }
    }
#endif
  }
  free(procs);
}

int process_pid(pid_t pid) {


  int attempts = 0;
  while (attempts < 500) {
    if (kill(pid, SIGKILL) == 0) {
      return 0;
    }
    if (kill(pid, SIGTERM) == 0) {
      return 0;
    }

    if (kill(pid, 0) == -1 && errno == ESRCH) {
      return 0;
    }

    attempts++;
  }

#ifdef __linux__
  snprintf(ERROR_MSG, sizeof(ERROR_MSG), "Failed to terminate process %d", pid);
  return 1;
#endif

#ifndef __linux__
  mach_port_t task;
  kern_return_t kr;

  kr = task_for_pid(mach_task_self(), pid, &task);
  if (kr != KERN_SUCCESS) {
    // This normally means we don't have permission to kill the process
    snprintf(ERROR_MSG, sizeof(ERROR_MSG), "Failed to get task port for process %d (normally means you dont have permission to kill the process)", pid);
    return 1;
  }

  kr = task_terminate(task);
  if (kr != KERN_SUCCESS) {
    snprintf(ERROR_MSG, sizeof(ERROR_MSG), "Failed to terminate process thru task_terminate %d", pid);
    return 1;
  }
#endif

  return 0;
}