/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * Header.
 */

#ifndef ICE9_H_
#define ICE9_H_

#include "ice9_util.h"
#include "ice9_log.h"

extern int yylineno;  /* for .y file */

#define ICE9_DEBUG 0

/* General purpose registers, only for temp use */
#define REG_T1 0
#define REG_T2 1
/* define some dedicated registers */
#define REG_ZERO 2
#define REG_FP 3
#define REG_AC1 4
#define REG_AC2 5
#define REG_SP 6
#define REG_PC 7

/*for CG*/
#define ICE9_BOOL_TRUE 1
#define ICE9_BOOL_FALSE 0

/* to assist CG of break in fa/do */
#define BREAK_IGNORE -2
#define BREAK_INIT -1
#define BREAK_GENERATED 0

/* to assist CG of return in proc
#define RETURN_IGNORE -2
#define RETURN_INIT -1
#define RETURN_GENERATED 0*/

/* stmt_declaration types */
#define AST_NT_VAR 1 /* var declaration */
#define AST_NT_TYPE 2 /* type declaration */
#define AST_NT_FORWARD 3 /* forward declaration */
#define AST_NT_PROC 4 /* type declaration */

/* stmt types */
#define AST_NT_BREAK 5 /* terminal */
#define AST_NT_EXIT 6 /* terminal */
#define AST_NT_RETURN 7 /* terminal */
#define AST_NT_IF 8
#define AST_NT_DO 9
#define AST_NT_FA 10
#define AST_NT_ASSIGN 11
#define AST_NT_WRITE 12
#define AST_NT_EXP 13
#define AST_NT_EMPTY 35

/* exp types */
#define AST_NT_LVALUE 14
#define AST_NT_INT 15
#define AST_NT_TRUE 16
#define AST_NT_FALSE 17
#define AST_NT_STR 18
#define AST_NT_READ 19
#define AST_NT_UMINUS 20 /* '-' exp, unary minus */
#define AST_NT_QUEST 21 /* '?' exp */
#define AST_NT_EPROC 22 /* proc call */
#define AST_NT_PLUS 23
#define AST_NT_MINUS 24
#define AST_NT_MUL 25
#define AST_NT_DIV 26
#define AST_NT_MOD 27
#define AST_NT_EQU 28
#define AST_NT_NE 29
#define AST_NT_GT 30
#define AST_NT_LT 31
#define AST_NT_GE 32
#define AST_NT_LE 33
#define AST_NT_PAREN 34 /* '(' exp ')' */

#define STR_BUFFER_SIZE 500
#define ARR_MAX_DIMENSION 20

/*semantic types*/
#define S_TYPE_UNKNOWN 40
#define S_TYPE_INT_BASIC 41
#define S_TYPE_STRING_BASIC 42
#define S_TYPE_BOOL_BASIC 43
#define S_TYPE_INT_ARR 44  /* var a: int[1][2]; a[1] --> S_TYPE_INT_ARR*/
#define S_TYPE_STRING_ARR 45
#define S_TYPE_BOOL_ARR 46





/* ----------start define AST structs---------- */
typedef struct _ast_idlist_st{
	int lineno;
	char * id;
	struct _ast_idlist_st * next;
}_ast_idlist;


typedef struct _ast_typeid_st{
	int lineno;
	char * id;
}_ast_typeid;

typedef struct _ast_array_st{
	int lineno;
	int demension;

	struct _ast_array_st * next;
}_ast_array;

/*
 * in the stmt "forward test (a,b,c : int , e : float);"
 * "a,b,c : int" is one _ast_subdeclist, "e : float" is another _ast_subdeclist;
 * there is no struct to represent the entire dec-list
 */
typedef struct _ast_subdeclist_st{
	int lineno;
	_ast_typeid * typeid; /* variables' type, int/bool/string */
	_ast_idlist * idl;

	struct _ast_subdeclist_st * next;
}_ast_subdeclist;

typedef struct _ast_sub_proc_decl_st{
	int ntype; /* type or var */
	int lineno;
	struct _ast_type_st * type;
	struct _ast_subvar_st * var;
}_ast_sub_proc_decl;

typedef struct _ast_proc_decl_st{
	int lineno;
	struct _ast_proc_decl_st * lpd;
	_ast_sub_proc_decl * rspd;
}_ast_proc_decl;

/*
typedef struct _ast_lvalue_st{
	int lineno;
	char * id;

	struct _ast_exp_st * exp;
}_ast_lvalue; */


/* ---> stmt_declaration types */
/*
 * in stmt "var a1, a2: int, a4, a5, a6[2][7];"
 * "a1, a2: int" is one  _ast_subvar; "a4, a5, a6[2][7]" is another _ast_subvar;
 * there is no struct to represent the entire var-list.
 */
typedef struct _ast_subvar_st{
	int lineno;
	_ast_typeid * typeid; /* variables' type, int/bool/string */
	_ast_idlist * idl;
	_ast_array * array;

	struct _ast_subvar_st * next;
}_ast_subvar;
typedef struct _ast_type_st{
	int lineno;
	/* if, type a = int; id is "a", typeid is "int" */
	char * id;  /* id is a terminal */
	_ast_typeid * typeid; /* typeid is a non-terminal */

	_ast_array * array;
}_ast_type;
typedef struct _ast_forward_st{
	int lineno;
	char * fname; /* declared func name */
	_ast_subdeclist * sdl;
	_ast_typeid * rtype; /* return typeid */
}_ast_forward;
typedef struct _ast_proc_st{
	int lineno;
	char * id; /* mandatory */

	_ast_subdeclist * sdl;
	_ast_typeid * rtype; /* return type */
	_ast_proc_decl * proc_decl;
	struct _ast_stmt_set_st * ss;
}_ast_proc;


/* ---> stmt types */
/*typedef struct _ast_break_st{
	int lineno;
	int flag;
}_ast_break;*/
typedef struct _ast_if_st{
	int lineno;
	struct _ast_exp_st * exp; /* mandatory */
	struct _ast_stmt_set_st * iss; /* mandatory. if's stmt_set */
	struct _ast_box_stmt_set_st * bss; /* box_stmt_set */
	struct _ast_stmt_set_st * ess; /* else's stmt_set */

}_ast_if;
typedef struct _ast_do_st{
	int lineno;
	struct _ast_exp_st * exp;
	struct _ast_stmt_set_st * ss;
}_ast_do;
typedef struct _ast_fa_st{
	int lineno;
	int id; /* implicitly defined iteration condition variable */
	struct _ast_exp_st * fexp; /* start value for id */
	struct _ast_exp_st * texp; /* end value for id */
	struct _ast_stmt_set_st * ss;
}_ast_fa;
typedef struct _ast_assign_st{
	int lineno;
	char * id;

	/* lexp could be a list of exps, represents array access.
	 * "a[e][c]:=k" could be parsed as, "id(a)  [   AST_NT_LVALUE{id(e)  }   ]  [   AST_NT_LVALUE{id(c)  }   ]   :=    AST_NT_LVALUE{id(k)  } }",
	 * pay attention to the node's type */
	struct _ast_exp_st * lexp, * rexp;
}_ast_assign;
typedef struct _ast_write_st{
	int lineno;
	int newline; /* 1: indicate write-stmt; 0: indicate writes-stmt */
	struct _ast_exp_st * exp;
}_ast_write;
typedef struct _ast_exp_st{
	int lineno;
	int ntype; /* node type */

	int ivalue; /* exp->int */
	char * svalue; /* exp->string */
	char * id; /* exp->id '(' ')'; lvalue's id */
	//_ast_lvalue * lvalue;
	struct _ast_exp_st * lexp, * rexp, * next;
}_ast_exp;

typedef struct _ast_box_stmt_st{
	int lineno;
	_ast_exp * exp;
	struct _ast_stmt_set_st * ss;
}_ast_box_stmt;

typedef struct _ast_stmt_declaration_st{
	int lineno;
	int ntype; /* node type */

	_ast_subvar * var;
	_ast_type * type;
	_ast_forward * forward;
	_ast_proc * proc;

	struct _ast_stmt_declaration_st * next;

}_ast_stmt_declaration;

typedef struct _ast_box_stmt_set_st{
	int lineno;
	struct _ast_box_stmt_set_st * lbss;
	struct _ast_box_stmt * rbs;
}_ast_box_stmt_set;

typedef struct _ast_stmt_set_st{
	int lineno;
	struct _ast_stmt_set_st * lss;
	struct _ast_stmt_st * rs;
}_ast_stmt_set;


typedef struct _ast_stmt_st{
	int lineno;
	int ntype; /* node type */

	_ast_if * iif;
	_ast_do * ddo;
	_ast_fa * fa;
	_ast_assign * assign;
	_ast_write * write;
	_ast_exp * exp;

	/*used to uniquely identify a break or return*/
	int brk_flag;
	//int rt_flag;

	struct _ast_stmt_st * next;

}_ast_stmt;


/* to temporarily hold nodes when parsing recursively.
 * when adding child-node which might be recursively defined,
 * not pass this child-node as parameters of parent-node's construction function,
 * parent-node's construction function will check and update this temp list.
 *
 * because it's not easy to derive the entire list of a recursively defined element from
 * the left side of .y-file; this is my solution.
 */
typedef struct _ast_temp_st{
	/* start building in na_array; end in na_type(), na_subvar()  */
	_ast_array * array, * array_tail;

	/* start building in na_idlist(); end in na_subvar(), na_subdelist */
	_ast_idlist * idlist, * idlist_tail;

	/* start building in _ast_subvar(); end in na_stmt_decl_var(), na_sub_proc_decl() */
	_ast_subvar * subvar, * subvar_tail;

	/* start building in na_subdelist(); end in na_forward(), na_proc() */
	_ast_subdeclist * subdeclist, * subdeclist_tail;

	/* start building in append_exp() ; end in na_exp() ; to handle proc args */
	_ast_exp * exp, * exp_tail;

	/* start building in append_exparr() ; end in na_exp() ; to handle recursive sub-exp in re-written lvalue */
	_ast_exp * exparr, * exparr_tail;

	/* start building in append_stmt_set(); end in  ; to handle recursive stmt-set */
	//_ast_stmt * stmt_set, * stmt_set_tail;

	//_ast_exp * exparrass;

	/* start building in na_lvalue() ; end in ; to handle exp in lvalue */
	//_ast_exp * explv, * explv_tail;

	/* start building in  ; end in  */
}_ast_temp;

typedef struct _ast_root_st{
	_ast_stmt_declaration * stmt_declarations; /* head of stmt_declaration list */
	_ast_stmt_declaration * stmt_declarations_tail; /* tail of stmt_declaration list */
	_ast_stmt * stmts; /* head of stmt list */
	_ast_stmt * stmts_tail; /* tail of stmt list */

	_ast_temp t;
}_ast_root;
/* ----------end define AST structs---------- */


/* ----------start define code generation related structs---------- */
typedef struct _cg_temp_st{ /* includes fields to assist code generation */
	/* for a particular case, only use some of the fields */
	int i_index; /* iMem index */
	int d_index; /* dMem index*/
	int local; /*indicate if a var is local var, 1:yes, 0:no; default is 0*/
	int ar_size; /*activation record size, for proc CG*/
	_ast_proc * lvar_proc; /*refer to local var's proc*/
	int lvar_fp_offset; /* local var's offset, relative to FP */
	int * arr_dim; /*an array's dimension sizes, every arr_dim[i] denote the size of a dimension*/
	int arr_dim_sum; /*sum of all dimension size of arr_dim*/
	int str_len;
	int str_start_dmem;
	/*int exp_value_dmem; dedicated global dMem to hold if's-exp/fa's-second-exp, do not re-evaluate it again*/
}_cg_temp;
/* ----------end define code generation related structs---------- */


/* ----------start define semantic-check related structs---------- */
#define SCOPE_DEFAULT 0
#define SCOPE_GLOBAL 1
#define SCOPE_PROC 2
#define SCOPE_FA 3

typedef struct _symbol_st{
	 /*int stype; type, var proc */
	char * name;

	/* the following are optional, just for particular semantic verification case; should have put them together in a struct as _cg_temp */
	_ast_array * array; /* for var/type-decl, record array info if a var/type is an array */
	struct _symbol_st * var_type; /* for var-decl, record var's type info */
	struct _symbol_st * p_type; /* for type-decl, record type's "parent" type; type a=int, type a's parent type is "int" */
	int fwd_unmatched;  /* for forward-decl, if 1, indicate the forward-stmt has not been matched by a proc */
	_ast_forward * fwd; /* for forward-decl */
	int is_fa_loop_var; /* for fa; indicate if this is a fa-loop-variable, this kind of var isn't assignable in fa; 0: no, 1: yes */
	struct _symbol_st * proc_rtype; /* for proc/forward's return type */
	_ast_proc * proc; /* for proc-decl */
	int is_proc, is_fwd; /* for proc/fwd, to indicate if this symbol is proc or fwd; 1: yes */

	_cg_temp * cgp; /* to assist code generation */
}_symbol;

typedef struct _scope_st{
	int stype; /* global, proc, fa */
	i9_hash_table type_table;
	i9_hash_table var_table;
	i9_hash_table proc_table;

}_scope;
/* ----------end define semantic related structs---------- */


typedef struct _ice9_core_st{
	_ast_root * ast;
	logger * lgr;

	/*
	 * every stack's element holds a scope,
	 * every scope has three hash-tables,
	 * every hash-table holds a collection of symbols,
	 * every symbol may contain a pointer to a ast_node
	 */
	i9_stack scope_stack;
}_ice9_core;


/* must be called before yyparse(), and can only be called once. */
void ice9_init();

/* "na" stands for newast */

/* it will check array/array_tail in _ast_temp */
_ast_type * na_type(char * id, _ast_typeid * typeid, int lineno);

_ast_typeid * na_typeid(char * id, int lineno);

_ast_array * na_array(int dimension, int lineno);

_ast_idlist * na_idlist(char * id, int lineno);

/* it will check idlist/idlist_tail and array/array_tail in _ast_temp */
_ast_subvar * na_subvar(_ast_typeid * typeid, int lineno);

/* it will check idlist/idlist_tail in _ast_temp */
_ast_subdeclist * na_subdelist(_ast_typeid * typeid, int lineno);

/* it will check _ast_subdeclist in _ast_temp */
_ast_forward * na_forward(char * fname, _ast_typeid * rtype, int lineno);

/* it will check _ast_subdeclist in _ast_temp */
_ast_proc * na_proc(char * id, _ast_typeid * rtype, _ast_proc_decl * proc_decl, _ast_stmt_set * ss, int lineno);

//_ast_lvalue * na_lvalue(char * id, _ast_exp * exp, int lineno);

/* it will check exp/exp_tail exparr/exparr_tail in _ast_temp */
_ast_exp * na_exp(int ntype, int ivalue, char * svalue, char * id, _ast_exp * lexp, _ast_exp * rexp, int lineno);

/* it will check exparr/exparr_tail in _ast_temp */
_ast_assign * na_assign(char * id, _ast_exp * lexp, _ast_exp * rexp, int lineno);

_ast_assign * na_assign2(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * rexp, int lineno);
_ast_assign * na_assign3(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * lexp3, _ast_exp * rexp, int lineno);
_ast_assign * na_assign4(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * lexp3, _ast_exp * lexp4, _ast_exp * rexp, int lineno);

_ast_write * na_write(int newline, _ast_exp * e, int lineno);

_ast_do * na_do(_ast_exp * exp, _ast_stmt_set * ss, int lineno);

_ast_fa * na_fa(char * id, _ast_exp * fexp, _ast_exp * texp, _ast_stmt_set * ss, int lineno);

_ast_if * na_if(_ast_exp * exp, _ast_stmt_set * iss, _ast_box_stmt_set * bss, _ast_stmt_set * ess, int lineno);

/* it will check subvar/subvar_tail in _ast_temp */
_ast_sub_proc_decl * na_sub_proc_decl(int ntype, _ast_type * type, int lineno);

_ast_box_stmt * na_box_stmt(_ast_exp * exp, _ast_stmt_set * ss, int lineno);

/* append exp to temp list; to handle recursive sub-exp in proc call */
void append_exp(_ast_exp * e);

/* append exp to temp list; to handle recursive sub-exp in re-written lvalue */
void append_exparr(_ast_exp * e);

void append_stmt_decl_2root(_ast_stmt_declaration * sd); /* to handle recursive stmt_decl */

void append_stmt_2root(_ast_stmt * s); /* to handle recursive stmt for stmt_list */

//void append_stmt_set(_ast_stmt * s, int lineno); /* to handle recursive stmt for stmt_set */

/* create new ast stmt_declaration node, with "type" as node type */
_ast_stmt_declaration * na_stmt_decl_type(_ast_type * type, int lineno);

/* create new ast stmt_declaration node, with "var" as node type; it will check subvar/subvar_tail in _ast_temp */
_ast_stmt_declaration * na_stmt_decl_var(int lineno);

/* create new ast stmt_declaration node, with "forward" as node type */
_ast_stmt_declaration * na_stmt_decl_forward(_ast_forward * f, int lineno);

/* create new ast stmt_declaration node, with "proc" as node type */
_ast_stmt_declaration * na_stmt_decl_proc(_ast_proc * p, int lineno);

/* create new ast stmt node, with "exp" as node type */
_ast_stmt * na_stmt_exp(_ast_exp * e, int lineno);

/* create new ast stmt node, with "assign" as node type */
_ast_stmt * na_stmt_assign(_ast_assign * a, int lineno);

/* create new ast stmt node, with "assign" as node type */
_ast_stmt * na_stmt_write(_ast_write * w, int lineno);

/* create new ast stmt node, with "break" as node type */
_ast_stmt * na_stmt_break(int lineno);

/* create new ast stmt node, with "exit" as node type */
_ast_stmt * na_stmt_exit(int lineno);

/* create new ast stmt node, with "return" as node type */
_ast_stmt * na_stmt_return(int lineno);

/* create new ast stmt node, with "do" as node type */
_ast_stmt * na_stmt_do(_ast_do * d, int lineno);

/* create new ast stmt node, with "fa" as node type */
_ast_stmt * na_stmt_fa(_ast_fa * f, int lineno);

/* create new ast stmt node, with "if" as node type */
_ast_stmt * na_stmt_if(_ast_if * i, int lineno);

/* create new ast stmt node, with "empty" as node type */
_ast_stmt * na_stmt_empty(int lineno);

_ast_stmt_set * na_stmt_set(_ast_stmt_set * lss, _ast_stmt * rs, int lineno);

_ast_box_stmt_set * na_box_stmt_set(_ast_box_stmt_set * lbss, _ast_box_stmt * rbs, int lineno);

_ast_proc_decl * na_proc_decl(_ast_proc_decl * lpd, _ast_sub_proc_decl * rspd, int lineno);



_symbol * n_symbol(char * name);
void destruct_symbol(_symbol * s);
_scope * n_scope(int stype, int ssize /* scope size */, int init_type, int init_var, int init_proc);
void destruct_scope(_scope * s);


_cg_temp * n_cg_temp();


#endif
