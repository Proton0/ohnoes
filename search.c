#include "search.h"
#include "process.h"
#include "display.h"

char search_term[SEARCH_MAX_LEN] = "";
int is_searching = 0;

void handle_search() {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  move(max_y - 1, 0);
  clrtoeol();

  mvprintw(max_y - 1, 0, "/");
  refresh();

  echo();

  int ch;
  int pos = 0;
  memset(search_term, 0, SEARCH_MAX_LEN);

  while ((ch = getch()) != ERR) {
    if (ch == 10) {
      break;
    } else if (ch == 27) {
      clear_search();
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127) {
      if (pos > 0) {
        search_term[--pos] = '\0';
        mvprintw(max_y - 1, 0, "/%-*s", SEARCH_MAX_LEN, search_term);
        move(max_y - 1, pos + 1);
      }
    } else if (pos < SEARCH_MAX_LEN - 1 && isprint(ch)) {
      search_term[pos++] = ch;
      search_term[pos] = '\0';
    }
    refresh();
  }


  noecho();

  if (strlen(search_term) > 0) {
    is_searching = 1;

    int next_match = find_next_match(selected_index, 1);
    if (next_match != -1) {
      selected_index = next_match;
    }
  }
}

void clear_search() {
  memset(search_term, 0, SEARCH_MAX_LEN);
  is_searching = 0;
}

int find_next_match(int start_index, int direction) {
  if (strlen(search_term) == 0) return -1;

  int current = start_index;
  int count = 0;

  while (count < process_count) {
    current += direction;
    if (current >= process_count) current = 0;
    if (current < 0) current = process_count - 1;

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", processes[current].pid);


    if (strcasestr_custom(processes[current].name, search_term) ||
        strcasestr_custom(pid_str, search_term)) {
      return current;
    }

    count++;
  }

  return -1;
}

int strcasestr_custom(const char *haystack, const char *needle) {
  char *h = strdup(haystack);
  char *n = strdup(needle);
  for (int i = 0; h[i]; i++) h[i] = tolower(h[i]);
  for (int i = 0; n[i]; i++) n[i] = tolower(n[i]);
  int result = strstr(h, n) != NULL;
  free(h);
  free(n);
  return result;
}