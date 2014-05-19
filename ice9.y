/* Compiler P2; Author: Wenzhao Zhang; UnityID: wzhang27 */

/*
changes from P1:
1. eliminate lvalue, flatten it.
have to rewrite assignment and exp.
*/

%{

#include <stdio.h>
#include <stdlib.h>
#include "ice9.h"

extern char *yytext;
void print_ast();
void semantic_check_ast();
void code_generation(char * fname);

int yylex(void); /* function prototype */

void yyerror(char *s)
{
  fprintf(stderr, "line %d: syntax error near %s\n", yylineno, yytext);
  
  exit(0);
}

%}

%union {
  int intt;
  char *str;

  _ast_stmt_declaration * ast_stmt_declaration;
  _ast_stmt * ast_stmt;

  _ast_type * ast_type;
  _ast_typeid * ast_typeid;
  _ast_forward * ast_forward;
  _ast_proc * ast_proc;
  _ast_exp * ast_exp;
  _ast_assign * ast_assign;
  _ast_write * ast_write;
  _ast_do * ast_do;  
  _ast_fa * ast_fa;
  _ast_if * ast_if;
  _ast_sub_proc_decl * ast_sub_proc_decl;
  _ast_proc_decl * ast_proc_decl;

  _ast_stmt_set * ast_stmt_set;
  _ast_box_stmt * ast_box_stmt;
  _ast_box_stmt_set * ast_box_stmt_set;
}

%token TK_IF
%token TK_FI
%token TK_ELSE
%token TK_DO
%token TK_OD
%token TK_FA
%token TK_AF
%token TK_TO
%token TK_PROC
%token TK_END
%token TK_RETURN
%token TK_FORWARD
%token TK_VAR
%token TK_TYPE
%token TK_BREAK
%token TK_EXIT
%token TK_TRUE
%token TK_FALSE
%token TK_WRITE
%token TK_WRITES
%token TK_READ
%token TK_BOX
%token TK_ARROW
%token TK_LPAREN
%token TK_RPAREN
%token TK_LBRACK
%token TK_RBRACK
%token TK_COLON
%token TK_SEMI
%token TK_ASSIGN
%token TK_QUEST
%token TK_COMMA
%token TK_PLUS
%token TK_MINUS
%token TK_STAR
%token TK_SLASH
%token TK_MOD
%token TK_EQ
%token TK_NEQ
%token TK_GT
%token TK_LT
%token TK_GE
%token TK_LE

%token <intt> TK_INT
%token <str> TK_ID
%token <str> TK_SLIT

%nonassoc '=' TK_NEQ '>' '<' TK_GE TK_LE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS '?'
/* %right '?' */

/* for non-terminals */
%type <ast_stmt_declaration> stmt_declaration
%type <ast_stmt> stmt
%type <ast_type> type
%type <ast_typeid> typeid
%type <ast_forward> forward
%type <ast_proc> proc
%type <ast_exp> exp
%type <ast_assign> ass_stmt
%type <ast_write> wrt_stmt
%type <ast_do> do
%type <ast_fa> fa
%type <ast_if> if
%type <ast_sub_proc_decl> sub_proc_declaration
%type <ast_proc_decl> proc_declaration

%type <ast_stmt_set> stmt_set
%type <ast_box_stmt> box_stmt
%type <ast_box_stmt_set> box_stmt_set

%start program

%%

program: 
	|	stmt_declaration_list 
	|	stmt_list
	|	stmt_declaration_list stmt_list
	;

stmt_list:	stmt {
	append_stmt_2root($1);
}
	|	stmt_list stmt {   /* recursion */
	append_stmt_2root($2);
}
	;

stmt_declaration_list:	stmt_declaration {
	append_stmt_decl_2root($1);
}
	| stmt_declaration_list stmt_declaration {   /* recursion */
	append_stmt_decl_2root($2);
}	
	;

stmt_declaration:	var {
	$$ = na_stmt_decl_var(yylineno);
}
	|	type {
	$$ = na_stmt_decl_type($1, yylineno);
}
	|	forward {
	$$ = na_stmt_decl_forward($1, yylineno);
}	
	|	proc {
	$$ = na_stmt_decl_proc($1, yylineno);
}
	;

stmt:	if {
	$$ = na_stmt_if($1, yylineno);
}	
	|	do {
	$$ = na_stmt_do($1, yylineno);
}
	|	fa {
	$$ = na_stmt_fa($1, yylineno);
}
	|	ass_stmt {
	$$ = na_stmt_assign($1, yylineno);
}
	|	wrt_stmt {
	$$ = na_stmt_write($1, yylineno);
}
	|	exp ';' {
	$$ = na_stmt_exp($1, yylineno);
}
	|	TK_BREAK ';' {
	$$ = na_stmt_break(yylineno);
}
	|	TK_EXIT ';' {
	$$ = na_stmt_exit(yylineno);
}
	|	TK_RETURN ';' {
	$$ = na_stmt_return(yylineno);
}
	|	';' { 
	$$ = na_stmt_empty(yylineno);
}
	;

/* rewrite ass_stmt to eliminate lvalue; and cannot handle sub_exparr, in case a[exp][exp] = b[exp][exp];
this kind of writing is a bug, cannot handle array with more dimension */
ass_stmt:	TK_ID TK_ASSIGN exp ';' {
	$$ = na_assign($1, NULL, $3, yylineno);
}
	|	TK_ID '[' exp ']' sub_exparr/*should comment this sub_exparr*/ TK_ASSIGN exp ';' {
	$$ = na_assign($1, $3, $7, yylineno);
}
	|	TK_ID '[' exp ']' '[' exp ']' TK_ASSIGN exp ';' {
	$$ = na_assign2($1, $3, $6, $9, yylineno);
}
	|	TK_ID '[' exp ']' '[' exp ']' '[' exp ']' TK_ASSIGN exp ';' {
	$$ = na_assign3($1, $3, $6, $9, $12, yylineno);
}
	|	TK_ID '[' exp ']' '[' exp ']' '[' exp ']' '[' exp ']' TK_ASSIGN exp ';' {
	$$ = na_assign4($1, $3, $6, $9, $12, $15, yylineno);
}
	;

wrt_stmt:	TK_WRITE exp ';' {
	$$ = na_write(1, $2, yylineno);
}
	|	TK_WRITES exp ';' {
	$$ = na_write(0, $2, yylineno);
}
	;

if:	TK_IF exp TK_ARROW stmt_set TK_FI {
	$$ = na_if($2, $4, NULL, NULL, yylineno);
}
	|	TK_IF exp TK_ARROW stmt_set box_stmt_set TK_FI {
	$$ = na_if($2, $4, $5, NULL, yylineno);
}
	|	TK_IF exp TK_ARROW stmt_set TK_BOX TK_ELSE TK_ARROW stmt_set TK_FI {
	$$ = na_if($2, $4, NULL, $8, yylineno);
}
	|	TK_IF exp TK_ARROW stmt_set box_stmt_set TK_BOX TK_ELSE TK_ARROW stmt_set TK_FI {
	$$ = na_if($2, $4, $5, $9, yylineno);
}
	;

stmt_set:	stmt {
	$$ = na_stmt_set(NULL, $1, yylineno);
}
	|	stmt_set stmt {  /* recursion */
	$$ = na_stmt_set($1, $2, yylineno);
}
	;

box_stmt_set:	box_stmt {
	$$ = na_box_stmt_set(NULL, $1, yylineno);
}
	|	box_stmt_set box_stmt {  /* recursion */
	$$ = na_box_stmt_set($1, $2, yylineno);
}
	;

box_stmt:	TK_BOX exp TK_ARROW stmt_set {
	$$ = na_box_stmt($2, $4, yylineno);
}
	;

do:	TK_DO exp TK_ARROW TK_OD {
	$$ = na_do($2, NULL, yylineno);
}
	| TK_DO exp TK_ARROW stmt_set TK_OD {
	$$ = na_do($2, $4, yylineno);
}
	;

fa:	TK_FA TK_ID TK_ASSIGN exp TK_TO exp TK_ARROW TK_AF {
	$$ = na_fa($2, $4, $6, NULL, yylineno);
}
	|	TK_FA TK_ID TK_ASSIGN exp TK_TO exp TK_ARROW stmt_set TK_AF {
	$$ = na_fa($2, $4, $6, $8, yylineno);
}
	;

proc:	TK_PROC TK_ID '(' declist ')' TK_END {
	$$ = na_proc($2, NULL, NULL, NULL, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' proc_declaration TK_END {
	$$ = na_proc($2, NULL, $6, NULL, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' stmt_set TK_END {
	$$ = na_proc($2, NULL, NULL, $6, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' proc_declaration stmt_set TK_END {
	$$ = na_proc($2, NULL, $6, $7, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' ':' typeid TK_END {
	$$ = na_proc($2, $7, NULL, NULL, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' ':' typeid proc_declaration TK_END {
	$$ = na_proc($2, $7, $8, NULL, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' ':' typeid stmt_set TK_END {
	$$ = na_proc($2, $7, NULL, $8, yylineno);
}
	|	TK_PROC TK_ID '(' declist ')' ':' typeid proc_declaration stmt_set TK_END {
	$$ = na_proc($2, $7, $8, $9, yylineno);
}
	;

idlist:	TK_ID sub_idlist {
	na_idlist($1, yylineno);
}
	;

sub_idlist:	
	|	',' TK_ID sub_idlist {   /* recursion */
	na_idlist($2, yylineno);
}
	;

declist:	
	|	declistx
	;

var:	TK_VAR sub_sub_varlist sub_varlist ';'
	;


sub_varlist:	
	|	',' sub_sub_varlist sub_varlist  
	;

sub_sub_varlist:	idlist ':' typeid {
	na_subvar($3, yylineno);
}
	|	idlist ':' typeid array {
	na_subvar($3, yylineno);
}
	;


/* left-recursion; final returned _ast_array will be the first dimension element; */
array:	 '[' TK_INT ']' {
	na_array($2, yylineno);
}
	| array '[' TK_INT ']' {   /* recursion */
	na_array($3, yylineno);
}
	;

declistx:	sub_sub_declistx sub_declistx
	;

sub_declistx:
	|	',' sub_sub_declistx sub_declistx   /* recursion */
	;

sub_sub_declistx: idlist ':' typeid {
	na_subdelist($3, yylineno);
}
	;

proc_declaration:	sub_proc_declaration {
	$$ = na_proc_decl(NULL, $1, yylineno);
}
	| proc_declaration sub_proc_declaration {  /* recursion */
	$$ = na_proc_decl($1, $2, yylineno);
}
	;

sub_proc_declaration:	type {
	$$ = na_sub_proc_decl(AST_NT_TYPE, $1, yylineno);
}
	|	var {
	$$ = na_sub_proc_decl(AST_NT_VAR, NULL, yylineno);
}
	;

forward:	TK_FORWARD TK_ID '(' declist ')' ';' {
	$$ = na_forward($2, NULL, yylineno);
}
	|	TK_FORWARD TK_ID '(' declist ')' ':' typeid ';' {
	$$ = na_forward($2, $7, yylineno);
}
	;

type:	TK_TYPE TK_ID '=' typeid ';' {
	$$ = na_type($2, $4, yylineno);
}
	|	TK_TYPE TK_ID '=' typeid array ';' {
	$$ = na_type($2, $4, yylineno);
}
	;

typeid:	TK_ID {
	$$ = na_typeid($1, yylineno);
}
	;

/*
lvalue:	TK_ID {
}
	| lvalue '[' exp ']' {   
} */
	;

/* na_exp(int ntype, int ivalue, char * svalue, char * id, _ast_exp * lexp, _ast_exp * rexp, int lineno) */
exp:	/*lvalue {
	$$ = na_exp(AST_NT_LVALUE, 0, NULL, NULL, NULL, NULL, yylineno);
	}*/
	/* rewritten lvalue, cannot handle this exp-lvalue combined recursion */
	TK_ID {
	$$ = na_exp(AST_NT_LVALUE, -1, NULL, $1, NULL, NULL, yylineno);
}
	|	TK_ID '[' exp ']' sub_exparr {
	$$ = na_exp(AST_NT_LVALUE, -1, NULL, $1, $3, NULL, yylineno);
} /* end rewritten lvalue */
	|	TK_INT {
	$$ = na_exp(AST_NT_INT, $1, NULL, NULL, NULL, NULL, yylineno);
}
	|	TK_TRUE {
	$$ = na_exp(AST_NT_TRUE, -1, NULL, NULL, NULL, NULL, yylineno);
}
	|	TK_FALSE {
	$$ = na_exp(AST_NT_FALSE, -1, NULL, NULL, NULL, NULL, yylineno);
}
	|	TK_SLIT {
	$$ = na_exp(AST_NT_STR, -1, $1, NULL, NULL, NULL, yylineno);
}
	|	TK_READ {
	$$ = na_exp(AST_NT_READ, -1, NULL, NULL, NULL, NULL, yylineno);
}
	|	'-' exp %prec UMINUS {
	$$ = na_exp(AST_NT_UMINUS, -1, NULL, NULL, NULL, $2, yylineno);
}
	|	'?' exp %prec '?' {
	$$ = na_exp(AST_NT_QUEST, -1, NULL, NULL, NULL, $2, yylineno);
}
	|	TK_ID '(' ')' {
	$$ = na_exp(AST_NT_EPROC, -1, NULL, $1, NULL, NULL, yylineno);
}
	|	TK_ID '(' exp sub_exp ')' {
	$$ = na_exp(AST_NT_EPROC, -1, NULL, $1, $3, NULL, yylineno);
}
	|	exp '+' exp {
	$$ = na_exp(AST_NT_PLUS, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '-' exp {
	$$ = na_exp(AST_NT_MINUS, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '*' exp {
	$$ = na_exp(AST_NT_MUL, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '/' exp {
	$$ = na_exp(AST_NT_DIV, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '%' exp {
	$$ = na_exp(AST_NT_MOD, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '=' exp {
	$$ = na_exp(AST_NT_EQU, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp TK_NEQ exp {
	$$ = na_exp(AST_NT_NE, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '>' exp {
	$$ = na_exp(AST_NT_GT, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp '<' exp {
	$$ = na_exp(AST_NT_LT, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp TK_GE exp {
	$$ = na_exp(AST_NT_GE, -1, NULL, NULL, $1, $3, yylineno);
}
	|	exp TK_LE exp {
	$$ = na_exp(AST_NT_LE, -1, NULL, NULL, $1, $3, yylineno);
}
	|	'(' exp ')' {
	$$ = na_exp(AST_NT_PAREN, -1, NULL, NULL, $2, NULL, yylineno);
}
	;

sub_exp:
	| ',' exp sub_exp {
	append_exp($2);
}

sub_exparr:
	| '[' exp ']' sub_exparr {
	append_exparr($2);
}
	;	


%%

int main(int argc, char ** argv) {
	ice9_init();
	yyparse();
	//print_ast();
	
	semantic_check_ast();
	
	code_generation(argv[1]);
}
