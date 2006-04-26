#ifndef LINKLIST_H
#define LINKLIST_H

#include <stdio.h>
#include <stdlib.h>

struct tag_list_entry {
	void *data;
	struct tag_list_entry *next;
};
typedef struct tag_list_entry list_entry;

struct tag_linked_list {
	list_entry *first;
	list_entry *loop;
};
typedef struct tag_linked_list linked_list;

linked_list *new_list(void);
void remove_first_elem(linked_list * list);
void new_first_elem(linked_list * list);
void destroy_list(linked_list * list);

#endif /* LINKLIST_H */
