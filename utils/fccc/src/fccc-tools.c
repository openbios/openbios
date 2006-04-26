#include <fccc.h>
#include <fccc-tools.h>

int id_taller = 0;

void reverse(char *s)
{
	char c;
	int i, j;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

void itoa(int number, char *dest_str)
{
	int count, sign;

	if ((sign = number) < 0)
		number *= -1;
	count = 0;
	do {
		dest_str[count++] = number % 10 + '0';
	} while ((number /= 10) > 0);
	if (sign < 0)
		dest_str[count++] = '-';
	dest_str[count] = '\0';
	reverse(dest_str);
}

void generate_id(const char *name, char *dest_str)
{
	char temp_str[LEXEM_SIZE];

	itoa(id_taller++, dest_str);
	reverse(dest_str);
	strcat(dest_str, "_");
	strcpy(temp_str, name);
	temp_str[LEXEM_SIZE - strlen(dest_str) - 1] = '\0';
	reverse(temp_str);
	strcat(dest_str, temp_str);
	reverse(dest_str);
}

void error_found(void)
{
	extern int fccc_error;

	printf("error_found(), line %d\n", getlinenum());
	fccc_error = 1;
}

char *dot_c2dot_f(char *c_str)
{
	c_str[strlen(c_str) - 2] = '\0';
	strcat(c_str, ".f");

	return (c_str);
}

FILE *init_input_file(char *input_filename)
{
	FILE *tempfp;

	tempfp = fopen(input_filename, "r");
	if (!tempfp) {
		printf("Cannot open %s - bailing out!\n", input_filename);
		exit(1);
	}
	printf("%s open for input.\n", input_filename);
	return (tempfp);
}

FILE *init_output_file(char *output_filename)
{
	FILE *tempfp;

	tempfp = fopen(output_filename, "w");
	if (!tempfp) {
		printf("Cannot open %s - bailing out!\n", output_filename);
		exit(1);
	}
	printf("%s open for output.\n", output_filename);
	return (tempfp);
}
