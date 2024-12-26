#include "process.h"
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <libproc.h>
#include <sys/sysctl.h>

#elif defined(__linux__)
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#endif

#include "display.h"

// External variables
struct ProcessInfo processes[MAX_PROCESSES];
int process_count = 0;
char ERROR_MSG[256];
bool no_username = false;
bool dont_get_fancy = false;
bool ignore_errors = false;
bool hide_system_processes = false;

#ifdef __linux__
static char* get_process_name_linux(pid_t pid) {
    char path[256];
    char buffer[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    FILE* fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
            fclose(fp);
            return strdup(buffer);
        }
        fclose(fp);
    }

    // Fallback to cmdline if comm is not available
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            fclose(fp);
            return strdup(buffer);
        }
        fclose(fp);
    }

    return strdup("Unknown");
}

static uid_t get_process_uid_linux(pid_t pid) {
    char path[256];
    struct stat st;

    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (stat(path, &st) == 0) {
        return st.st_uid;
    }
    return -1;
}

void get_processes() {
    DIR* proc_dir;
    struct dirent* entry;
    process_count = 0;

    proc_dir = opendir("/proc");
    if (!proc_dir) {
        handle_error("Failed to open /proc directory");
        return;
    }

    while ((entry = readdir(proc_dir)) != NULL && process_count < MAX_PROCESSES) {
        // Check if the entry is a process (directory with numeric name)
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            uid_t uid = get_process_uid_linux(pid);

            if (uid != -1) {
                processes[process_count].pid = pid;
                char* name = get_process_name_linux(pid);
                strncpy(processes[process_count].name, name, sizeof(processes[process_count].name) - 1);
                processes[process_count].name[sizeof(processes[process_count].name) - 1] = '\0';
                free(name);

                struct passwd* pw = getpwuid(uid);
                if (pw && !no_username) {
                    processes[process_count].username = strdup(pw->pw_name);
                    processes[process_count].system_process = (strcmp(pw->pw_name, "root") == 0);
                } else {
                    processes[process_count].username = strdup("Unknown");
                    processes[process_count].system_process = false;
                }

                process_count++;
            }
        }
    }

    closedir(proc_dir);
}

#elif defined(__APPLE__)

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

  process_count = (int) size / sizeof(struct kinfo_proc);
  if (process_count > MAX_PROCESSES) process_count = MAX_PROCESSES;

  for (int i = 0; i < process_count; i++) {
    processes[i].pid = procs[i].kp_proc.p_pid;
    proc_name(procs[i].kp_proc.p_pid, processes[i].name, sizeof(processes[i].name));

    if (!no_username) {
      struct passwd const *pw = getpwuid(procs[i].kp_eproc.e_ucred.cr_uid);
      if (pw) {
        processes[i].username = strdup(pw->pw_name);
        processes[i].system_process = (strcmp(pw->pw_name, "root") == 0);
      } else {
        processes[i].username = strdup("Unknown");
        processes[i].system_process = false;
      }
    } else {
      processes[i].username = strdup("Unknown");
      processes[i].system_process = false;
    }

    if (!dont_get_fancy && strlen(processes[i].name) == 0) {
      if (processes[i].pid == 0) {
        strncpy(processes[i].name, "kernel_task", sizeof(processes[i].name) - 1);
        processes[i].name[sizeof(processes[i].name) - 1] = '\0';
        continue;
      }

      char path[PROC_PIDPATHINFO_MAXSIZE];
      int ret = proc_pidpath(procs[i].kp_proc.p_pid, path, sizeof(path));
      if (ret > 0) {
        char *name = strrchr(path, '/');
        if (name) {
          name++;
          strncpy(processes[i].name, name, sizeof(processes[i].name) - 1);
          processes[i].name[sizeof(processes[i].name) - 1] = '\0';
        }
      }
    }
  }

  free(procs);
}

#endif

int terminate_pid(pid_t pid) {
  // First attempt: SIGTERM
  if (kill(pid, SIGTERM) == 0) {
    usleep(100000); // 100ms grace period

    if (kill(pid, 0) == -1 && errno == ESRCH) {
      return 0; // Process terminated successfully
    }
  }

  // Second attempt: SIGKILL
  if (kill(pid, SIGKILL) == 0) {
    usleep(50000); // 50ms wait
    if (kill(pid, 0) == -1 && errno == ESRCH) {
      return 0;
    }
  }

  // Platform-specific last resort
#ifdef __APPLE__
  mach_port_t task;
  kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

  if (kr == KERN_SUCCESS) {
    kr = task_terminate(task);
    mach_port_deallocate(mach_task_self(), task);

    if (kr == KERN_SUCCESS) {
      return 0;
    }
  }

  snprintf(ERROR_MSG, sizeof(ERROR_MSG),
           "Failed to terminate process %d via task_terminate", pid);
#else
  char proc_path[32];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);

    if (access(proc_path, F_OK) == -1) {
        return 0; // Process no longer exists
    }

    snprintf(ERROR_MSG, sizeof(ERROR_MSG),
             "Failed to terminate process %d after all attempts", pid);
#endif

  return 1; // Failed to terminate
}