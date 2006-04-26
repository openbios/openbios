#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <fccc.h>
#include <linklist.h>
#include <fccc-tools.h>

linked_list* current_scope;
linked_list* symboltable;
int scope_count;

void init_symboltable(void);
void destroy_symboltable(void);
symbol* insert(char* lexem_to_insert);
symbol* lookup(char* lexem_to_find);
symbol* lookup_local(char* lexem_to_find);
void new_scope(void);
void exit_scope(void);
void dump(void);

void dump_symbol(symbol* symbol_to_dump);


#endif /* SYMBOLTABLE_H */
