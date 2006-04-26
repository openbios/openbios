#include <linklist.h>

linked_list *new_list()
{
	linked_list *ll = (linked_list *) malloc(sizeof(linked_list));

	ll->first = 0;
	ll->loop = 0;
	return ll;
}

void new_first_elem(linked_list * list)
{
	list_entry *ne = (list_entry *) malloc(sizeof(list_entry));

	ne->data = 0;
	ne->next = list->first;
	list->first = ne;
}

void remove_first_elem(linked_list * list)
{
	list_entry *del = list->first;

	if (del != 0) {
		if (del->data != 0)
			free(del->data);
		list->first = del->next;
		free(del);
	}
}

void destroy_list(linked_list * list)
{
	list_entry *del;

	while (list->first != 0) {
		del = list->first;
		list->first = del->next;
		if (del->data != 0)
			free(del->data);
		free(del);
	}
	free(list);
}
