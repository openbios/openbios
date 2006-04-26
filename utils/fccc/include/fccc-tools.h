#ifndef FCCCTOOLS_H
#define FCCCTOOLS_H

void reverse(char *);
void itoa(int number, char *);
void generate_id(const char *, char *);
void error_found(void); /* Function to set the global error flag */
char *dot_c2dot_f(char *);
FILE *init_output_file(char *);
FILE *init_input_file(char *);

#endif /* FCCCTOOLS_H */
