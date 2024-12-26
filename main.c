#include "process.h"
#include "search.h"
#include "display.h"



void help() {
  printf("Usage: ohnoes [-i|--ignore] [-h|--help]\n");
  printf("  -i, --ignore  Ignore errors\n");
  printf("  -h, --help    Display this help message\n");
  printf("  -n  --nofancy Disables the methods that ohnoes uses to get more details about the process (macos only)\n");
  printf("  -u  --nouser  Makes the username blank\n");
  printf("  -s  --system  Hides system processes\n");
  printf("---------------------------------------------\n");
  printf("  Press / to search\n");
  printf("  Press n to find the next match\n");
  printf("  Press N to find the previous match\n");
  printf("  Press Enter to kill the selected process\n");
  printf("  Press q to quit\n");
  printf("---------------------------------------------\n");
  printf("  Press 1 to toggle username\n");
  printf("  Press 2 to toggle system processes\n");
  printf("  Press 3 to toggle fancy mode\n");
  printf("  Press 4 to toggle error ignoring\n");
  printf("---------------------------------------------\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    switch (argv[i][1]) {
      case 's':
        hide_system_processes = true;
        break;
      case 'u':
        no_username = true;
        break;
      case 'n':
        dont_get_fancy = true;
        break;
      case 'i':
        ignore_errors = true;
        break;
      case 'h':
        help();
        break;
      default:
        if (strcmp(argv[i], "--system") == 0) {
          hide_system_processes = true;
        } else if (strcmp(argv[i], "--nouser") == 0) {
          no_username = true;
        } else if (strcmp(argv[i], "--nofancy") == 0) {
          dont_get_fancy = true;
        } else if (strcmp(argv[i], "--ignore") == 0) {
          ignore_errors = true;
        } else if (strcmp(argv[i], "--help") == 0) {
          help();
        }
        break;
    }
  }


  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  timeout(500);

  get_processes();

  int ch;

  while (1) { // Main loop
    displayProcesses();
    ch = getch();

    switch (ch) {
      case KEY_UP:
        if (selected_index > 0) selected_index--;
        break;

      case KEY_DOWN:
        if (selected_index < process_count - 1) selected_index++;
        break;

      case KEY_PPAGE:
        selected_index = (selected_index > 10) ? selected_index - 10 : 0;
        break;

      case KEY_NPAGE:
        selected_index = (selected_index < process_count - 11) ?
                         selected_index + 10 : process_count - 1;
        break;

      case '/':
        handle_search();
        break;

      case '1':
        no_username = !no_username;
        break;

      case '2':
        hide_system_processes = !hide_system_processes;
        break;

      case '3':
        dont_get_fancy = !dont_get_fancy;
        break;

      case '4':
        ignore_errors = !ignore_errors;
        break;

      case 'n':
        if (is_searching) {
          int next_match = find_next_match(selected_index, 1);
          if (next_match != -1) selected_index = next_match;
        }
        break;

      case 'N':
        if (is_searching) {
          int prev_match = find_next_match(selected_index, -1);
          if (prev_match != -1) selected_index = prev_match;
        }
        break;

      case 10:
        displayProcesses();
        refresh();

        int result = terminate_pid(processes[selected_index].pid);
        if (result == 1) {
          handle_error(ERROR_MSG);
        } else if (result == 0) {
          get_processes();
        }
        break;

      case 'q':
        endwin();
        return 0;

      default:
        get_processes();
        displayProcesses();
        refresh();
        break;
    }

  }
}
