/* Compiler P2; Author: Wenzhao Zhang; UnityID: wzhang27 */

%option yylineno nodefault noyywrap
%{
#include <stdio.h>
#include <string.h>
#include "ice9.h"
#include "ice9.tab.h"

%}

%%

if { return TK_IF; } 
fi { return TK_FI; }
else { return TK_ELSE; }
do { return TK_DO; }
od { return TK_OD; }
fa { return TK_FA; }
af { return TK_AF; }
to { return TK_TO; }
proc { return TK_PROC; }
end { return TK_END; }
"return" { return TK_RETURN; }
forward { return TK_FORWARD; }
var { return TK_VAR; }
type { return TK_TYPE; }
break { return TK_BREAK; }
exit { return TK_EXIT; }
true { return TK_TRUE; }
false { return TK_FALSE; }
write { return TK_WRITE; }
writes { return TK_WRITES; }
read { return TK_READ; }
"[]" { return TK_BOX; }
-> { return TK_ARROW; }
"(" { return yytext[0]; }
")" { return yytext[0]; }
"[" { return yytext[0]; }
"]" { return yytext[0]; }
: { return yytext[0]; }
; { return yytext[0]; }
:= { return TK_ASSIGN; }
"?" { return yytext[0]; }
, { return yytext[0]; }
"+" { return yytext[0]; }
- { return yytext[0]; }
"*" { return yytext[0]; }
"/" { return yytext[0]; }
% { return yytext[0]; }
= { return yytext[0]; }
!= { return TK_NEQ; }
">" { return yytext[0]; }
"<" { return yytext[0]; }
>= { return TK_GE; }
"<=" { return TK_LE; }
\"[^"\n]*\" |
'[^'\n]*' { 
	yylval.str = strdup(yytext); 
	return TK_SLIT; }
[0-9]+ { 
	yylval.intt = atof(yytext); 
	return TK_INT; }
[A-Za-z][A-Za-z0-9_]* { 
	yylval.str = strdup(yytext); 
	return TK_ID; }

#.*  {} /* comment is ignored; --> #[^\n]*(\n|EOL) {} */
\n {}
[\t ]+ {}

. { fprintf(stderr, "line %d: illegal character %s\n", yylineno, yytext); exit(1); }

%%


