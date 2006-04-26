#include <symboltable.h>

void init_symboltable(void)
{
	scope_count = -1;
	symboltable = new_list();
	new_scope();
}

void destroy_symboltable(void)
{
	destroy_list(symboltable);
}

symbol *insert(char *lexem_to_insert)
{
	symbol *new_symbol;

	new_symbol = (symbol *) malloc(sizeof(symbol));
	strcpy(new_symbol->lexem, lexem_to_insert);
	generate_id(lexem_to_insert, new_symbol->unique_id);
	new_symbol->scope_no = scope_count;
	new_first_elem(current_scope);
	current_scope->first->data = (void *) new_symbol;
	return new_symbol;
}

symbol *lookup(char *lexem_to_find)
{
	linked_list *current_scope_save;
	symbol *temp_symbol;

	symboltable->loop = symboltable->first;
	current_scope_save = current_scope;
	while (current_scope != 0) {
		if ((temp_symbol = lookup_local(lexem_to_find)) != 0) {
			current_scope = current_scope_save;
			return temp_symbol;
		}
		if (symboltable->loop->next == 0)
			current_scope = 0;
		else {
			symboltable->loop = symboltable->loop->next;
			current_scope =
			    (linked_list *) (symboltable->loop->data);
		}
	}
	current_scope = current_scope_save;
	return 0;
}

symbol *lookup_local(char *lexem_to_find)
{
	current_scope->loop = current_scope->first;
	while (current_scope->loop != 0) {
		if (!strcmp
		    ((char *) ((symbol *) (current_scope->loop->data))->
		     lexem, lexem_to_find))
			return (symbol *) current_scope->loop->data;
		current_scope->loop = current_scope->loop->next;
	}
	return 0;
}

void new_scope(void)
{
	linked_list *temp_scope;

	temp_scope = new_list();
	new_first_elem(symboltable);
	++scope_count;
	symboltable->first->data = (void *) temp_scope;
	current_scope = temp_scope;
}

void exit_scope(void)
{
	remove_first_elem(symboltable);
	--scope_count;
	current_scope = (linked_list *) symboltable->first->data;
}

void dump_symbol(symbol * symbol_to_dump)
{
	printf("Lexem              : %s\n", symbol_to_dump->lexem);
	printf("  Unique ID        : %s\n", symbol_to_dump->unique_id);
	printf("  Scope number     : %d\n", symbol_to_dump->scope_no);
}

void dump(void)
{
	linked_list *temp_scope;

	printf("Dumping... ==========> \n");
	symboltable->loop = symboltable->first;
	while (symboltable->loop != 0) {
		temp_scope = (linked_list *) symboltable->loop->data;
		temp_scope->loop = temp_scope->first;
		while (temp_scope->loop != 0) {
			dump_symbol((symbol *) temp_scope->loop->data);
			temp_scope->loop = temp_scope->loop->next;
		}
		symboltable->loop = symboltable->loop->next;
	}
}
