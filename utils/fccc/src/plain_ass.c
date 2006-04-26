/* plain_ass.c -- mmj@mmj.dk.
 * Code to handle Plain Assignments */

#include <parserfunctions.h>

YYSTYPE plain_ass(YYSTYPE _ID, YYSTYPE expr)
{
	symbol *tmpsym;
	YYSTYPE retval;

	/* First check for errors, return if any.
	 * FIXME: not complete yet, 10202001 mmj */
	
	if(expr.var_type == ERROR_T) {
		retval.var_type=ERROR_T;
		return(retval);
	} 
	tmpsym=lookup(_ID.text);
	if(!tmpsym) {
		printf("Parser, %d: '%s' undefined\n", getlinenum(), _ID.text );
		retval.var_type = ERROR_T;
		error_found();
		return(retval);
	}
	_ID.symbol_ref = tmpsym;
	if(tmpsym->type != expr.var_type && tmpsym->type != ERROR_T) {
		printf("Parser, %d: Wrong type for assignment.", getlinenum());
		retval.var_type = ERROR_T;
		error_found();
		return(retval);
	}

	/* Should not get here unless valid code */
	
	P_DEBUG(printf("Parser, %d: Assignment found\n", getlinenum()));
	retval.num=expr.num;
	P_DEBUG(printf("Parser, %d: %s = %d\n", getlinenum(),
			retval.text, retval.num));
	
	return(retval);
}
