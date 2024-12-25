#ifndef SEARCH_H
#define SEARCH_H

#define SEARCH_MAX_LEN 64

void handle_search();
int find_next_match(int start_index, int direction);
void clear_search();
int strcasestr_custom(const char *haystack, const char *needle);

extern char search_term[SEARCH_MAX_LEN];
extern int is_searching;

#endif