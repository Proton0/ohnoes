#include "display.h"
#include "process.h"
#include "search.h"

int selected_index = 0;
int top_index = 0;

void display_processes() {
  clear();
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  attron(A_BOLD);
  mvprintw(0, 0, "%-8s %-30s %s %s", "PID", "NAME", "USER", "SYSTEM");
  attroff(A_BOLD);
  mvprintw(1, 0, "------------------------------------------------------------");

  int visible_rows = max_y - 3;

  if (selected_index < top_index) {
    top_index = selected_index;
  }
  if (selected_index >= top_index + visible_rows) {
    top_index = selected_index - visible_rows + 1;
  }

  int displayed_rows = 0;
  for (int i = 0; displayed_rows < visible_rows && (i + top_index) < process_count; i++) {
    int current = i + top_index;
    if (hide_system_processes && processes[current].system_process) {
      continue;
    }

    const char *system_symbol = processes[current].system_process ? "Y" : "N";
    const char *username = processes[current].username ? processes[current].username : "Unknown";
    if (no_username) {
      username = "    ";
    }
    int should_highlight = 0;
    if (current == selected_index) {
      should_highlight = 1;
    } else if (is_searching && strlen(search_term) > 0) {
      char pid_str[32];
      snprintf(pid_str, sizeof(pid_str), "%d", processes[current].pid);
      if (strcasestr_custom(processes[current].name, search_term) ||
          strcasestr_custom(pid_str, search_term)) {
        attron(A_BOLD);
      }
    }

    if (should_highlight) attron(A_REVERSE);

    mvprintw(displayed_rows + 2, 0, "%-8d %-30s %s %s",
             processes[current].pid,
             processes[current].name,
             username,
             system_symbol);

    if (should_highlight) attroff(A_REVERSE);
    if (is_searching && !should_highlight) attroff(A_BOLD);

    displayed_rows++;
  }

  if (is_searching) {
    mvprintw(max_y - 1, max_x - strlen(search_term) - 8, "Search: %s", search_term);
  }

  refresh();
}

void handle_error(const char* message) {
  if (ignore_errors) return;
  clear();
  mvprintw(0, 0, "Error: %s", message);
  refresh();
  sleep(2);
}