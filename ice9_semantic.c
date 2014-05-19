/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * ice9.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ice9.h"

extern _ice9_core * ice9_engine;

static char str_buffer[STR_BUFFER_SIZE];
static int do_count=0; /* to count if currently in do-loop */

void check_stmt(_ast_stmt * sp);
void check_stmt_set(_ast_stmt_set * ssp);

static void reset_str_buffer(){
	memset(str_buffer, 0, sizeof(char)*STR_BUFFER_SIZE);
}

static void semantic_error(char * op){
	fprintf(stderr, op);
	exit(1);
}

/* search for a proc-symbol in proc tables,
 * from current scope to higher global;
 * will return one matched proc in the closest scope. */
_symbol * search_all_proc_tables(char * id){
	_scope * scp = NULL;
	i9_stack_elem stke = NULL;
	_symbol * sy = NULL;

	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "search_all_proc_tables, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return NULL;
	}

	stke = stk_peek_elem(ice9_engine->scope_stack);
	scp = (_scope*)stk_get_elem_data(stke);
	while(stke!=NULL){
		//printf("---%d\n", scp->proc_table);

		sy = (_symbol*)ht_search(id, scp->proc_table);
		//printf("---\n");
		//printf("---1\n");
		if(sy!=NULL)
			return sy;
		//printf("---2\n");
		stke = stk_get_next_elem( stke );
		if(stke!=NULL)
			scp = (_scope*)stk_get_elem_data(stke);
		//printf("---3\n");
	}

	return NULL;
}

/* search for a var-symbol in var tables,
 * from current scope to higher global;
 * will return one matched var in the closest scope. */
_symbol * search_all_var_tables(char * id){
	_scope * scp = NULL;
	i9_stack_elem stke = NULL;
	_symbol * sy = NULL;

	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "search_all_var_tables, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return NULL;
	}

	stke = stk_peek_elem(ice9_engine->scope_stack);
	scp = (_scope*)stk_get_elem_data(stke);
	while(stke!=NULL){
		sy = (_symbol*)ht_search(id, scp->var_table);
		if(sy!=NULL)
			return sy;

		stke = stk_get_next_elem( stke );
		if(stke!=NULL)
			scp = (_scope*)stk_get_elem_data(stke);
	}

	return NULL;
}

/* search for a type-symbol in type tables,
 * from current scope to higher global;
 * will return one matched type in the closest scope. */
_symbol * search_all_type_tables(char * id){
	_scope * scp = NULL;
	i9_stack_elem stke = NULL;
	_symbol * sy = NULL;

	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "search_all_type_table, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return NULL;
	}

	stke = stk_peek_elem(ice9_engine->scope_stack);
	scp = (_scope*)stk_get_elem_data(stke);
	while(stke!=NULL){
		sy = (_symbol*)ht_search(id, scp->type_table);
		if(sy!=NULL){
			/* printf("*****%s******\n",sy->name); */
			return sy;
		}

		stke = stk_get_next_elem( stke );
		if(stke!=NULL)
			scp = (_scope*)stk_get_elem_data(stke);
	}

	return NULL;
}

/* given a string, check if it's basic type: int/string/bool; 0:no, 1:yes */
int is_basic_type(char * s){
	if(s==NULL)
		return 0;

	if( strcmp(s, "int")==0 )
		return 1;
	if( strcmp(s, "string")==0 )
		return 1;
	if( strcmp(s, "bool")==0 )
		return 1;

	return 0;
}

/* input: return value of get_typeid_array_dimensions();
 * return: how many valid elements(how many dimensions) in the array */
int get_array_d_num(int * arr){
	int i=0;
	if(arr==NULL)
		return 0;

	for(i=0;i<ARR_MAX_DIMENSION;i++){
		if(arr[i]==-1)
			break;
	}

	return i;
}

/*
 * given a var symbol, get its total array dimensions info.
 *
 * type a=int[5];
 * var x: a[2];
 */
int * get_var_array_dimensions(_symbol * sy){
	int * arr = NULL;
	int i;
	_ast_array * ap = NULL;
	_symbol * sy1 = NULL;

	if(sy==NULL)
		return NULL;

	arr = (int*)malloc(sizeof(int)*ARR_MAX_DIMENSION+1);
	if(arr==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "get_var_array_dimensions, out of memory", ICE9_LOG_LEVEL_ERROR);
		exit(1);
#endif
	}
	memset(arr, 0, sizeof(int)*ARR_MAX_DIMENSION+1);
	for(i=0; i<ARR_MAX_DIMENSION; i++)
		arr[i]=-1;

	ap = sy->array;
	i=0;
	while(ap!=NULL){
		arr[i++]=ap->demension;
		ap=ap->next;
	}

	sy1 = (_symbol*)search_all_type_tables( sy->var_type->name );
	while(sy1!=NULL){
		ap = sy1->array;
		while(ap!=NULL){
			arr[i++]=ap->demension;
			ap=ap->next;
		}

		sy1=sy1->p_type;
	}

	if(i==0){ /* if not an array */
		free(arr);
		return NULL;
	}

	return arr;
}
/*
 * used by proc-forward parameter's types verification.
 * proc/forward's parameters cannot be explicitly declared as an array, but could be implicitly.
 *
 * given a typeid's id, return the typeid's array dimensions as an int array, if it's a redefined array;
 * array's length is how many dimensions;
 * every element of the array means the size of this dimension.
 * if return NULL, means the redefined typeid is not an array.
 * users of this function should free the returned array. */
int * get_typeid_array_dimensions(char * id){
	int * arr = NULL;
	int i;
	_symbol * sy1 = NULL;
	_ast_array * ap = NULL;
	/* _scope * current_sp=NULL; */

	if(id==NULL)
		return NULL;

	/* if typeid is basic, then this is not an array */
	if( is_basic_type( id) )
		return NULL;

	arr = (int*)malloc(sizeof(int)*ARR_MAX_DIMENSION+1);
	if(arr==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "get_typeid_array_dimensions, out of memory", ICE9_LOG_LEVEL_ERROR);
		exit(1);
#endif
	}
	memset(arr, 0, sizeof(int)*ARR_MAX_DIMENSION+1);
	for(i=0; i<ARR_MAX_DIMENSION; i++)
		arr[i]=-1;

	i=0;
	/* current_sp = (_scope*)stk_peek(ice9_engine->scope_stack); */
	sy1 = (_symbol*)search_all_type_tables( id ); /* sy1's pre-definition should be guaranteed in earlier code check */

	while(sy1!=NULL){
		//printf("---%s---\n", sy1->name);
		ap = sy1->array;
		while(ap!=NULL){
			arr[i++]=ap->demension;
			ap=ap->next;
		}

		sy1=sy1->p_type;
	}

	/*for(i=0; i<ARR_MAX_DIMENSION; i++)
		printf(" %d ", arr[i]);
	printf("\n");*/

	if(i==0){ /* if not an array */
		free(arr);
		return NULL;
	}

	return arr;
}

/**
 * type a=int[5];
 * var x: a[2];
 */
int is_var_array(_symbol * sy){
	_symbol * syt = NULL;

	if(sy==NULL){
		return 0;
	}

	syt=sy;
	if(syt->array!=NULL)
		return 1;

	syt = syt->var_type; /* check var type*/
	while(syt!=NULL){
		if(syt->array!=NULL)
			return 1;
		syt=syt->p_type;
	}

	return 0;
}

/* check if the symbol is redefined by type-stmt as an array,
 * such as, type a1 = int[2][7];
 * 1: yes; 0:no */
int is_type_array(_symbol *sy){
	_symbol * syt = NULL;

	if(sy==NULL){
		return 0;
	}

	syt=sy;
	while(syt!=NULL){
		if(syt->array!=NULL)
			return 1;
		syt=syt->p_type;
	}

	return 0;
}

/* return a type-symbol's root type, int/string/bool; ignore array info */
int get_root_type(_symbol * sy){
	_symbol * syt = NULL;
	int type=S_TYPE_UNKNOWN;

	if(sy==NULL)
		return S_TYPE_UNKNOWN;

	syt=sy;
	while(syt->p_type!=NULL)
		syt=syt->p_type;

	if(strcmp(syt->name, "int")==0)
		type = S_TYPE_INT_BASIC;
	else if(strcmp(syt->name, "string")==0)
		type = S_TYPE_STRING_BASIC;
	else if(strcmp(syt->name, "bool")==0)
		type = S_TYPE_BOOL_BASIC;
	else{
#if ICE9_DEBUG
		sprintf(str_buffer, "get_root_type, unknown root type, %s ", syt->name );
		ice9_log_level(ice9_engine->lgr, str_buffer, ICE9_LOG_LEVEL_ERROR);
		reset_str_buffer();
#endif
	}

	return type;
}

int get_proc_smallest_lineno(_ast_proc * ap){
	int dft=0;

	if(ap==NULL)
		return 0;

	dft = ap->lineno;
	if(ap->sdl!=NULL){
		return ap->sdl->lineno;
	}

	if(ap->rtype!=NULL)
		return ap->rtype->lineno;

/*	if(ap->proc_decl!=NULL)
		return ap->proc_decl->lineno;

	if(ap->ss!=NULL)
		return ap->proc_decl->lineno;*/

	return dft;
}




/* will return exp's evaluation value type: int/string/bool;
 * verify if different types matches */
int check_exp(_ast_exp * exp){
	_ast_exp * te=NULL, * te1=NULL;
	int e1=0, e2=0, i=0, arr_total_d=0, root_type=0, j=0, k=0;
	_symbol * sy1=NULL, *sy2=NULL, *sy3=NULL;
	_ast_subdeclist * sdl=NULL;
	_ast_idlist * idl;
	int * type_arr_d1=NULL, * type_arr_d2=NULL;
	int d1=0, d2=0;

	if(exp==NULL)
		return S_TYPE_UNKNOWN;

	switch(exp->ntype){
	/* LVALUE is a var. maybe an array, such as:
	 * type a=int[5];
	 * var x: a[2]; */
	case AST_NT_LVALUE:
		/* verify if LVALUDE's id defined */
		sy1=search_all_var_tables(exp->id);
		if(sy1==NULL){
			sprintf(str_buffer, "line %d: undefined variable %s\n", exp->lineno, exp->id);
			semantic_error(str_buffer);
		}
		root_type = get_root_type(sy1->var_type);

		te=exp->lexp;
		/* if LVALUE is not array operation, but this var's is redefined array */
		if(te==NULL && is_var_array(sy1)==1 ){
			switch (root_type) {
			case S_TYPE_INT_BASIC:
				return S_TYPE_INT_ARR;
			case S_TYPE_STRING_BASIC:
				return S_TYPE_STRING_ARR;
			case S_TYPE_BOOL_BASIC:
				return S_TYPE_BOOL_ARR;
			default:
				return S_TYPE_UNKNOWN;
			}
		}

		/* if LVALUE is array operation, check if every dimension exp is int */
		i=0;
		while(te!=NULL){
			e1 = check_exp(te);
			/* printf("----%d---\n", te->ivalue); */
			if(e1!=S_TYPE_INT_BASIC){
				sprintf(str_buffer, "line %d: invalid array operation, %s's NO. %d dimension index-exp must be int\n", te->lineno, exp->id, i);
				semantic_error(str_buffer);
			}
			i++;
			te=te->next;
		}

		/* if LVALUE is array operation, check dimension operation */
		if(exp->lexp!=NULL){
			/* get this var's total number of dimension, may has explicit array decl or redefined array type or both */
			arr_total_d = get_array_d_num( get_var_array_dimensions(sy1) );
			if(i<arr_total_d){
				switch(root_type){
				case S_TYPE_INT_BASIC:
					return S_TYPE_INT_ARR;
				case S_TYPE_STRING_BASIC:
					return S_TYPE_STRING_ARR;
				case S_TYPE_BOOL_BASIC:
					return S_TYPE_BOOL_ARR;
				default:
					return S_TYPE_UNKNOWN;
				}
			}

			if(i>arr_total_d){ /* too many array dimensions */
				if(arr_total_d==0)
					sprintf(str_buffer, "line %d: invalid array operation, %s is not an array\n", exp->lineno, exp->id, arr_total_d );
				else
					sprintf(str_buffer, "line %d: invalid array operation, %s only has %d dimension(s)\n", exp->lineno, exp->id, arr_total_d );
				semantic_error(str_buffer);
			}
		}

		/* return LVALUDE's id's type */
		return root_type;

		break;
	case AST_NT_INT:
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_TRUE:
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_FALSE:
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_STR:
		return S_TYPE_STRING_BASIC;
		break;
	case AST_NT_READ:
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_UMINUS:
		e1 = check_exp(exp->rexp);

		if(e1!=S_TYPE_BOOL_BASIC && e1!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in unary operation, operand must be int or boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}

		switch(e1){
		case S_TYPE_BOOL_BASIC:
			return S_TYPE_BOOL_BASIC;
		case S_TYPE_INT_BASIC:
			return S_TYPE_INT_BASIC;
		}
		break;
	case AST_NT_QUEST:   /* The ? operator converts a bool value to an int. */
		e1 = check_exp(exp->rexp);

		if(e1!=S_TYPE_BOOL_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in ? operation, operand must be boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_EPROC:
		/* check proc must be defined earlier */
		sy1 = search_all_proc_tables(exp->id);
		if(sy1==NULL){
			sprintf(str_buffer, "line %d: proc %s undefined\n", exp->lineno, exp->id );
			semantic_error(str_buffer);
		}

		/* verify proc-call's parameters: numbers/types match */
		i=0; /* i: proc-call's parameter number */
		te = exp->lexp;
		while(te!=NULL){
			i++;
			te=te->next;
		}
		j=0; /* j: proc defined parameter number */
		if(sy1->proc!=NULL)  /*it's a proc*/  /*this kind of if-else check is for, mutually recursive defined procs, test case "proc4" */
			sdl = sy1->proc->sdl;
		else  /*it's a forward*/
			sdl = sy1->fwd->sdl;
		while(sdl!=NULL){
			idl = sdl->idl;
			while(idl!=NULL){
				j++;

				idl=idl->next;
			}
			sdl=sdl->next;
		}
		if(i!=j){
			sprintf(str_buffer, "line %d: wrong number of parameters for proc-call, proc definition requires %d parameter, but given %d\n", exp->lineno, j, i );
			semantic_error(str_buffer);
		}  /* finish checking parameter number */


		/* verify if parameters type match;
		 * proc-call's argument could have explicit array info; but proc's def cannot, could only be implicit array type; */
		i=0;
		te = exp->lexp; /* te: one proc-call's parameter */
		if(sy1->proc!=NULL)
			sdl = sy1->proc->sdl;
		else
			sdl = sy1->fwd->sdl;
		if(sdl!=NULL){
			idl = sdl->idl;
			sy2 = search_all_type_tables(sdl->typeid->id); /* get proc parameter-list's type */
			root_type = get_root_type(sy2); /* ignore array info */
		}
		while(te!=NULL){
			e1=check_exp(te);
			i++;

			/* if proc-call parameter is basic, and proc def's parameter is also basic; */
			if( (e1>=S_TYPE_INT_BASIC && e1<=S_TYPE_BOOL_BASIC) && is_type_array(sy2)==0 ){
				/* then further check if basic type match */
				if(e1!=root_type){
					sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
					semantic_error(str_buffer);
				}
			/* if proc-call parameter not basic, but proc def's parameter is basic; */
			}else if( e1>=S_TYPE_INT_ARR && is_type_array(sy2)==0){
				sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
				semantic_error(str_buffer);
			/* if proc-call parameter is basic, but proc def's parameter is not basic; */
			}else if( (e1>=S_TYPE_INT_BASIC && e1<=S_TYPE_BOOL_BASIC) && is_type_array(sy2)==1 ){
				sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
				semantic_error(str_buffer);

			}else{ /* if proc-call parameter is array(must be LVALUE), and proc def's parameter is also array */
				switch(e1){ /* first check if array types match */
				case S_TYPE_INT_ARR:
					if(root_type!=S_TYPE_INT_BASIC){
						sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
						semantic_error(str_buffer);
					}
					break;
				case S_TYPE_STRING_ARR:
					if(root_type!=S_TYPE_STRING_BASIC){
						sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
						semantic_error(str_buffer);
					}
					break;
				case S_TYPE_BOOL_ARR:
					if(root_type!=S_TYPE_BOOL_BASIC){
						sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
						semantic_error(str_buffer);
					}
					break;
				default:
					sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
					semantic_error(str_buffer);
				} /* end of switch */

				/* then check if array dimensions match */
				if(te->ntype==AST_NT_LVALUE){ /* should be AST_NT_LVALUE, just in case.... */
					d1=d2=0;
					j=0;
					type_arr_d1 = get_var_array_dimensions( search_all_var_tables(te->id) ); /* proc-call parameter type's array dimensions */
					type_arr_d2 = get_typeid_array_dimensions(sy2->name); /* proc def's parameter's type array info; proc parameter only be defined as array implicitly */
					d1 = get_array_d_num(type_arr_d1);
					d2 = get_array_d_num(type_arr_d2);

					te1 = te->lexp;
					if(te1==NULL){ /* this proc-call parameter is implicit array; then just check if type_arr_d1 match type_arr_d1 */
						if(d1!=d2){ /* dimensions number not match */
							sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
							semantic_error(str_buffer);   /* maybe more helpful info?? */
						}
						for(j=0;j<d1;j++){
							if( type_arr_d1[j]!=type_arr_d2[j] ){
								sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
								semantic_error(str_buffer);   /* maybe more helpful info?? */
							}
						}
					}else{ /* this proc-call parameter has explicit array access operation */
						j=0;
						while(te1!=NULL){
							check_exp(te1);
							j++;  /* how many dimensions on left side */
							te1=te1->next;
						}
						k=0;
						for(;j<d1;j++){  /* check if remaining dimension size match */
							if( type_arr_d1[j]!=type_arr_d2[k++] ){
								sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
								semantic_error(str_buffer);   /* maybe more helpful info?? */
							}
						}
						if( type_arr_d2[k]!=-1 ){ /* proc def's parameter's type array has more unmatched dimensions */
							sprintf(str_buffer, "line %d: invalid NO. %d proc-call parameter, %s, type not match proc definition\n", te->lineno, i, te->id );
							semantic_error(str_buffer);   /* maybe more helpful info?? */
						}
					}

					free(type_arr_d1);
					free(type_arr_d2);
				}else{  /* unknown error */
#if ICE9_DEBUG
					ice9_log_level(ice9_engine->lgr, "check_exp(), UNKNOWN condition1", ICE9_LOG_LEVEL_ERROR);
#endif
				}
			}  /* end of else */

			te=te->next;
			idl=idl->next;
			if(idl==NULL){
				sdl=sdl->next;
				if(sdl!=NULL){
					idl = sdl->idl;
					sy2 = search_all_type_tables(sdl->typeid->id);
					root_type = get_root_type(sy2);
				}
			}
		} /* end while, match proc-call parameters */


		/* use proc's return value as */
		if( sy1->proc_rtype!=NULL )
			return get_root_type( sy1->proc_rtype );
		else
			return S_TYPE_UNKNOWN;
		break;
	case AST_NT_PLUS:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if( (e1==S_TYPE_INT_BASIC && e2==S_TYPE_INT_BASIC) || (e1==S_TYPE_BOOL_BASIC && e2==S_TYPE_BOOL_BASIC) ){
		}else{
			sprintf(str_buffer, "line %d: incompatible types in + operation, both operands must all be integer or boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}

		if(e1==S_TYPE_INT_BASIC)
			return S_TYPE_INT_BASIC;
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_MINUS:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in integer subtraction, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_MUL:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if( (e1==S_TYPE_INT_BASIC && e2==S_TYPE_INT_BASIC) || (e1==S_TYPE_BOOL_BASIC && e2==S_TYPE_BOOL_BASIC) ){
		}else{
			sprintf(str_buffer, "line %d: incompatible types in * operation, both operands must all be integer or boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}

		if(e1==S_TYPE_INT_BASIC)
			return S_TYPE_INT_BASIC;
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_DIV:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in integer division, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_MOD:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in integer modulo, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_EQU:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if( (e1==S_TYPE_INT_BASIC && e2==S_TYPE_INT_BASIC) || (e1==S_TYPE_BOOL_BASIC && e2==S_TYPE_BOOL_BASIC) ){
		}else{
			sprintf(str_buffer, "line %d: incompatible types in equal comparison, both operands must all be integer or boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_NE:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if( (e1==S_TYPE_INT_BASIC && e2==S_TYPE_INT_BASIC) || (e1==S_TYPE_BOOL_BASIC && e2==S_TYPE_BOOL_BASIC) ){
		}else{
			sprintf(str_buffer, "line %d: incompatible types in not-equal comparison, both operands must all be integer or boolean\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_GT:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in greater-than comparison, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_LT:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in less-than comparison, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_GE:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in greater-than-or-equal comparison, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_LE:
		e1 = check_exp(exp->lexp);
		e2 = check_exp(exp->rexp);

		if(e1!=S_TYPE_INT_BASIC || e2!=S_TYPE_INT_BASIC){
			sprintf(str_buffer, "line %d: incompatible types in less-than-or-equal comparison, both operands must be integer\n", exp->lineno );
			semantic_error(str_buffer);
		}
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_PAREN:
		return check_exp(exp->lexp);
		break;
	}

	return S_TYPE_UNKNOWN;
}

void check_box_stmt(_ast_box_stmt * bs){
	int e=0;
	if(bs==NULL)
		return;

	e = check_exp(bs->exp);
	if(e!=S_TYPE_BOOL_BASIC){
		sprintf(str_buffer, "line %d: if's exp must be boolean\n", bs->exp->lineno );
		semantic_error(str_buffer);
	}

	check_stmt_set(bs->ss);
}

void check_box_stmt_set(_ast_box_stmt_set * bss){
	if(bss==NULL)
		return;

	if(bss->lbss!=NULL){
		check_box_stmt_set(bss->lbss->lbss);
		check_box_stmt(bss->lbss->rbs);
	}
	check_box_stmt(bss->rbs);
}

void check_stmt_set(_ast_stmt_set * ssp){
	if(ssp==NULL)
		return;

	if(ssp->lss!=NULL){
		check_stmt_set(ssp->lss->lss);
		check_stmt(ssp->lss->rs);
	}
	check_stmt(ssp->rs);
}

/*var/type checking functionality similar to check_stmt_decl(), but parameter not the same*/
void check_sub_proc_decl(_ast_sub_proc_decl * spd){
	_ast_subvar *sv;
	_ast_idlist * idl;
	_scope * current_sp=NULL;
	_symbol * sy1 = NULL, * sy2=NULL;

	if(spd==NULL)
		return;

	current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);

	switch(spd->ntype){
	case AST_NT_TYPE:
		/* verify type's typeid, the "parent" type, must be defined earlier; type a4 = int, check if "int" is defined */
		sy1 = (_symbol*)search_all_type_tables(spd->type->typeid->id);
		if(sy1==NULL){
			sprintf(str_buffer, "line %d: undefined type, %s\n", spd->type->typeid->lineno, spd->type->typeid->id);
			semantic_error(str_buffer);
		}

		/* verify type's id, cannot be redefined in CURRENT scope's type-table; type a4 = int, check if "a4" is defined */
		sy2 = (_symbol*) ht_search(spd->type->id, current_sp->type_table);
		if (sy2 != NULL) {
			sprintf(str_buffer, "line %d: redefined type, %s\n", spd->type->lineno, spd->type->id);
			semantic_error(str_buffer);
		}
		/* else, insert the new type to type-table */
		sy2 = n_symbol(spd->type->id);
		sy2->p_type = sy1;
		if (spd->type->array != NULL)
			sy2->array = spd->type->array;
		ht_insert(spd->type->id, sy2, current_sp->type_table);

		break;

	case AST_NT_VAR:

		sv = spd->var;
		while (sv != NULL) {
			/* verify var's type, must be defined earlier; */
			sy1 = (_symbol*)search_all_type_tables( sv->typeid->id );
			if(sy1==NULL){
				sprintf(str_buffer, "line %d: undefined type, %s\n", sv->lineno, sv->typeid->id );
				semantic_error(str_buffer);
			}

			/* verify var's id, cannot be redefined in current scope's var-table */
			idl = sv->idl;
			while (idl != NULL) {
				/* check if redefined */
				sy2 = (_symbol*)ht_search(idl->id, current_sp->var_table);
				if(sy2!=NULL){
					sprintf(str_buffer, "line %d: redefined variable, %s\n", idl->lineno, idl->id );
					semantic_error(str_buffer);
				}

				/* else, insert the var into var-table */
				sy2 = n_symbol(idl->id);
				sy2->var_type=sy1; /* record this var's type */
				if(sv->array!=NULL) /* if var is an array */
					sy2->array=sv->array;
				ht_insert(idl->id, sy2, current_sp->var_table);
				idl = idl->next;
			}

			sv = sv->next;
		}

		break;
	} /* end of switch-case */
}

void check_proc_decl(_ast_proc_decl * pd){
	if(pd==NULL)
		return;

	if(pd->lpd!=NULL){
		check_proc_decl(pd->lpd->lpd);
		check_sub_proc_decl(pd->lpd->rspd);
	}
	check_sub_proc_decl(pd->rspd);
}

void check_stmt_decl(_ast_stmt_declaration * sdp){
	_ast_array *ap=NULL;
	_ast_subvar *sv=NULL;
	_ast_idlist * idl=NULL, *idl1=NULL, * idl2=NULL;
	_ast_forward * fw=NULL;
	_ast_subdeclist * sdl=NULL, * sdl1=NULL, * sdl2=NULL;
	_ast_assign * ass=NULL;
	_ast_exp * exp=NULL;

	_symbol * sy1 = NULL, * sy2 = NULL, * sy3=NULL, *sy4=NULL;
	int proc_lineno=0;
	int fwd_rt, proc_rt; /* forward/proc return-type */
	int fwd_pt, proc_pt; /* forward/proc parameter-type */
	int * fwd_arr_d, * proc_arr_d;  /* forward/proc parameter type's array dimension */
	int i=0, j=0;
	int d1=0, d2=0;

	_scope * current_sp = NULL;
	_scope * scp1 = NULL;
	void * scope_bak=NULL;

	current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);
	while (sdp != NULL) {

		switch(sdp->ntype){
		/* this is for stmt-decl checking, var-stmt must be in global-scope; don't need to worry outter scope */
		case AST_NT_VAR:
			sv = sdp->var;
			while (sv != NULL) {

				/* verify var's type, must be defined earlier; */
				sy1 = (_symbol*)search_all_type_tables( sv->typeid->id );
				if(sy1==NULL){
					sprintf(str_buffer, "line %d: undefined type, %s\n", sv->lineno, sv->typeid->id );
					semantic_error(str_buffer);
				}

				/* verify var's id, cannot be redefined in CURRENT scope's var-table */
				idl = sv->idl;
				while (idl != NULL) {
					/* check if redefined */
					sy2 = (_symbol*)ht_search(idl->id, current_sp->var_table);
					if(sy2!=NULL){
						sprintf(str_buffer, "line %d: redefined variable, %s\n", idl->lineno, idl->id );
						semantic_error(str_buffer);
					}

					/* else, insert the var into var-table */
					sy2 = n_symbol(idl->id);
					sy2->var_type=sy1; /* record this var's type */
					if(sv->array!=NULL) /* if var is an array */
						sy2->array=sv->array;
					ht_insert(idl->id, sy2, current_sp->var_table);
					idl = idl->next;
				}

				sv = sv->next;
			}
			break;

		case AST_NT_TYPE:
			/* verify type's typeid, the "parent" type, must be defined earlier; type a4 = int, check if "int" is defined */
			sy1 = (_symbol*)search_all_type_tables(sdp->type->typeid->id );
			if(sy1==NULL){
				sprintf(str_buffer, "line %d: undefined type, %s\n", sdp->type->typeid->lineno, sdp->type->typeid->id );
				semantic_error(str_buffer);
			}

			/* verify type's id, cannot be redefined in CURRENT scope's type-table; type a4 = int, check if "a4" is defined */
			sy2 = (_symbol*)ht_search(sdp->type->id, current_sp->type_table);
			if(sy2!=NULL){
				sprintf(str_buffer, "line %d: redefined type, %s\n", sdp->type->typeid->lineno, sdp->type->id );
				semantic_error(str_buffer);
			}
			/* else, insert the new type to type-table */
			sy2 = n_symbol( sdp->type->id );
			sy2->p_type=sy1;
			if(sdp->type->array!=NULL)
				sy2->array = sdp->type->array;
			ht_insert(sdp->type->id, sy2, current_sp->type_table);

			break;

		case AST_NT_FORWARD:
			/* verify if forward decl's name is redefined, cannot be redefined in CURRENT scope */
			sy1 = (_symbol*)ht_search(sdp->forward->fname, current_sp->proc_table);
			if(sy1!=NULL && sy1->is_fwd!=0 ){
				sprintf(str_buffer, "line %d: redefined forward, %s\n", sdp->forward->lineno, sdp->forward->fname );
				semantic_error(str_buffer);
			}

			if(sy1!=NULL && sy1->is_proc==1){ /* if forward meet a pre-define proc, just ignore this forward, not verify, not insert to proc-table */
				break;
			}

			/* if forward has a return type, verify if that type is defined earlier, must be already defined */
			if(sdp->forward->rtype!=NULL){
				sy2 = (_symbol*)search_all_type_tables(sdp->forward->rtype->id );
				if(sy2==NULL){
					sprintf(str_buffer, "line %d: undefined forward's return type, %s\n", sdp->forward->rtype->lineno, sdp->forward->rtype->id );
					semantic_error(str_buffer);
				}

				/* verify if return type is basic */
				if( is_type_array(sy2) ){
					sprintf(str_buffer, "line %d: forward's return type, %s, is not basic type\n", sdp->forward->rtype->lineno, sdp->forward->rtype->id );
					semantic_error(str_buffer);
				}
			}

			/* verify forward's parameters list, every parameter's type must be defined earlier; forward's parameter names don't matter */
			sdl = sdp->forward->sdl;
			while (sdl != NULL) {
				sy2 = (_symbol*)search_all_type_tables(sdl->typeid->id );
				if(sy2==NULL){
					sprintf(str_buffer, "line %d: undefined forward parameter's type, %s\n", sdl->typeid->lineno, sdl->typeid->id );
					semantic_error(str_buffer);
				}
				sdl = sdl->next;
			}

			/* insert forward to the proc-table */
			sy2 = n_symbol( sdp->forward->fname );
			sy2->fwd_unmatched=1;
			sy2->is_fwd=1;
			sy2->fwd = sdp->forward;
			if(sdp->forward->rtype!=NULL){
				sy2->proc_rtype = search_all_type_tables(sdp->forward->rtype->id );
			}
			ht_insert(sdp->forward->fname, sy2, current_sp->proc_table);

			break;

		/* before going into proc's body, don't have to worry about scope */
		case AST_NT_PROC:
			proc_lineno = get_proc_smallest_lineno(sdp->proc);

			/* verify if the proc's name is redefined */
			sy1 = (_symbol*)ht_search(sdp->proc->id, current_sp->proc_table);
			if(sy1!=NULL){ /* sy1 might be a proc which has the same name, or a forward-decl which has the same name */

				/* if found a forward decl, which has the same name as the proc and unmatched before,
				 * then continue to verify return type and parameters. not creating new scope during this verification. */
				if(sy1->fwd_unmatched==1){

					/* verify parameter lists */
					if (sy1->fwd->sdl != NULL && sdp->proc->sdl != NULL) {  /* if both have parameters */
						sdl1 = sdp->proc->sdl;
						sdl2 = sy1->fwd->sdl;

						while(sdl1!=NULL && sdl2!=NULL){ /* iterate over every parameter lists pair; not care parameter/return-var name redefinition, will do later. */
							/* for every id in each parameter list pair, check if id number match */
							i = j = 0;
							idl1=sdl1->idl;
							while(idl1!=NULL){
								i++;
								idl1=idl1->next;
							}
							idl2=sdl2->idl;
							while(idl2!=NULL){
								j++;
								idl2=idl2->next;
							}
							idl1=idl2=NULL;
							if(i!=j){
								sprintf(str_buffer, "line %d: parameter numbers in proc and forward not match\n", sdl1->lineno );
								semantic_error(str_buffer);
							}

							/* verify if parameter lists' types are the same; ignore if type is redefined array */
							sy2 = (_symbol*)search_all_type_tables(sdl1->typeid->id );
							if(sy2==NULL){ /* if proc's parameter type undefined before */
								sprintf(str_buffer, "line %d: undefined proc parameter's type, %s\n", sdl1->typeid->lineno,sdl1->typeid->id );
								semantic_error(str_buffer);
							}
							sy3 = (_symbol*)search_all_type_tables(sdl2->typeid->id ); /* forward's parameter type should be checked ealier */
							proc_pt = get_root_type(sy2);
							fwd_pt = get_root_type(sy3);
							if (proc_pt == S_TYPE_UNKNOWN || fwd_pt == S_TYPE_UNKNOWN) {
								sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, %s %s\n",
										proc_lineno, sdl1->typeid->id,	sdl2->typeid->id);
								semantic_error(str_buffer);
							} else if (proc_pt != fwd_pt) {
								sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, %s %s\n",
										proc_lineno, sdl1->typeid->id,	sdl2->typeid->id);
								semantic_error(str_buffer);
							}

							/* verify if parameter lists' types are the same; if both types are array, check dimensions;
							 * proc's parameter type cannot be explicit array, proc t1(a:int[2]) end, is syntactic error. */
							proc_arr_d = get_typeid_array_dimensions(sdl1->typeid->id);
							fwd_arr_d = get_typeid_array_dimensions(sdl2->typeid->id);

							if( proc_arr_d==NULL && fwd_arr_d==NULL ){ /* correct */
							}else if ( proc_arr_d!=NULL && fwd_arr_d==NULL ){ /* error */
								sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, %s is an array, %s is not an array\n",
										proc_lineno, sdl1->typeid->id,	sdl2->typeid->id);
								semantic_error(str_buffer);
							}else if( proc_arr_d==NULL && fwd_arr_d!=NULL ){ /* error */
								sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, %s is not an array, %s is an array\n",
										proc_lineno, sdl1->typeid->id,	sdl2->typeid->id);
								semantic_error(str_buffer);
							}else{ /* further check it two arrays' dimension match */
								d1 = get_array_d_num(proc_arr_d);
								d2= get_array_d_num(fwd_arr_d);
								if(d1!=d2){
									sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, "
											"%s is a %d dimension array, but %s is a %d dimension array\n",
											proc_lineno, sdl1->typeid->id, d1, sdl2->typeid->id, d2);
									semantic_error(str_buffer);
								}
								for(i=0; i< d1; i++){  /* check every dimension size */
									if(proc_arr_d[i]!=fwd_arr_d[i]){
										sprintf(str_buffer,	"line %d: unmatched parameter-types of proc and forward, "
												"the size of the NO. %d dimension of %s and %s not match, %d not match %d\n",
												proc_lineno, i, sdl1->typeid->id, sdl2->typeid->id, proc_arr_d[i], fwd_arr_d[i]);
										semantic_error(str_buffer);
									}
								} /* end for */
							} /* end of if-else */

							sdl1=sdl1->next;
							sdl2=sdl2->next;
						} /* end of while */

						if(sdl1!=NULL && sdl2==NULL){
							sprintf(str_buffer,	"line %d: proc has more parameters than forward\n", proc_lineno);
							semantic_error(str_buffer);
						}else if(sdl1==NULL && sdl2!=NULL){
							sprintf(str_buffer,	"line %d: proc has less parameters than forward\n", proc_lineno);
							semantic_error(str_buffer);
						}

						free(proc_arr_d);
						free(fwd_arr_d);
					} else if (sy1->fwd->sdl == NULL && sdp->proc->sdl != NULL) {
						sprintf(str_buffer,	"line %d: unmatched parameters, "
										"proc has parameter, but forward doesn't have\n",proc_lineno);
						semantic_error(str_buffer);
					} else if (sy1->fwd->sdl != NULL && sdp->proc->sdl == NULL) {
						sprintf(str_buffer, "line %d: unmatched parameters, "
										"proc doesn't have parameter, but forward has\n",proc_lineno);
						semantic_error(str_buffer);
					}  /* end verify parameters list */

					/* verify return types; proc's return type must be basic type */
					if(sy1->fwd->rtype!=NULL && sdp->proc->rtype!=NULL){
						/* verify if proc's return-type is defined before, must be pre-defined */
						sy2 = (_symbol*)search_all_type_tables(sdp->proc->rtype->id );
						if(sy2==NULL){
							sprintf(str_buffer, "line %d: undefined proc's return type, %s\n", sdp->proc->rtype->lineno, sdp->proc->rtype->id );
							semantic_error(str_buffer);
						}

						/* verify if proc's return type is basic */
						if( is_type_array( sy2 ) ){
							sprintf(str_buffer, "line %d: proc's return type, %s, is not basic type\n", sdp->proc->rtype->lineno, sdp->proc->rtype->id );
							semantic_error(str_buffer);
						}

						/* verify if proc and forward return types match */
						proc_rt= get_root_type( sy2 );
						sy3 = (_symbol*)search_all_type_tables( sy1->fwd->rtype->id );
						fwd_rt = get_root_type( sy3 );


						if( fwd_rt==S_TYPE_UNKNOWN || proc_rt==S_TYPE_UNKNOWN ){
							sprintf(str_buffer, "line %d: unmatched return-types of proc and forward, %s %s\n", proc_lineno, sdp->proc->rtype->id, sy1->fwd->rtype->id );
							semantic_error(str_buffer);
						}else if( fwd_rt!=proc_rt ){
							sprintf(str_buffer, "line %d: unmatched return-types of proc and forward, %s %s\n", proc_lineno, sdp->proc->rtype->id, sy1->fwd->rtype->id );
							semantic_error(str_buffer);
						}

					}else if(sy1->fwd->rtype==NULL && sdp->proc->rtype!=NULL){
						sprintf(str_buffer, "line %d: unmatched return-type, "
								"proc has a return-type(%s), but forward doesn't have a return-type\n", proc_lineno, sdp->proc->rtype->id );
						semantic_error(str_buffer);
					}else if(sy1->fwd->rtype!=NULL && sdp->proc->rtype==NULL){
						sprintf(str_buffer, "line %d: unmatched return-type, "
								"proc doesn't have return-type, but forward has a return-type(%s)\n", proc_lineno, sy1->fwd->rtype->id );
						semantic_error(str_buffer);
					}

					sy1->fwd_unmatched=0; /* proc's parameter/return-type matched with forward, mark the forward as match */
					ht_delete(sy1->name, sy1, current_sp->proc_table); /* delete matched forward; for check_exp()'s proc-call, or segment fault */
				}else{ /* else, the found name is a proc, then redefined proc name error */
					sprintf(str_buffer, "line %d: redefined proc name, %s\n", proc_lineno, sdp->proc->id );
					semantic_error(str_buffer);
				}
			} /* end, verify if the proc's name is redefined */


			/* then create new scope for this proc, and insert any parameters and return-value into var-table;
			 * because there might not be corresponding forward-decl, so the above verification code might not
			 * be executed, thus still need to verify parameter/return type */
			scp1 = n_scope(SCOPE_PROC, HT_ARRAY_SIZE_S, 1, 1, 0);
			stk_push(scp1, ice9_engine->scope_stack);  /* push */
			current_sp = scp1;

			sdl1 = sdp->proc->sdl; /* verify parameter list, and update var-table */
			while(sdl1!=NULL){
				idl = sdl1->idl;

				sy1 = search_all_type_tables(sdl1->typeid->id);
				/* printf("-----%s-----%s--------\n", sdp->proc->id, sdl1->typeid->id); */
				if(sy1==NULL){
					sprintf(str_buffer, "line %d: undefined proc parameter's type, %s\n", sdl1->typeid->lineno,sdl1->typeid->id );
					semantic_error(str_buffer);
				}

				while(idl!=NULL){
					sy2 = (_symbol*)ht_search(idl->id, current_sp->var_table); /* check redefinition of parameters */
					if(sy2!=NULL){
						sprintf(str_buffer, "line %d: redefinition of proc's parameter %s\n", sdl1->lineno, idl->id );
						semantic_error(str_buffer);
					}

					sy2 = n_symbol(idl->id);
					sy2->var_type=sy1; /* record this var's type */
					ht_insert(idl->id, sy2, current_sp->var_table);

					idl=idl->next;
				}

				sdl1=sdl1->next;
			}

			if(sdp->proc->rtype!=NULL){ /* verify return-type, and update var-list */
				sy1 = search_all_type_tables( sdp->proc->rtype->id );
				if(sy1==NULL){
					sprintf(str_buffer, "line %d: undefined proc's return type, %s\n", sdp->proc->rtype->lineno, sdp->proc->rtype->id );
					semantic_error(str_buffer);
				}

				/* verify if proc's return type is basic */
				if( is_type_array( sy1 ) ){
					sprintf(str_buffer, "line %d: proc's return type, %s, is not basic type\n", sdp->proc->rtype->lineno, sdp->proc->rtype->id );
					semantic_error(str_buffer);
				}

				/* implicit return var's name is the same as the proc's name;
				 * check if one of parameter name is identical to the proc's name, then name clash */
				sy2 = (_symbol*)ht_search( sdp->proc->id, current_sp->var_table);
				if( sy2!=NULL ){
					sprintf(str_buffer, "line %d: redefinition, parameter %s cannot not have the same name as the implicit return variable\n",
							sdp->proc->rtype->lineno, sdp->proc->id );
					semantic_error(str_buffer);
				}

				sy2 = n_symbol( sdp->proc->id );
				sy2->var_type=sy1;
				ht_insert( sdp->proc->id, sy2, current_sp->var_table);
			}

			/* insert proc's info to previous proc-table; this will enable recursive self calls;
			 * don't have to worry if proc's body doesn't pass verification and pop out proc scope, since the program exit if find an error */
			sy3 = n_symbol( sdp->proc->id );
			if(sdp->proc->rtype!=NULL)
				sy3->proc_rtype=sy1 ; /* add return type info */
			sy3->proc = sdp->proc;  /* add proc info */
			sy3->is_proc=1;
			scope_bak = stk_pop(ice9_engine->scope_stack); /* backup previously pushed proc-scope */
			ht_insert( sdp->proc->id, sy3, ((_scope*)stk_peek(ice9_engine->scope_stack))->proc_table ); /* insert to proc table */
			stk_push(scope_bak, ice9_engine->scope_stack); /* push back proc scope again */

			/* then verify proc's body */
			if(sdp->proc->proc_decl!=NULL){ /* verify proc's stmt declaration: type/var */
				check_proc_decl(sdp->proc->proc_decl);
			}
			if(sdp->proc->ss!=NULL){ /* verify proc's statements */
				check_stmt_set(sdp->proc->ss);
			}

			/* after verification of proc's body, pop the proc's scope */
			stk_pop(ice9_engine->scope_stack);
			current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);  /* update current_sp */

			break;
		} /* end of switch */

		sdp = sdp->next;
	}  /* end of while */
}

void check_stmt(_ast_stmt * sp){
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;
	_ast_forward * fw;
	_ast_subdeclist * sdl;
	_ast_exp * exp;

	_scope * current_scope = NULL, * fa_scope=NULL;
	_symbol * sy1 = NULL;
	int * ass_arr_d=NULL;  /* assignment's left side's array dimension */
	int i=0;
	int e1=0, e2=0;
	int ass_arr_d_num=0;

	current_scope = (_scope*)stk_peek(ice9_engine->scope_stack);
	while (sp != NULL) {
		switch (sp->ntype) {
		case AST_NT_EXP:
			check_exp(sp->exp);
 			break;
		case AST_NT_ASSIGN:
			/* verify assignment left part's id variable, must be defined before */
			sy1 = search_all_var_tables(sp->assign->id);
			if(sy1==NULL){
				sprintf(str_buffer, "line %d: undefined variable, %s\n", sp->assign->lineno, sp->assign->id );
				semantic_error(str_buffer);
			}

			if(sy1->is_fa_loop_var==1){
				sprintf(str_buffer, "line %d: fa's loop variable, %s, is not assignable in the body of fa\n", sp->assign->lineno, sp->assign->id );
				semantic_error(str_buffer);
			}

			//printf("--------- AST_NT_ASSIGN, id:%s, type:%s \n", sp->assign->id, sy1->var_type->name);

			/* if the left part is not an array operation, but the var's type is actually an array, --> error */
			if(sp->assign->lexp==NULL && is_var_array( sy1 ) ){
				sprintf(str_buffer, "line %d: invalid array operation in assignment, only the last dimension of the array %s is assignable\n", sp->assign->lineno, sp->assign->id );
				semantic_error(str_buffer);
			}

			/* if the left part is an array operation, then check only elem in the last dimension of the array is assignable */
			if(sp->assign->lexp!=NULL){
				ass_arr_d = get_var_array_dimensions( sy1 ); /* var type's total dimension */

				i=0;
				exp = sp->assign->lexp;
				while (exp != NULL) {
					i++;  /* to find out this assignment's final dimension number */
					exp=exp->next;
				}
				//printf("-------- i:%d, ass_arr_d:%d\n", i, get_array_d_num(ass_arr_d));
				ass_arr_d_num = get_array_d_num(ass_arr_d);
				if( i< ass_arr_d_num ){
					sprintf(str_buffer, "line %d: invalid array operation in assignment, only the last dimension of the array %s is assignable\n", sp->assign->lineno, sp->assign->id );
					semantic_error(str_buffer);
				}
				if( i > ass_arr_d_num ){
					if(ass_arr_d_num==0)
						sprintf(str_buffer, "line %d: invalid array operation in assignment, %s is not an array\n", sp->assign->lineno, sp->assign->id );
					else
						sprintf(str_buffer, "line %d: invalid array operation in assignment, array %s only has %d dimension(s)\n", sp->assign->lineno, sp->assign->id, ass_arr_d_num );
					semantic_error(str_buffer);
				}

				free(ass_arr_d);
			}

			/* if the left part is an array operation, but the var's type is not an array, --> error
			if( sp->assign->lexp!=NULL && is_var_array(sy1 )==0 ){
				sprintf(str_buffer, "line %d: invalid array operation in assignment, %s is not an array\n", sp->assign->lineno, sp->assign->id );
				semantic_error(str_buffer);
			} */

			/* if left side is array, make sure every array index is int;
			 * this must be check specially in assignment-stmt, because in my grammar, assignment left side is not defined as exp; */
			if(sp->assign->lexp!=NULL){
				exp = sp->assign->lexp;
				i=0;
				while (exp != NULL) {
					e1 = check_exp(exp);
					if(e1!=S_TYPE_INT_BASIC){
						sprintf(str_buffer, "line %d: invalid array operation in assignment, array %s's NO. %d dimension index exp must be int\n", exp->lineno, sp->assign->id, i);
						semantic_error(str_buffer);
					}
					i++;
					exp=exp->next;
				}
			}

			/* check if left and right sides match */
			e1 = get_root_type( sy1->var_type );  /* left side is already guaranteed to be the last dimension of an array */
			e2 = check_exp(sp->assign->rexp);
			if( (e1==e2) && e1!=S_TYPE_UNKNOWN ){
			}else{
				sprintf(str_buffer, "line %d: incompatible types in assignment\n", sp->assign->lineno );
				semantic_error(str_buffer);
			}

			break;
		case AST_NT_WRITE:
			e1 = check_exp(sp->write->exp);
			if(e1!=S_TYPE_INT_BASIC && e1!=S_TYPE_STRING_BASIC){
				sprintf(str_buffer, "line %d: write statement's exp must be int or string\n", sp->write->lineno );
				semantic_error(str_buffer);
			}
			break;
		case AST_NT_BREAK:
			if(current_scope->stype!=SCOPE_FA && do_count==0){
				sprintf(str_buffer, "line %d: misplaced break, not in fa nor in do\n", sp->lineno );
				semantic_error(str_buffer);
			}
			break;
		case AST_NT_EXIT:
			break;
		case AST_NT_RETURN:
			break;
		case AST_NT_DO:
			do_count++;

			/* do's exp must be bool */
			e1=check_exp(sp->ddo->exp);
			if(e1!=S_TYPE_BOOL_BASIC){
				sprintf(str_buffer, "line %d: do's exp must be boolean\n", sp->ddo->exp->lineno );
				semantic_error(str_buffer);
			}

			/* verify fa's body */
			if(sp->ddo->ss!=NULL){
				check_stmt_set(sp->ddo->ss);
			}

			do_count--;
			break;
		case AST_NT_FA:
			/* create fa scope */
			fa_scope = n_scope(SCOPE_FA, HT_ARRAY_SIZE_T, 0, 1, 0);
			stk_push(fa_scope, ice9_engine->scope_stack);
			current_scope = fa_scope;

			/* insert fa loop variable into var-table */
			sy1 = n_symbol("int");  /*bug?? should be: sp->fa->id ??*/
			sy1->var_type = search_all_type_tables("int");
			sy1->is_fa_loop_var=1;
			ht_insert(sp->fa->id, sy1, current_scope->var_table);

			/* verify fa's range exps */
			e1 = check_exp(sp->fa->fexp);
			if (e1 != S_TYPE_INT_BASIC) {
				sprintf(str_buffer, "line %d: fa's start-condition exp must be int\n", sp->fa->fexp->lineno);
				semantic_error(str_buffer);
			}
			e2 = check_exp(sp->fa->texp);
			if (e2 != S_TYPE_INT_BASIC) {
				sprintf(str_buffer, "line %d: fa's end-condition exp must be int\n", sp->fa->texp->lineno);
				semantic_error(str_buffer);
			}

			/* verify fa's body */
			if(sp->fa->ss!=NULL){
				check_stmt_set(sp->fa->ss);
			}

			stk_pop( ice9_engine->scope_stack ); /* pop current fa scope */
			current_scope = (_scope*)stk_peek( ice9_engine->scope_stack ); /* restore previous scope */
			break;
		case AST_NT_IF:

			/* if's exp must be bool */
			e1=check_exp(sp->iif->exp);
			if (e1 != S_TYPE_BOOL_BASIC) {
				sprintf(str_buffer, "line %d: if's condition exp must be boolean\n", sp->iif->exp->lineno);
				semantic_error(str_buffer);
			}

			check_stmt_set( sp->iif->iss );

			if( sp->iif->bss!=NULL) {
				check_box_stmt_set(sp->iif->bss);
			}

			if(sp->iif->ess!=NULL){
				check_stmt_set( sp->iif->ess );
			}

			break;
		} /* end of switch */
		sp=sp->next;
	} /* end of while */
}

static void init_semantic_check(){
	_scope * global_sp = NULL, * default_sp=NULL;

	ice9_engine->scope_stack = stk_create();

	default_sp = n_scope(SCOPE_DEFAULT, HT_ARRAY_SIZE_T, 1, 0, 0);
	ht_insert("int", n_symbol("int"), default_sp->type_table);
	ht_insert("bool", n_symbol("bool"), default_sp->type_table);
	ht_insert("string", n_symbol("string"), default_sp->type_table);

	stk_push(default_sp,ice9_engine->scope_stack);  /* push into default scope */

	global_sp = n_scope(SCOPE_GLOBAL, HT_ARRAY_SIZE_M, 1, 1, 1);
	stk_push(global_sp,ice9_engine->scope_stack);  /* push into global scope */
}

static void clean_semantic_check(){
	_scope * t_scope = NULL;

	while( stk_getsize(ice9_engine->scope_stack)!=0 ){
		t_scope = stk_pop(ice9_engine->scope_stack);

		ht_destruct(t_scope->proc_table);
		ht_destruct(t_scope->type_table);
		ht_destruct(t_scope->var_table);
	}

	stk_destruct(ice9_engine->scope_stack);
	ice9_engine->scope_stack=NULL;
}

void semantic_check_ast(){
	_symbol * sy=NULL;

	init_semantic_check(); /* setup scope stack and symbol tables */

	reset_str_buffer();
	check_stmt_decl(ice9_engine->ast->stmt_declarations);

	sy = (_symbol*)ht_search_unmatched_fwd( ((_scope*)stk_peek( ice9_engine->scope_stack ))->proc_table );
	if(sy!=NULL){
		sprintf(str_buffer, "line %d: unmatched forward, %s\n", sy->fwd->lineno, sy->fwd->fname);
		semantic_error(str_buffer);
	}
	check_stmt(ice9_engine->ast->stmts);

	clean_semantic_check(); /* cleanup */
}
