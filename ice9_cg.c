/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * Code generation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ice9.h"

#define FP_OFFSET 300

extern _ice9_core * ice9_engine;

static char str_buffer[STR_BUFFER_SIZE];
static char * fname=NULL; /* file name of the generated.tm */
static FILE * to_file=NULL;
static int i_count=0; /* instruction count */
static int d_count=1; /* data count; dMem[0] is the total length of dMem */
static int brk_flag_count=1; /* used to uniquely identify breaks for do/fa */
static _ast_proc * temp_proc=NULL; /*for CG of local var; when enter a proc, update this pointer to that proc, when leave, set to NULL*/
static int rt_flag_count=1; /*used to uniquely identify returns for a proc*/
static int exec_entrance=0; /*because there might be proc's definition, program cannot start executing at the beginning, instruction at this iMem will jump to the first stmt*/
static int arr_err_msg_start_dMem=0;  /*start dMem of array error msg*/
static int arr_err_msg_len=0;  /*length of error msg*/
static int arr_err_msg_index_offset=0;  /*the offset of dMem where to place the actual error index*/
static int arr_err_proc_start_iMem=0;
static int str_wproc_start_iMem=0;

void stmt_cg(_ast_stmt * sp, int brk_flag, int * brk_imem, int * rt_imem);
void stmt_set_cg(_ast_stmt_set * ssp, int brk_flag, int * brk_imem, int * rt_imem);
void count_stmt_set_break(_ast_stmt_set * ssp, int * count, int brkcount);

/* func in ice9_semantic.c */
_symbol * search_all_var_tables(char * id);
_symbol * search_all_type_tables(char * id);
_symbol * search_all_proc_tables(char * id);
int get_root_type(_symbol * sy);
int * get_var_array_dimensions(_symbol * sy);
int get_array_d_num(int * arr);

/*
 * input: an integer array, eg. get_var_array_dimensions(), get_typeid_array_dimensions().
 * output: totally number of elements of the array(the sum of all dimensions)
 */
static int get_array_dim_sum(int * arr){
	int i=0, sum=1;

	if(arr==NULL){
		return 0;
	}

	for(i=0;i<ARR_MAX_DIMENSION;i++){
		if(arr[i]==-1)
			break;

		sum*=arr[i];
	}

	return sum;
}

static void reset_str_buffer(){
	memset(str_buffer, 0, sizeof(char)*STR_BUFFER_SIZE);
}

static int get_current_icount(){
	return i_count;
}

/* start from 0; on every call, return current value, and increase icount */
static int get_next_icount(){
	int i=0;
	i=i_count;
	i_count++;
	return i;
}

static void update_dcount(int offset){
	d_count+=offset;
}

static int get_current_dcount(){
	return d_count;
}

static int get_next_dcount(){
	int d=0;
	d=d_count;
	d_count++;
	return d;
}

static int get_next_brkcount(){
	int b=0;
	b=brk_flag_count;
	brk_flag_count++;
	return b;
}

static int get_next_rtcount(){
	int b=0;
	b=rt_flag_count;
	rt_flag_count++;
	return b;
}

static void write_tmf(const char * s){
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "write_tmf, s is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}
	fputs(s, to_file);
	fflush( to_file );
}

/**
 * add "*" before s, add "\n" after s
 */
static void emit_comment(char * s){
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "emit_comment, s is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	write_tmf(s);
}

/* RegisterOnly code generation */
static void ro_cg(const char * ins, int r, int s, int t, const char * comment){
	if(ins==NULL || comment==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "ro_cg, ins or comment is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	reset_str_buffer();
	sprintf(str_buffer, "%d: %s %d, %d, %d		*%s\n", get_next_icount(), ins, r, s, t, comment);
	write_tmf(str_buffer);
}

/* Register To Memory code generation, with specified iMem as argument; used in condition where jump location not know immediately, such as if-else */
static void rm_cg_i(int iMem, const char * ins, int r, int d, int s, const char * comment){
	if(ins==NULL || comment==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "rm_cg_i, ins or comment is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	reset_str_buffer();
	sprintf(str_buffer, "%d: %s %d, %d(%d)		*%s\n", iMem, ins, r, d, s, comment);
	write_tmf(str_buffer);
}

/* Register To Memory code generation */
static void rm_cg(const char * ins, int r, int d, int s, const char * comment){
	if(ins==NULL || comment==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "rm_cg, ins or comment is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	rm_cg_i(get_next_icount(), ins, r, d, s, comment);
}

/* for binary operation in exp, after push and pop, right-exp's value in AC1, left-exp's value in AC2 */
/* temp memory stack push.
  	CG(L)
	LDA SP, -1(SP)	*push, update stack-top pointer
	ST AC, 0(SP)	*push, push in the value
*/
static void tm_stack_push(){
	rm_cg("LDA", REG_SP, -1, REG_SP, "tm stack push1");
	rm_cg("ST", REG_AC1, 0, REG_SP, "tm stack_push2");
}

/* temp memory stack pop. store value to AC2, for binary exp operation.
	CG(R)
	LD AC2, 0(SP)	*pop, pop out the value
	LDA SP, 1(SP)	*pop, update the stack-top pointer
*/
static void tm_stack_pop(){
	rm_cg("LD", REG_AC2, 0, REG_SP, "tm stack pop_ac2 1");
	rm_cg("LDA", REG_SP, 1, REG_SP, "tm stack pop_ac2 2");
}

/* temp memory stack pop. store value to AC1, not for binary exp operation. used in conditions, such as if-else.
	CG(R)
	LD AC1, 0(SP)	*pop, pop out the value
	LDA SP, 1(SP)	*pop, update the stack-top pointer
*/
static void tm_stack_pop_ac1(){
	rm_cg("LD", REG_AC1, 0, REG_SP, "tm stack pop_ac1 1");
	rm_cg("LDA", REG_SP, 1, REG_SP, "tm stack pop_ac1 2");
}

/**
 * temp memory stack peek. put it to REG_AC1
 */
static void tm_stack_peek(){
	rm_cg("LD", REG_AC1, 0, REG_SP, "tm stack peek1");
}

static void init_cg(char * fn){
	_scope * global_sp = NULL, * default_sp=NULL;
	int i=0, len=0;
	char * arr_error_msg = ".SDATA \"ArrayOutOfBoundsException: Array Index  is out of bounds\"\n";
	char * arr_error_msg1 = "ArrayOutOfBoundsException: Array Index  is out of bounds";
	char * arr_error_msg2 = "ArrayOutOfBoundsException: Array Index ";

	/* init scope and symbol tables */
	ice9_engine->scope_stack = stk_create();
	default_sp = n_scope(SCOPE_DEFAULT, HT_ARRAY_SIZE_T, 1, 0, 0);
	ht_insert("int", n_symbol("int"), default_sp->type_table);
	ht_insert("bool", n_symbol("bool"), default_sp->type_table);
	ht_insert("string", n_symbol("string"), default_sp->type_table);
	stk_push(default_sp,ice9_engine->scope_stack);  /* push into default scope */

	global_sp = n_scope(SCOPE_GLOBAL, HT_ARRAY_SIZE_M, 1, 1, 1);
	stk_push(global_sp,ice9_engine->scope_stack);  /* push into global scope */

	/* init output file */
	fname = fn;
	to_file = fopen(fname, "wb+");

	/*init dMem for array out of bound error:*/
	//printf("-------------%d\n", get_current_dcount() );
	write_tmf(arr_error_msg);  /*generate .SDATA section*/
	arr_err_msg_start_dMem = get_current_dcount();
	arr_err_msg_len=strlen(arr_error_msg1);
	arr_err_msg_index_offset=strlen(arr_error_msg2); /*the offset to place the actual error index value*/
	update_dcount(arr_err_msg_len);
	//printf("-------------%d\n", get_current_dcount() );
	/* generate init code */
	rm_cg("LD", REG_SP, 0, REG_ZERO, "init stack pointer"); /* update stack pointer, let REG_SP points to the last cell of dMem */
	rm_cg("LD", REG_FP, 0, REG_ZERO, "init frame pointer1"); /*update frame pointer, 2 dMem ahead of SP*/
	rm_cg("LDA", REG_FP, -FP_OFFSET, REG_FP, "init frame pointer2");

	exec_entrance = get_next_icount();

	/*redefined proc to handle array out of bound error*/
#if ICE9_DEBUG
	emit_comment("*---> start of invalid array bound handling proc\n");
#endif
	/*init iMem for array out of bound error; define a specific proc to handle this kind of error:*/
	arr_err_proc_start_iMem = get_current_icount();
	rm_cg("LDC", REG_T1, arr_err_msg_len, 0, "load str length");  /*str length in T1*/
	rm_cg("LDC", REG_T2, arr_err_msg_start_dMem, 0, "load str base"); /*str base in T2*/
	/*
	 * during the output of the array error msg, all elem of the msg should be output using OUTC, except the integer index, so have to specially treat this case.
	 * if (arr_err_msg_start_dMem+arr_err_msg_index_offset)==str_base, it is the integer index
	 */
	rm_cg("LD", REG_AC1, 0, REG_T2, "load str elem"); /*load str's actual char value*/
	rm_cg("LDC", REG_AC2, arr_err_msg_start_dMem+arr_err_msg_index_offset, 0, "load arr_err_msg_start_dMem+arr_err_msg_index_offset");
	ro_cg("SUB", REG_AC2, REG_AC2, REG_T2, "do sub for jump"); /*do actual sub*/
	rm_cg("JEQ", REG_AC2, 2, REG_PC, "jump to print integer index?");  /*if should use OUT to print the actual error index*/
	ro_cg("OUTC", REG_AC1, 0, 0, "output char");  /*output a char in str msg*/
	rm_cg("LDA", REG_PC, 1, REG_PC, "finish outputing char, skip output integer index");
	ro_cg("OUT", REG_AC1, 0, 0, "output error integer index");

	rm_cg("LDA", REG_T1, -1, REG_T1, "decrease str length");
	rm_cg("LDA", REG_T2, 1, REG_T2, "increase str base");
	rm_cg("JGT", REG_T1, -10, REG_PC, "more chars?");
	ro_cg("OUTNL", 0, 0, 0, "out new line");
	rm_cg("LD", REG_PC, 0, REG_ZERO, "terminate, jump to last iMem");
#if ICE9_DEBUG
	emit_comment("*---> end of invalid array bound handling proc\n");
#endif


	/*redefined proc to handle string write; base in AC1, string len in AC2, return addr is in T1*/
#if ICE9_DEBUG
	emit_comment("*---> start of string write proc\n");
#endif
	str_wproc_start_iMem = get_current_icount();
	rm_cg("LD", REG_T2, 0, REG_AC1, "load str char value to T2");
	ro_cg("OUTC", REG_T2, 0, 0, "string char output");
	ro_cg("LDA", REG_AC1, 1, REG_AC1, "increase str base");
	ro_cg("LDA", REG_AC2, -1, REG_AC2, "decrease str len");
	rm_cg("JGT", REG_AC2, -5, REG_PC, "more chars?");
	rm_cg("LDA", REG_PC,0, REG_T1, "finish writing str, jump back");
#if ICE9_DEBUG
	emit_comment("*---> end of string write proc\n");
#endif

	//printf("-------------%d\n", get_current_dcount() );
}

static void clean_cg(){
	_scope * t_scope = NULL;

	while( stk_getsize(ice9_engine->scope_stack)!=0 ){
		t_scope = stk_pop(ice9_engine->scope_stack);

		ht_destruct(t_scope->proc_table);
		ht_destruct(t_scope->type_table);
		ht_destruct(t_scope->var_table);
	}

	stk_destruct(ice9_engine->scope_stack);
	ice9_engine->scope_stack=NULL;

	fclose(to_file);
}

/* binary conditional operation's result is 1(true) or 0(false), no other value. */
int exp_cg(_ast_exp * exp){
	_ast_exp * te=NULL, * te1=NULL;
	int e1=0, e2=0, i=0, arr_total_d=0, root_type=0, j=0, k=0;
	_symbol * sy1=NULL, *sy2=NULL, *sy3=NULL;
	_ast_subdeclist * sdl=NULL;
	_ast_idlist * idl;
	//int * type_arr_d1=NULL, * type_arr_d2=NULL;
	int d1=0, d2=0;
	int tvalue=0;
	int temp_offset=0;
	int rt_offset=0; /*to assist determining return addr iMem*/

	char lbuf[STR_BUFFER_SIZE];
	int * arr_dims=NULL;
	//int arr_dim_sum=0;

	if(exp==NULL)
		return S_TYPE_UNKNOWN;

	switch(exp->ntype){
	case AST_NT_LVALUE:  /*LVALUE to determine the dMem of an identifier*/
		sy1=search_all_var_tables(exp->id);  /* get identifier's info */
		root_type = get_root_type(sy1->var_type);

		if(sy1->cgp==NULL){ /*every identifier should have an associated symbol*/
#if ICE9_DEBUG
			ice9_log_level(ice9_engine->lgr, "exp_cg, LVALUE, cgp is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		}

		if(sy1->cgp->local==1){  /*if local, get related proc info*/
			sy2 = search_all_proc_tables( sy1->cgp->lvar_proc->id );  /*get the proc's symbol*/
			if(sy2==NULL){
#if ICE9_DEBUG
				ice9_log_level(ice9_engine->lgr, "exp_cg, LVALUE, cannot find local var's proc symbol", ICE9_LOG_LEVEL_ERROR);
#endif
			}
		}

		/*the outter if-else, check if array; the inner if or else, check if local var*/
		if(exp->lexp==NULL){  /* identifier not array */
			if(sy1->cgp->local==0){  /*if not proc's local var*/
				rm_cg("LD", REG_AC1, sy1->cgp->d_index, REG_ZERO, "LVALUE, GLOBAL, put value from dMem to AC1");
			}else{  /*local var*/

				memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
				sprintf(lbuf, "LVALUE, put LOCAL(%s) to REG_AC1", exp->id);
				rm_cg("LD", REG_AC1, (sy2->cgp->ar_size - sy1->cgp->lvar_fp_offset), REG_FP, lbuf); /*FP+(AR-offset)*/
			}

		}else{ /*array*/
			arr_dims = get_var_array_dimensions(sy1); /*get all dimension size*/
			arr_total_d = get_array_d_num(arr_dims);

			i=0;
			te=exp->lexp;

			rm_cg("LDC", REG_AC1, 0, 0, "LVALUE, init array elem offset");
			tm_stack_push();  /*save offset*/

			while(te!=NULL){

				exp_cg(te);  /*evaluate specified dimension size in the LVALUE, in AC1*/
				//rm_cg("LDA", REG_T1, 0, REG_AC1, "LVALUE, backup AC1 to T1");
				rm_cg("JLT", REG_AC1, 6, REG_PC, "LVALUE, if specified index is negative, jump to error handler" );
				tm_stack_push();/*temporarily save this specified dimension size*/
				rm_cg("LDC", REG_AC2, arr_dims[i], 0, "LVALUE, get the defined size for an array's dim 1"); /*defined size of the i-th dimension is now in AC2*/

				ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "LVALUE, specified dim - defined dim");
				rm_cg("JLT", REG_AC1, 3, REG_PC, "LVALUE, valid array index, skip error routine");
				tm_stack_peek();
				rm_cg("ST", REG_AC1, arr_err_msg_start_dMem+arr_err_msg_index_offset, REG_ZERO, "LVALUE, load invalid index to error msg"); /*load invalid array index to error msg*/
				rm_cg("LDC", REG_PC, arr_err_proc_start_iMem, REG_ZERO, "LVALUE, jump to error routine");/*call pre-defined proc to process array-out-of-bound error, this proc will not return, just print error msg and exit*/


				/* the d_index in the symbol for array var, denotes the first dMem of this array
				 * var a: int[5]; a[3]; -->offset:3
				 * var a: int[4][5]; a[3][4]; -->offset: 3*4+4
				 * var a: int[5][6][7]; a[4][5][6]; -->offset: 4*5+5*6+6
				 */
				tm_stack_pop(); /*pop previously evaluated specified dimension size to AC2*/
				if( (i+1)<arr_total_d ){  /*if current index is not the last dimension.*/
					rm_cg("LDC", REG_AC1, arr_dims[i], 0, "LVALUE, get the defined size for an array's dim 2"); /*defined size of the i-th dimension is now in AC1*/
					ro_cg("MUL", REG_AC1, REG_AC1, REG_AC2, "LVALUE, mul to get part of array offset"); /*new part of offset in AC1*/
					tm_stack_pop(); /*pop previously saved offset to AC2*/
					ro_cg("ADD", REG_AC1, REG_AC1, REG_AC2, "LVALUE, update offset");

				}else{
					tm_stack_pop_ac1(); /*pop previously saved offset to AC1*/
					ro_cg("ADD", REG_AC1, REG_AC1, REG_AC2, "LVALUE, update offset, last index");
				}
				tm_stack_push(); /*store updated offset*/

				i++;
				te=te->next;
			}  /*end of while, array final offset in temp stack*/

			tm_stack_pop(); /*pop array final offset to AC2*/


			if(sy1->cgp->local==0){  /*if not proc's local var*/
				rm_cg("LD", REG_AC1, sy1->cgp->d_index, REG_AC2, "LVALUE, GLOBAL, load final array_elem's dMem to AC1");
			}else{  /*local var*/

				/*FP+(AR - array_start_offset - array_elem_offset)*/
				ro_cg("SUB", REG_AC2, REG_FP, REG_AC2, "LVALUE, LOCAL, load final array_elem's dMem to AC1 1");
				rm_cg("LD", REG_AC1, (sy2->cgp->ar_size - sy1->cgp->lvar_fp_offset), REG_AC2, "LVALUE, LOCAL, load final array_elem's dMem to AC1 2");
				//rm_cg("LD", REG_AC1, (sy2->cgp->ar_size - sy1->cgp->lvar_fp_offset), REG_FP, lbuf);
			}
		} /*end of if-array*/

		return root_type;
		break;
	case AST_NT_INT:
		rm_cg("LDC", REG_AC1, exp->ivalue, 0, "put Integer to AC1");
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_TRUE:
		rm_cg("LDC", REG_AC1, ICE9_BOOL_TRUE, 0, "put TRUE to AC1");
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_FALSE:
		rm_cg("LDC", REG_AC1, ICE9_BOOL_FALSE, 0, "put FALSE to AC1");
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_STR:

		return S_TYPE_STRING_BASIC;
		break;
	case AST_NT_READ:
		ro_cg("IN", REG_AC1, 0, 0, "read integer, put it to AC1"); /* put&return read integer to AC1 */
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_UMINUS:
		e1 = exp_cg(exp->rexp);

		if( e1==S_TYPE_INT_BASIC ){
			rm_cg("LDC", REG_T1, -1, 0, "UMINUS int, put -1 to REG_T1");
			ro_cg("MUL", REG_AC1, REG_AC1, REG_T1, "UMINUS int, mul with -1");

		}else if( e1==S_TYPE_BOOL_BASIC ){
			rm_cg("JEQ", REG_AC1, 3-1, REG_PC, "UMINUS bool, Step1"); /* if the given bool is not false(not integer 0), then continue to the following line, else jump to iMem[PC+3] */
			/* given boolean is true */
			rm_cg("LDA", REG_AC1, -1, REG_AC1, "UMINUS bool, Step2");  /* then deduct 1 from the given value, should yield 0 */
			rm_cg("LDA", REG_PC, 2-1, REG_PC, "UMINUS bool, Step3");  /* then goto the first iMem after this UMINUS operation */

			/* given boolean is false */
			rm_cg("LDA", REG_AC1, 1, REG_AC1, "UMINUS bool, Step4");  /* then add 1 to the given value, should yield 0 */
		}

		return e1; /*return type*/
		break;
	case AST_NT_QUEST: /* change bool to int */
		/* put value in AC1; suppose ice9/TM only use 1 to denote true, 0 to denote false,
		 * and because TM uses integer to store boolean-value, so no need to do further conversion */
		exp_cg(exp->rexp);
		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_EPROC:
		temp_offset=1;
		sy1 = search_all_proc_tables(exp->id);  /*get the to be called proc's symbol*/
		if(sy1==NULL){
#if ICE9_DEBUG
			ice9_log_level(ice9_engine->lgr, "exp_cg, cannot find symbol for the called proc", ICE9_LOG_LEVEL_ERROR);
#endif
		}

		if( (te=exp->lexp)!=NULL ){ /*if has parameter*/
			while(te!=NULL){
				exp_cg(te);  /*evaluate exp, whose value will be in REG_AC1*/
				//ro_cg("OUT", REG_AC1, 0,0,"testing");
				rm_cg("ST", REG_AC1, -temp_offset, REG_FP, "proc-call, store parameters to current AR"); /*store parameter value to AR*/
				temp_offset++;

				te=te->next;
			}
		}

#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> to call proc %s\n", exp->id);
		emit_comment(lbuf);
#endif

		/*before update FP, "memset" this proc-call's AR range to 0, the range to hold parameters is already set above*/
		for(i=temp_offset;i<=sy1->cgp->ar_size;i++)
			rm_cg("ST", REG_ZERO, -i, REG_FP, "pro-call, init AR, set to 0");

		/*push on AR AFTER storing parameters, or when refering to a parameter, the FP is not original FP(it has changed to point to the called-proc) */
		rm_cg("LDA", REG_FP, -(sy1->cgp->ar_size), REG_FP, "proc-call, push in AR"); /*update FP, allocate space for the AR, REG_FP=REG_FP-AR*/
		//printf("----------proc call, sy1->cgp->ar_size:%d\n", sy1->cgp->ar_size);

		/*update return iMem addr, which is the "topest" iMem cell in the AR*/
		rt_offset=0;
		rt_offset=1+1+1; /* the return iMem is current iMem+3 */

		rm_cg("LDC", REG_T1, get_current_icount()+rt_offset, 0, "proc-call, get proc return iMem addr"); /*cannot directly store the value to AR, load to REG_AC1 first*/
		rm_cg("ST", REG_T1, 0, REG_FP, "proc-call, store proc return iMem addr to AR");  /*change 1->0??*/

		/*jump to the proc*/
		rm_cg("LDC", REG_PC, sy1->cgp->i_index, 0, "proc-call, jump to proc");

		/*update FP, REG_FP=REG_FP+AR, equivalent to pop out the AR*/
		rm_cg("LDA", REG_FP, sy1->cgp->ar_size, REG_FP, "proc-call, pop out AR");

		/* use proc's return value as */
		if( sy1->proc_rtype!=NULL )
			return get_root_type( sy1->proc_rtype );
		else
			return S_TYPE_UNKNOWN;

		break;
	case AST_NT_PLUS:
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_PLUS\n");
		emit_comment(lbuf);
#endif

		e1 = exp_cg(exp->lexp);
		if(e1==S_TYPE_BOOL_BASIC){j=get_next_icount();}
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		e2 = exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		/* don't have to worry boolean type, bool's plus is bool-or; in TM*/
		ro_cg("ADD", REG_AC1, REG_AC2, REG_AC1, "exp add");

		if(e1==S_TYPE_INT_BASIC)
			return S_TYPE_INT_BASIC;

		//tm_stack_push(); /*backup result*/
		rm_cg("LDC", REG_AC2, 1, 0, "PLUS, load constant 1 to AC2");
		ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "PLUS, REG_AC1 - 1");
		rm_cg("JGE", REG_AC1, 3-1, REG_PC, "PLUS, jump1"); /*if boolean plus, result>1, means true*/
		rm_cg("LDC", REG_AC1, 0, 0, "PLUS, load constant 0 to AC1");
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "PLUS, jump2");
		rm_cg("LDC", REG_AC1, 1, 0, "PLUS, load constant 1 to AC1");
		rm_cg_i(j, "JGT", REG_AC1, get_current_icount()-j-1, REG_PC, "PLUS, short circuit");j=0;
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_MINUS:
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_MINUS\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp sub");

#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> end of AST_NT_MINUS\n");
		emit_comment(lbuf);
#endif

		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_MUL:
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_MUL\n");
		emit_comment(lbuf);
#endif

		e1 = exp_cg(exp->lexp);
		if(e1==S_TYPE_BOOL_BASIC){j=get_next_icount();}
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		e2 = exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("MUL", REG_AC1, REG_AC2, REG_AC1, "exp mul");

#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> end of AST_NT_MUL\n");
		emit_comment(lbuf);
#endif

		if(e1==S_TYPE_INT_BASIC)
			return S_TYPE_INT_BASIC;
		rm_cg_i(j, "JEQ", REG_AC1, get_current_icount()-j-1, REG_PC, "MUL, short circuit");j=0;
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_DIV:
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_DIV\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("DIV", REG_AC1, REG_AC2, REG_AC1, "exp div");

		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_MOD:  /* a%b = a-(a/b)*b; */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_MOD\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp); /* compute a */
		tm_stack_push(); /* a's value in AC1, push it to temp stack */
		rm_cg("LDA", REG_T1, 0, REG_AC1, "exp-MOD, Step1");  /* put a's value to REG_T1 */
		exp_cg(exp->rexp); /* compute b */
		tm_stack_pop();  /* b's value in AC1, pop a's value to AC2 */
		rm_cg("LDA", REG_T2, 0, REG_AC1, "exp-MOD, Step2");  /* put b's value to REG_T2 */

		ro_cg("DIV", REG_AC1, REG_AC2, REG_AC1, "exp-MOD, Step3");  /* (a/b)'s value now in AC1 */
		tm_stack_push(); /* push (a/b)'s value to temp stack */
		rm_cg("LDA", REG_AC1, 0, REG_T2, "exp-MOD, Step4");  /* put b's value(previously stored in REG_T2) to AC1 */
		tm_stack_pop(); /* pop (a/b)'s value to AC2 */
		ro_cg("MUL", REG_AC2, REG_AC2, REG_AC1, "exp-MOD, Step5"); /* Now, AC1: b's value, AC2: (a/b)'s value; then do multiply, put result to AC2 */
		rm_cg("LDA", REG_AC1, 0, REG_T1, "exp-MOD, Step6"); /* put a's value(previously stored in REG_T1) to AC1 */
		ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "exp-MOD, Step7"); /* Now, AC1: a's value, AC2: (a/b)*b; then do sub, store the value to AC1 */

		return S_TYPE_INT_BASIC;
		break;
	case AST_NT_EQU: /* a==b,--> (a-b)==0? */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_EQU\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-EQU, Step1");

		rm_cg("JEQ", REG_AC1, 3-1, REG_PC, "exp-EQU, Step2"); /* if(a!=b), continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 0, 0, "exp-EQU, Step3"); /* a!=b, return 0 as false */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-EQU, Step4"); /* jump to the first iMem after this EQU operation */

		rm_cg("LDC", REG_AC1, 1,0, "exp-EQU, Step5"); /* a==b, return 1 as true */

		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_NE: /* a!=b,--> (a-b)!=0? */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_NE\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-NE, Step1");

		rm_cg("JEQ", REG_AC1, 3-1, REG_PC, "exp-NE, Step2"); /* if(a!=b), continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 1, 0, "exp-NE, Step3"); /* a!=b, return 1 as true */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-NE, Step4"); /* jump to the first iMem after this NE operation */

		rm_cg("LDC", REG_AC1, 0,0, "exp-EQU, Step5"); /* a==b, return 0 as false */

		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_GT:  /* a>b, --> (a-b)>0 */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_GT\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-GT, Step1");

		rm_cg("JGT", REG_AC1, 3-1, REG_PC, "exp-GT, Step2"); /* if a>b is false, continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 0, 0, "exp-GT, Step3"); /* a>b is false, return 0 as false */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-GT, Step4"); /* jump to the first iMem after this GT operation */

		rm_cg("LDC", REG_AC1, 1,0, "exp-GT, Step5"); /* a>b, return 1 as true */

		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_LT: /* a<b, --> (a-b)<0 */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_LT\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-LT, Step1");

		rm_cg("JLT", REG_AC1, 3-1, REG_PC, "exp-LT, Step2"); /* if a<b is false, continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 0, 0, "exp-LT, Step3"); /* a<b is false, return 0 as false */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-LT, Step4"); /* jump to the first iMem after this LT operation */

		rm_cg("LDC", REG_AC1, 1,0, "exp-LT, Step5"); /* a<b, return 1 as true */

		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_GE: /* a>=b, --> (a-b)>=0 */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_GE\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-GE, Step1");

		rm_cg("JGE", REG_AC1, 3-1, REG_PC, "exp-GE, Step2"); /* if a>=b is false, continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 0, 0, "exp-GE, Step3"); /* a>=b is false, return 0 as false */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-GE, Step4"); /* jump to the first iMem after this GE operation */

		rm_cg("LDC", REG_AC1, 1,0, "exp-GE, Step5"); /* a>=b, return 1 as true */
		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_LE:  /* a<=b, --> (a-b)<=0 */
#if ICE9_DEBUG  /*generate comment*/
		memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
		sprintf(lbuf, "*----> start of AST_NT_LE\n");
		emit_comment(lbuf);
#endif

		exp_cg(exp->lexp);
		tm_stack_push(); /* left-exp's value in AC1, push it to temp stack */
		exp_cg(exp->rexp);
		tm_stack_pop();  /* right-exp's value in AC1, pop left-exp's value to AC2 */

		ro_cg("SUB", REG_AC1, REG_AC2, REG_AC1, "exp-LE, Step1");

		rm_cg("JLE", REG_AC1, 3-1, REG_PC, "exp-LE, Step2"); /* if a<=b is false, continue to the following iMem, else jump to the next 3 iMem */
		rm_cg("LDC", REG_AC1, 0, 0, "exp-LE, Step3"); /* a<=b is false, return 0 as false */
		rm_cg("LDA", REG_PC, 2-1, REG_PC, "exp-LE, Step4"); /* jump to the first iMem after this LE operation */

		rm_cg("LDC", REG_AC1, 1,0, "exp-LE, Step5"); /* a<=b, return 1 as true */

		return S_TYPE_BOOL_BASIC;
		break;
	case AST_NT_PAREN:
		return exp_cg(exp->lexp);
		break;

	} /* end of switch */

	return S_TYPE_UNKNOWN;
}

void box_stmt_cg(_ast_box_stmt * bs, int * box_stmt_true_imem, int brk_flag, int * brk_imem, int * rt_imem){
	//int e=0;
	int box_index_false=0;
	if(bs==NULL)
		return;

	if(box_stmt_true_imem==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "box_stmt_cg, box_stmt_true_imem is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	exp_cg(bs->exp);

	//tm_stack_push(); /* push the box-stmt's exp value to temp stack */ /*don't use temp stack to save exp value, because if the stmts has a "break", then stack_pop will not be executed*/

	box_index_false = get_next_icount(); /* box-stmt's exp is false, should jump over this box-stmt; right now not know where to jump, just record the iMem */

	stmt_set_cg(bs->ss, brk_flag, brk_imem, rt_imem);

	/* update the iMem when box-stmt's exp is false */
	rm_cg_i(box_index_false, "JEQ", REG_AC1, get_current_icount()-box_index_false-1, REG_PC, "box-stmt's exp false, jump over entire box-stmt ");

	//tm_stack_pop_ac1(); /* restore the saved box-stmt's exp to REG_AC1, since the above box-stmt might change REG_AC1; for the condition, exp is true */

	exp_cg(bs->exp); /*evaluate exp again; for the condition, exp is true*/
	(*box_stmt_true_imem)++; /* use the first elem as an index */
	box_stmt_true_imem[ *box_stmt_true_imem ] = get_next_icount(); /* box-stmt's exp true, after this box-stmt, should jump out the entire if; right now not know where to jump, just record iMem to the assisting structure. */

}

void box_stmt_set_cg(_ast_box_stmt_set * bss, int * box_stmt_true_imem, int brk_flag, int * brk_imem, int * rt_imem){
	if(bss==NULL)
		return;

	if(bss->lbss!=NULL){
		box_stmt_set_cg(bss->lbss->lbss, box_stmt_true_imem, brk_flag, brk_imem,rt_imem);
		box_stmt_cg(bss->lbss->rbs, box_stmt_true_imem, brk_flag, brk_imem, rt_imem);
	}
	box_stmt_cg(bss->rbs, box_stmt_true_imem, brk_flag, brk_imem, rt_imem);
}

static void count_box_stmt(_ast_box_stmt * bs, int * count){
	(*count)++;
}

static void count_box_stmt_set(_ast_box_stmt_set * bss, int * count){
	if(bss==NULL)
		return ;

	if(bss->lbss!=NULL){
		count_box_stmt_set(bss->lbss->lbss,count);
		count_box_stmt(bss->lbss->rbs, count);
	}
	count_box_stmt(bss->rbs, count);
}

void stmt_set_cg(_ast_stmt_set * ssp, int brk_flag, int * brk_imem, int * rt_imem){
	if(ssp==NULL)
		return;

	if(ssp->lss!=NULL){
		stmt_set_cg(ssp->lss->lss, brk_flag, brk_imem, rt_imem);

		stmt_cg(ssp->lss->rs, brk_flag, brk_imem, rt_imem);
	}

	stmt_cg(ssp->rs, brk_flag, brk_imem, rt_imem);
}

void sub_proc_decl_cg(_ast_sub_proc_decl * spd, int * ar_count){
	_ast_subvar *sv;
	_ast_idlist * idl;
	_scope * current_sp=NULL;
	_symbol * sy1 = NULL, * sy2=NULL;

	int arr_dim_sum=0, i=0;

	if(spd==NULL)
		return;

	current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);

	switch(spd->ntype){
	case AST_NT_TYPE:
		sy1 = (_symbol*)search_all_type_tables(spd->type->typeid->id ); /* get type's parent type info */
		sy2 = n_symbol( spd->type->id );
		sy2->p_type=sy1;
		if(spd->type->array!=NULL)
			sy2->array = spd->type->array;
		ht_insert(spd->type->id, sy2, current_sp->type_table);

		break;
	case AST_NT_VAR:
		sv = spd->var;
		while (sv != NULL) {
			sy1 = (_symbol*)search_all_type_tables( sv->typeid->id ); /* get var's type */
			idl = sv->idl;
			while (idl != NULL) {
				sy2 = n_symbol(idl->id);
				sy2->var_type = sy1; /* record this var's type */
				sy2->cgp = n_cg_temp(); /* update CG info */
				sy2->cgp->local=1; /*mark as local var*/
				sy2->cgp->lvar_proc=temp_proc;
				//sy2->cgp->lvar_fp_offset = (*ar_count); /*update relative dMem offset of local vars; */
				//(*ar_count)++;

				if (sv->array != NULL){ /* if var is an array */
					sy2->array = sv->array;
				}

				sy2->cgp->arr_dim = get_var_array_dimensions(sy2); /*this var has explicit array definition, or its type is redefined type, or both or none*/
				arr_dim_sum = get_array_dim_sum( sy2->cgp->arr_dim );

				if(sy2->cgp->arr_dim==NULL){  /*if not array, just allocate a dMem*/
					sy2->cgp->lvar_fp_offset = (*ar_count); /*update relative dMem offset of local vars; */
					(*ar_count)++;
				}else{  /*if array, allocate dMem for all elem*/
					sy2->cgp->arr_dim_sum = arr_dim_sum;
					sy2->cgp->lvar_fp_offset = (*ar_count);  /*offset is the start elem of the local array*/
					(*ar_count)+=arr_dim_sum;
				}

				ht_insert(idl->id, sy2, current_sp->var_table);
				idl = idl->next;
			}

			sv = sv->next;
		}

		break;
	} /* end of switch */
}
/* generate code for proc's decl-stmt, var/type */
void proc_decl_cg(_ast_proc_decl * pd, int * ar_count){
	if(pd==NULL)
		return;

	if(pd->lpd!=NULL){
		proc_decl_cg(pd->lpd->lpd, ar_count);
		sub_proc_decl_cg(pd->lpd->rspd, ar_count);
	}
	sub_proc_decl_cg(pd->rspd, ar_count);
}



void count_fwd_sub_proc_decl_ar(_ast_sub_proc_decl * spd, int * ar_count){
	_ast_subvar *sv;
	_ast_idlist * idl;
	_symbol * sy1 = NULL, * sy2=NULL;

	if(spd==NULL)
		return;

	if(spd->ntype==AST_NT_VAR){
		sv = spd->var;
		while (sv != NULL) {
			idl = sv->idl;
			while (idl != NULL) {
				(*ar_count)++;
				idl = idl->next;
			}
			sv = sv->next;
		}
	} /*end if*/
}
void count_fwd_proc_decl_ar(_ast_proc_decl * pd, int * ar_count){
	if(pd==NULL)
		return;

	if(pd->lpd!=NULL){
		count_fwd_proc_decl_ar(pd->lpd->lpd, ar_count);
		count_fwd_sub_proc_decl_ar(pd->lpd->rspd, ar_count);
	}
	count_fwd_sub_proc_decl_ar(pd->rspd, ar_count);
}
/*iterate over ice9_engine->ast->stmt_declarations, find the proc with the specified name, get its ActivationRecord size*/
int count_fwd_proc_ar(char * name){
	_ast_stmt_declaration * sdp=NULL;
	_ast_subdeclist * sdl1=NULL;
	_ast_idlist * idl=NULL;
	int ar_count=0;

	sdp = ice9_engine->ast->stmt_declarations;

	while (sdp != NULL) {
		if (sdp->ntype == AST_NT_PROC && strcmp(sdp->proc->id, name)==0) {
			/*count parameters*/
			sdl1 = sdp->proc->sdl;
			while (sdl1 != NULL) {
				idl = sdl1->idl;
				while (idl != NULL) {
					ar_count++;

					idl = idl->next;
				}
				sdl1 = sdl1->next;
			}

			/*count return type*/
			if (sdp->proc->rtype != NULL) {
				ar_count++;
			}

			if (sdp->proc->proc_decl != NULL) { /* CG proc's stmt declaration: type/var */
				count_fwd_proc_decl_ar(sdp->proc->proc_decl, &ar_count);
			}

			ar_count++; /*count return iMem addr*/
			break;
		}

		sdp = sdp->next;
	}

	return ar_count;
}



void count_box_stmt_return(_ast_box_stmt * bs, int * count){
	if(bs==NULL)
		return;

	count_stmt_set_return(bs->ss,count);
}
void count_box_stmt_set_return(_ast_box_stmt_set * bss, int * count){
	if(bss==NULL)
		return ;

	if(bss->lbss!=NULL){
		count_box_stmt_set_return(bss->lbss->lbss,count);
		count_box_stmt_return(bss->lbss->rbs, count);
	}
	count_box_stmt_return(bss->rbs, count);
}
void count_stmt_return(_ast_stmt * sp, int * count){
	if(sp==NULL)
		return;

	/* if(sp->ntype==AST_NT_DO || sp->ntype==AST_NT_FA )
		return; */

	switch(sp->ntype){
	case AST_NT_RETURN:
		(*count)++;
		//sp->brk_flag=rtcount;
		break;
	case AST_NT_IF:
		count_stmt_set_return(sp->iif->iss, count);  /* check possible returns in if's stmt-set */

		if( sp->iif->bss!=NULL) {  /* check possible returns in if's box-stmts */
			count_box_stmt_set_return(sp->iif->bss, count);
		}

		if(sp->iif->ess!=NULL){  /* check possible returns in if's else-stmt */
			count_stmt_set_return(sp->iif->ess, count);
		}
		break;
	case AST_NT_DO:
		if(sp->ddo->ss!=NULL){
			count_stmt_set_return(sp->ddo->ss, count);
		}
		break;
	case AST_NT_FA:
		//printf("+++++++\n");
		if(sp->fa->ss!=NULL){
			count_stmt_set_return(sp->fa->ss, count);
		}
		break;
	}
}
/* count a proc's all return, since proc cannot be defined nestedly, this call will only count returns in a proc */
void count_stmt_set_return(_ast_stmt_set * ssp, int * count){
	if(ssp==NULL)
		return;

	if(ssp->lss!=NULL){
		count_stmt_set_return(ssp->lss->lss, count);
		count_stmt_return(ssp->lss->rs, count);
	}
	count_stmt_return(ssp->rs, count);
}



void stmt_decl_cg(_ast_stmt_declaration * sdp){
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
	//int d1=0, d2=0;
	int ar_count=0;

	_scope * current_sp = NULL;
	_scope * scp1 = NULL;
	void * scope_bak=NULL;
	char lbuf[STR_BUFFER_SIZE];

	int * return_imem=NULL; /* to assist proc's CG, count all return' iMem for a proc */
	int return_count=0, return_flag=0;  /*return_flag: uniquely identify a proc's return*/
	int arr_dim_sum=0;

	current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);

	while (sdp != NULL) {
		switch(sdp->ntype){
		case AST_NT_VAR:
			sv = sdp->var;
			while (sv != NULL) {
				sy1 = (_symbol*)search_all_type_tables( sv->typeid->id ); /* get var's type */
				idl = sv->idl;
				while (idl != NULL) {
					sy2 = n_symbol(idl->id);
					sy2->var_type = sy1; /* record this var's type */
					sy2->cgp = n_cg_temp(); /* update CG info */

					if (sv->array != NULL){ /* if var is an array */
						sy2->array = sv->array;  /*add array info*/
					}

					sy2->cgp->arr_dim = get_var_array_dimensions(sy2); /*this var has explicit array definition, or its type is redefined type, or both or none*/
					arr_dim_sum = get_array_dim_sum( sy2->cgp->arr_dim );
					//printf("--------%d\n", arr_dim_sum);

					if(sy2->cgp->arr_dim==NULL){  /*if not array, just allocate a dMem*/
						if( get_root_type(sy1)!=S_TYPE_STRING_BASIC )  /*if not string; string doesn't use d_index*/
							sy2->cgp->d_index=get_next_dcount();
						else{

						}
					}else{  /*if array, allocate dMem for all elem*/
						sy2->cgp->arr_dim_sum = arr_dim_sum;
						sy2->cgp->d_index=get_current_dcount();  /*d_index is the first dMem of the array*/
						for(i=0;i<arr_dim_sum;i++)
							get_next_dcount();
					}

					ht_insert(idl->id, sy2, current_sp->var_table);
					idl = idl->next;
				}

				sv = sv->next;
			}

			break;
		case AST_NT_TYPE:
			sy1 = (_symbol*)search_all_type_tables(sdp->type->typeid->id ); /* get type's parent type info */
			sy2 = n_symbol( sdp->type->id );
			sy2->p_type=sy1;
			if(sdp->type->array!=NULL)
				sy2->array = sdp->type->array;
			ht_insert(sdp->type->id, sy2, current_sp->type_table);

			break;
		case AST_NT_FORWARD:
			ar_count=0;
			sy1 = n_symbol( sdp->forward->fname );
			sy1->fwd_unmatched=1;
			sy1->is_fwd=1;
			sy1->fwd = sdp->forward;
			if( sdp->forward->rtype!=NULL )
				sy1->proc_rtype = search_all_type_tables(sdp->forward->rtype->id );
			sy1->cgp = n_cg_temp(); /* update CG info */
			sy1->cgp->i_index = get_next_icount();  /*use this iMem to generate jump, to jump to the proc's actual code*/

			/*go ahead to search for the corresponding proc, to get AR size*/
			sy1->cgp->ar_size = count_fwd_proc_ar(sdp->forward->fname);
			//printf("--------forward AR:%d\n", sy1->cgp->ar_size);

			ht_insert(sdp->forward->fname, sy1, current_sp->proc_table);

			break;
		case AST_NT_PROC:
			ar_count=1;  /*return address*/
			sy1 = (_symbol*)ht_search(sdp->proc->id, current_sp->proc_table);

			if(sy1!=NULL && sy1->fwd_unmatched==1){ /*if there is a pre-defined forward*/
				rm_cg_i(sy1->cgp->i_index, "LDC", REG_PC, get_current_icount(), 0, "this is forward, jump to proc");

				ht_delete(sy1->name, sy1, current_sp->proc_table);
			}

			/* then create new scope for this proc, and insert any parameters and return-value into var-table; */
			scp1 = n_scope(SCOPE_PROC, HT_ARRAY_SIZE_S, 1, 1, 0);
			stk_push(scp1, ice9_engine->scope_stack);  /* push */
			current_sp = scp1;
			temp_proc = sdp->proc; /*update temp global reference*/

			/* insert parameters */
			sdl1 = sdp->proc->sdl;
			while(sdl1!=NULL){
				idl = sdl1->idl;

				sy1 = search_all_type_tables(sdl1->typeid->id);

				while(idl!=NULL){
					sy2 = n_symbol(idl->id);
					sy2->var_type = sy1; /* record this var's type */
					sy2->cgp = n_cg_temp(); /* update CG info */
					sy2->cgp->local=1; /*mark as local var*/
					sy2->cgp->lvar_proc=sdp->proc; /*update proc reference*/
					sy2->cgp->lvar_fp_offset = ar_count++; /*update local var offset, relative to FP*/

					/*if parameter redefined array, maybe something more to be done ??*/

					ht_insert(idl->id, sy2, current_sp->var_table);
					idl = idl->next;
				}

				sdl1=sdl1->next;
			}
			/* insert return type */
			if(sdp->proc->rtype!=NULL){
				sy1 = search_all_type_tables( sdp->proc->rtype->id );

				sy2 = n_symbol( sdp->proc->id ); /* implicit return var's name is the same as the proc's name; */
				sy2->var_type=sy1;
				sy2->cgp = n_cg_temp(); /* update CG info */
				sy2->cgp->local=1; /*mark as local var*/
				sy2->cgp->lvar_proc=sdp->proc; /*update proc reference*/
				sy2->cgp->lvar_fp_offset = ar_count++;  /*update local var offset, relative to FP*/
				//printf("-----------%d\n", sy2->cgp->lvar_fp_offset);
				ht_insert( sdp->proc->id, sy2, current_sp->var_table);
			}

			/* insert proc's info to previous proc-table; */
			sy3 = n_symbol( sdp->proc->id ); /*new symbol for this proc*/
			//printf("%s\n", sdp->proc->id);
			if(sdp->proc->rtype!=NULL)
				sy3->proc_rtype=sy1;
			sy3->proc = sdp->proc;
			sy3->is_proc=1;
			sy3->cgp = n_cg_temp(); /* update CG info */
			sy3->cgp->i_index = get_current_icount(); /*proc's entrance iMem*/

			scope_bak = stk_pop(ice9_engine->scope_stack); /* backup previously pushed proc-scope */
			ht_insert( sdp->proc->id, sy3, ((_scope*)stk_peek(ice9_engine->scope_stack))->proc_table ); /* insert to proc table */
			stk_push(scope_bak, ice9_engine->scope_stack); /* push back proc scope again */

#if ICE9_DEBUG
			memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
			sprintf(lbuf, "*----> start of proc %s\n", sdp->proc->id);
			emit_comment(lbuf);
#endif

			if(sdp->proc->proc_decl!=NULL){ /* CG proc's stmt declaration: type/var */
				proc_decl_cg(sdp->proc->proc_decl, &ar_count);
			}

			sy3->cgp->ar_size=ar_count;  /*set final AR, since ar_count starts from 1, return-iMem-addr is already included*/
			//printf("--------proc AR size:%d\n", sy3->cgp->ar_size);

			if(sdp->proc->ss!=NULL){ /* CG proc's statements body */
				/* count how many returns in this proc, return_imem is the CG assisting structure.
				 * "return" in proc will cause the control to jump to the end of the proc, but this compiler
				 * cannot decide where is the end of the proc until it sees the actual end of the
				 * proc. So, in this assisting return_imem, the first elem is a temp index,
				 * every following elem stores a iMem where the above discussed jump should be generated. */
				//return_flag=get_next_rtcount();
				count_stmt_set_return(sdp->proc->ss, &return_count);  /*don't need the return_flag, since proc cannot be defined nestedly*/
				//printf("----------%d\n\n", return_count);
				if(return_count>0){
					//printf("----------%d\n\n\n", return_count);
					return_imem=(int*)malloc(sizeof(int)*(return_count+1)+1);
					if(return_imem==NULL){
#if ICE9_DEBUG
						ice9_log_level(ice9_engine->lgr, "stmt_decl_cg, out of memory when creating return_imem,", ICE9_LOG_LEVEL_ERROR);
#endif
						exit(1);
					}
					memset(return_imem, 0, sizeof(int)*(return_count+1)+1);
					(*return_imem)=0;
				}

				stmt_set_cg(sdp->proc->ss, BREAK_IGNORE, NULL, return_imem);

			}

			/*now know where this proc ends, return stmt should jump to this point; if proc has return value, still need to load it to REG_AC1, then jump back to the caller*/
			if(return_count>0){
				for(i=1;i<=return_count;i++){
					rm_cg_i(return_imem[i], "LDA", REG_PC, get_current_icount()-return_imem[i]-1, REG_PC, "proc return, jump out of this proc");
				}
				free(return_imem);
			}

			/*if has return value, put it to REG_AC1*/
			if(sdp->proc->rtype!=NULL){
				//printf("-------------%d\n", ar_count - sy2->cgp->lvar_fp_offset);
				rm_cg("LD", REG_AC1, ar_count - sy2->cgp->lvar_fp_offset , REG_FP, "in proc, load proc return-value to REG_AC1");
			}

			/* jump to return addr, which is the "topest" iMem cell in the AR */
			rm_cg("LD", REG_PC, 0, REG_FP, "in proc call, jump back to caller");

#if ICE9_DEBUG
			memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
			sprintf(lbuf, "*----> end of proc %s\n", sdp->proc->id);
			emit_comment(lbuf);
#endif

			/* pop the proc's scope */
			stk_pop(ice9_engine->scope_stack);
			current_sp = (_scope*)stk_peek(ice9_engine->scope_stack);  /* update current_sp */
			temp_proc=NULL;
			break;
		} /* end of switch */
		sdp = sdp->next;
	} /* end of outter while */
}



void count_box_stmt_break(_ast_box_stmt * bs, int * count, int brkcount){
	if(bs==NULL)
		return;

	count_stmt_set_break(bs->ss,count, brkcount);
}
void count_box_stmt_set_break(_ast_box_stmt_set * bss, int * count, int brkcount){
	if(bss==NULL)
		return ;

	if(bss->lbss!=NULL){
		count_box_stmt_set_break(bss->lbss->lbss,count, brkcount);
		count_box_stmt_break(bss->lbss->rbs, count, brkcount);
	}
	count_box_stmt_break(bss->rbs, count, brkcount);
}
void count_stmt_break(_ast_stmt * sp, int * count, int brkcount){
	if(sp==NULL)
		return;

	if(sp->ntype==AST_NT_DO || sp->ntype==AST_NT_FA )
		return;

	/* only care direct break, or possible breaks in if */
	if(sp->ntype==AST_NT_BREAK){
		(*count)++;
		sp->brk_flag=brkcount;
	}

	if(sp->ntype==AST_NT_IF){
		count_stmt_set_break(sp->iif->iss, count, brkcount);  /* check possible breaks in if's stmt-set */

		if( sp->iif->bss!=NULL) {  /* check possible breaks in if's box-stmts */
			count_box_stmt_set_break(sp->iif->bss, count, brkcount);
		}

		if(sp->iif->ess!=NULL){  /* check possible breaks in if's else-stmt */
			count_stmt_set_break(sp->iif->ess, count, brkcount);
		}
	}
}
/* count a do/fa's all breaks, ignore those breaks in sub-do/fa */
void count_stmt_set_break(_ast_stmt_set * ssp, int * count, int brkcount){
	if(ssp==NULL)
		return;

	if(ssp->lss!=NULL){
		count_stmt_set_break(ssp->lss->lss, count, brkcount);
		count_stmt_break(ssp->lss->rs, count, brkcount);
	}
	count_stmt_break(ssp->rs, count, brkcount);
}



/*
 * the last two parameters are optional, used to assist CG of do/fa.
 */
void stmt_cg(_ast_stmt * sp, int brk_flag, int * brk_imem, int * rt_imem){
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;
	_ast_forward * fw;
	_ast_subdeclist * sdl;
	_ast_exp * exp, * te;

	_scope * current_scope = NULL, * fa_scope=NULL;
	_symbol * sy1 = NULL, *sy2=NULL, *sy3=NULL;
	int * ass_arr_d=NULL;  /* assignment's left side's array dimension */
	int i=0, box_stmt_count=0, break_count=0, break_flag=0;  /*break_flag: uniquely identify a fa/do's break*/
	int e1=0, e2=0;
	int ass_arr_d_num=0;
	int * box_stmt_true_imem=NULL; /* to assist CG of if's box-stmt */
	int * break_imem=NULL; /* to assist do's CG, count all breaks' iMem */
	int if_index_false=0; /* if if's exp false, jump to the iMem(else-stmt or out of if) */
	int if_index_true=0; /* if if's exp true, after finishing if's stmt_set and box_stmt_set, jump to the iMem*/
	int do_index_entrance=0;
	int do_index_false=0;
	int fa_index_entrance=0;
	int fa_index_false=0;

	char lbuf[STR_BUFFER_SIZE];
	int * arr_dims=NULL;
	int arr_total_d=0;
	int slen=0, t_dmem=0;
	int exp_value_dmem=0; /*dedicated global dMem to hold if's-exp/fa's-second-exp, do not re-evaluate it again*/

	current_scope = (_scope*)stk_peek(ice9_engine->scope_stack);

	while (sp != NULL) {
		switch (sp->ntype) {
		case AST_NT_EXP:
			exp_cg(sp->exp);
			break;
		case AST_NT_ASSIGN:
			//printf("-------------%d\n", get_current_dcount() );
			sy1 = search_all_var_tables( sp->assign->id );  /* get identifier's info */
			//printf("----------%s\n\n", sp->assign->id );
			sy3 = search_all_type_tables(sy1->var_type->name); /*sy3: var's type symbol*/

			if(sy3==NULL){
#if ICE9_DEBUG
				ice9_log_level(ice9_engine->lgr, "stmt_cg, ASSIGN, var's type symbol is NULL, UNKNOWN", ICE9_LOG_LEVEL_ERROR);
#endif
				return;
			}

			if(sy1->cgp==NULL){ /*every identifier should have an associated symbol*/
#if ICE9_DEBUG
				ice9_log_level(ice9_engine->lgr, "stmt_cg, ASSIGN, cgp is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
				return;
			}

#if ICE9_DEBUG
			memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
			sprintf(lbuf, "*----> ASSIGNMENT, identifier %s\n",sp->assign->id );
			emit_comment(lbuf);
#endif

			if(sy1->cgp->local==1){  /*if local var, get related proc's info*/
				//printf("----------%s\n\n", sy1->cgp->lvar_proc->id);
				sy2 = search_all_proc_tables( sy1->cgp->lvar_proc->id );  /*get the proc's symbol<<<<<<???*/

				if (sy2 == NULL) {
#if ICE9_DEBUG
					ice9_log_level(ice9_engine->lgr, "stmt_cg, ASSIGN, cannot find local var's proc symbol", ICE9_LOG_LEVEL_ERROR);
#endif
					return;
				}
			}

			exp_cg( sp->assign->rexp ); /* right exp cg */

			/*the outter if-else, check if array; the inner if or else, check if local var*/
			if(sp->assign->lexp==NULL){  /* if not array */
				//printf("----------%d\n\n", get_root_type(sy3));
				if( get_root_type(sy3)==S_TYPE_STRING_BASIC ){  /*if string, simple&naive: not care global or local....*/

					if(sp->assign->rexp->svalue==NULL){  /*just make sure...*/
#if ICE9_DEBUG
						ice9_log_level(ice9_engine->lgr,"stmt_cg, Assignment, string NULL",ICE9_LOG_LEVEL_ERROR);
#endif
						break;
					}

					slen = strlen(sp->assign->rexp->svalue);
					sy1->cgp->str_len = slen;
					sy1->cgp->str_start_dmem = get_current_dcount();
					for(i=0; i<slen; i++){
						rm_cg("LDC", REG_AC1, *(sp->assign->rexp->svalue+i), 0, "Assignment, load str char to AC1");
						rm_cg("ST", REG_AC1, get_current_dcount(), REG_ZERO, "Assignment, store AC1 to str loc");
						get_next_dcount();
					}
					//printf("----------\n\n");
					//printf("-------------%d\n", get_current_dcount() );
					break;
				}

				if(sy1->cgp->local==0){  /*if not proc's local var*/
					rm_cg("ST", REG_AC1, sy1->cgp->d_index, REG_ZERO, "Assignment, store AC1 to GLOBAL left identifier");
				}else{
					memset(lbuf, 0, sizeof(char)*STR_BUFFER_SIZE);
					sprintf(lbuf, "Assignment, store AC1 to LOCAL left identifier, %s", sp->assign->id);
					rm_cg("ST", REG_AC1, (sy2->cgp->ar_size - sy1->cgp->lvar_fp_offset), REG_FP, lbuf); /*FP+(AR-offset)*/
				}
			}else{ /*array*/
				tm_stack_push(); /*save sp->assign->rexp's value*/
				arr_dims = get_var_array_dimensions(sy1); /*get all dimension size*/
				arr_total_d = get_array_d_num(arr_dims);

				i=0;
				te=sp->assign->lexp;

				rm_cg("LDC", REG_AC1, 0, 0, "ASSIGNMENT, init array elem offset");
				tm_stack_push();

				while(te!=NULL){

					exp_cg(te);  /*evaluate specified dimension index in the LVALUE, in AC1*/
					rm_cg("JLT", REG_AC1, 6, REG_PC, "ASSIGNMENT, if specified index is negative, jump to error handler" );
					tm_stack_push();/*temporarily save this specified dimension index*/
					rm_cg("LDC", REG_AC2, arr_dims[i], 0, "ASSIGNMENT, get the defined size for an array's dim 1"); /*defined size of the i-th dimension is now in AC2*/

					ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "ASSIGNMENT, specified dim - defined dim");
					rm_cg("JLT", REG_AC1, 3, REG_PC, "ASSIGNMENT, valid array index, skip error routine");
					tm_stack_peek();  /*get specified dimension index*/
					rm_cg("ST", REG_AC1, arr_err_msg_start_dMem+arr_err_msg_index_offset, REG_ZERO, "ASSIGNMENT, load invalid index to error msg"); /*load invalid array index to error msg*/
					rm_cg("LDC", REG_PC, arr_err_proc_start_iMem, REG_ZERO, "ASSIGNMENT, jump to error routine");/*call pre-defined proc to process array-out-of-bound error, this proc will not return, just print error msg and exit*/


					tm_stack_pop(); /*pop previously evaluated specified dimension size to AC2*/
					if( (i+1)<arr_total_d ){  /*if current index is not the last dimension.*/
						rm_cg("LDC", REG_AC1, arr_dims[i], 0, "ASSIGNMENT, get the defined size for an array's dim 2"); /*defined size of the i-th dimension is now in AC1*/
						ro_cg("MUL", REG_AC1, REG_AC1, REG_AC2, "ASSIGNMENT, mul to get part of array offset"); /*new part of offset in AC1*/
						tm_stack_pop(); /*pop previously saved offset to AC2*/
						ro_cg("ADD", REG_AC1, REG_AC1, REG_AC2, "ASSIGNMENT, update offset");

					}else{
						tm_stack_pop_ac1(); /*pop previously saved offset to AC1*/
						ro_cg("ADD", REG_AC1, REG_AC1, REG_AC2, "ASSIGNMENT, update offset, last index");
					}
					tm_stack_push(); /*store updated offset*/

					i++;
					te=te->next;
				}  /*end of while, array final offset in temp stack*/
				//exp_cg( sp->assign->rexp ); /* evaluate right exp again, might optimize to store it to stack?? */
				tm_stack_pop(); /*pop array final offset to AC2, should be after the above exp_cg(), or AC2 might be overwritten*/
				tm_stack_pop_ac1(); /*pop saved sp->assign->rexp to AC1*/
				if(sy1->cgp->local==0){  /*if not proc's local var*/
					rm_cg("ST", REG_AC1, sy1->cgp->d_index, REG_AC2, "ASSIGNMENT, GLOBAL finally load AC1 to dMem");
				}else{ /*local var*/
					ro_cg("SUB", REG_AC2, REG_FP, REG_AC2, "ASSIGNMENT, LOCAL, load final array_elem's dMem to AC1 1");
					rm_cg("ST", REG_AC1, (sy2->cgp->ar_size - sy1->cgp->lvar_fp_offset), REG_AC2, "ASSIGNMENT, LOCAL, load final array_elem's dMem to AC1 2");
				}
			}/*end if array*/

			break;
		case AST_NT_WRITE:
			e1 = exp_cg(sp->write->exp);
			//printf("---------%d\n\n", e1);
			if( e1==S_TYPE_INT_BASIC ){
				ro_cg("OUT", REG_AC1, 0, 0, "write int");  /* write exp, whose value should be in REG_AC1 */
				//printf("---------\n\n");
			}else if( e1==S_TYPE_STRING_BASIC ){
				/* string, base in AC1, string len in AC2, return addr in T1 */
				//printf("---------%s\n", sp->write->exp->svalue);
				if(sp->write->exp->svalue!=NULL){ /*if write target is string literal: write "abc" */

					slen = strlen( sp->write->exp->svalue );
					t_dmem = get_current_dcount();
					for(i=0; i<slen; i++){
						rm_cg("LDC", REG_AC1, *(sp->write->exp->svalue+i), 0, "Write, load str char to AC1");
						rm_cg("ST", REG_AC1, get_current_dcount(), REG_ZERO, "Write,c store AC1 to str loc");
						get_next_dcount();
					}

					rm_cg("LDC", REG_AC1, t_dmem, 0, "Write, load str base1");
					rm_cg("LDC", REG_AC2, slen, 0, "Write, load str len1");

				}else{ /*if write target is a string var: write s, s is "abc"*/
					sy1 = search_all_var_tables( sp->write->exp->id );
					//printf("-------%s\n\n", sp->write->exp->id);
					if(sy1==NULL){
#if ICE9_DEBUG
						ice9_log_level(ice9_engine->lgr,"stmt_cg, write string, UNKNOWN condition",ICE9_LOG_LEVEL_ERROR);
#endif
						return;
					}
					//printf("---------%d, %d\n\n", sy1->cgp->str_start_dmem, sy1->cgp->str_len);
					rm_cg("LDC", REG_AC1, sy1->cgp->str_start_dmem, 0, "Write, load str base2");
					rm_cg("LDC", REG_AC2, sy1->cgp->str_len, 0, "Write, load str len2");
				}
				rm_cg("LDC", REG_T1, get_current_icount()+2, 0, "Write, load ret addr");
				rm_cg("LDC", REG_PC, str_wproc_start_iMem, 0, "jump to str writer");
			}

			if(sp->write->newline == 1){
				ro_cg("OUTNL", 0, 0, 0, "write new line");
			}
			//printf("--------\n\n");
			break;
		case AST_NT_BREAK:
			if(sp->brk_flag==brk_flag){
				//printf("-------sp->brk_flag:%d\n", sp->brk_flag);
				if(brk_imem==NULL){
#if ICE9_DEBUG
					ice9_log_level(ice9_engine->lgr,"stmt_cg, UNKNOWN condition",ICE9_LOG_LEVEL_ERROR);
#endif
				}
				(*brk_imem)++;
				brk_imem[ *brk_imem ]=get_next_icount();
				//printf("-------created break's iMem:%d\n", brk_imem[ *brk_imem ]);
				sp->brk_flag=BREAK_GENERATED;
			}
			break;
		case AST_NT_EXIT:
			rm_cg("LD", REG_PC, 0, REG_ZERO, "global return, jump to the last iMem");

			break;
		case AST_NT_RETURN:
			if(temp_proc!=NULL){  /*if this return is in a proc*/
				if(rt_imem==NULL){
#if ICE9_DEBUG
					ice9_log_level(ice9_engine->lgr,"stmt_cg, rt_imem is NULL, UNKNOWN condition",ICE9_LOG_LEVEL_ERROR);
#endif
					return;
				}
				(*rt_imem)++;
				rt_imem[ *rt_imem ]=get_next_icount();
			}else{  /*if this return is global, means exit*/
				rm_cg("LD", REG_PC, 0, REG_ZERO, "global return, jump to the last iMem");
			}

			break;
		case AST_NT_DO:
			exp_cg(sp->ddo->exp);

			if(sp->ddo->ss!=NULL){
				break_flag=get_next_brkcount();  /* brkcount flag; used to identify breaks specifically for this do-stmt */
				/* count how many breaks in this do-stmt, break_imem is the CG assisting structure.
				 * "break" will cause the control to jump to the end of the do-stmt, but this compiler
				 * cannot decide where is the end of the do-stmt until it sees the actual end of the
				 * do-stmt. So, in this assisting break_imem, the first elem is a temp index,
				 * every following elem stores a iMem where the above discussed jump should be generated. */
				count_stmt_set_break(sp->ddo->ss, &break_count, break_flag); /* all direct breaks or breaks in if-stmt will be marked by this flag */
				//printf("---------break_count:%d\n", break_count);
				if(break_count>0){
					break_imem = (int*)malloc(sizeof(int)*(break_count+1)+1);
					if(break_imem==NULL){
#if ICE9_DEBUG
						ice9_log_level(ice9_engine->lgr, "stmt_cg, out of memory, do", ICE9_LOG_LEVEL_ERROR);
#endif
						exit(1);
					}
					memset(break_imem, 0, sizeof(int)*(break_count+1)+1);
					(*break_imem)=0;
				}  /* end if(break_count>0) */

				do_index_entrance = get_current_icount();  /* the first instruction's iMem after do's exp */
				/* if do's exp is false, jump to end of do-stmt; but right now, don't where to jump, just record the iMem */
				do_index_false = get_next_icount();

				stmt_set_cg(sp->ddo->ss, break_flag, break_imem, rt_imem);  /* do's stmt_set CG */

				exp_cg(sp->ddo->exp); /* CG and evaluate do's exp again, to determine if go back*/

				/* do's exp sill true, go back */
				rm_cg("JGT", REG_AC1, do_index_entrance-get_current_icount(), REG_PC, "if do's exp sill true, go back");

				/* now know where do's stmt-set finishes, update all valid break-instruction */
				if(break_count>0){
					for(i=1;i<=break_count;i++){
						//printf("-----%dth break's iMem:%d\n", i, break_imem[i]);
						rm_cg_i(break_imem[i], "LDA", REG_PC, get_current_icount()-break_imem[i]-1, REG_PC, "break, jump out of this do-stmt");
					}
					free(break_imem);
				}

				/* now know where do ends, if do's exp false, jump over */
				rm_cg_i(do_index_false, "JEQ", REG_AC1, get_current_icount()-do_index_false-1, REG_PC, "if do's exp false, jump over entire stmt-set");
			}

			break;
		case AST_NT_FA:
			/* create fa scope */
			fa_scope = n_scope(SCOPE_FA, HT_ARRAY_SIZE_T, 0, 1, 0);
			stk_push(fa_scope, ice9_engine->scope_stack);
			current_scope = fa_scope;

			/* insert fa loop variable into var-table */
			sy1 = n_symbol( sp->fa->id );
			sy1->var_type = search_all_type_tables("int");
			sy1->is_fa_loop_var=1;
			ht_insert(sp->fa->id, sy1, current_scope->var_table);
			sy1->cgp = n_cg_temp(); /* update CG info */
			sy1->cgp->d_index=get_next_dcount();

			/* CG of fexp and texp */
			exp_cg(sp->fa->fexp);
			tm_stack_push();
			rm_cg("ST", REG_AC1, sy1->cgp->d_index, REG_ZERO, "Fa, assign fexp's value to loop var"); /*update loop var's value*/
			exp_cg(sp->fa->texp);
			tm_stack_pop();   /* to-exp's value in AC1, from-exp's value in AC2 */
			exp_value_dmem = get_next_dcount(); /*allocate dedicated dMem to save fa's texp value*/
			rm_cg("ST", REG_AC1, exp_value_dmem, REG_ZERO, "backup fa's texp's value");
			ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "evaluate fa's exps first time");

			if(sp->fa->ss!=NULL){
				break_flag=get_next_brkcount();  /* brkcount flag; used to identify breaks specifically for this fa-stmt */
				/* count how many breaks in this fa-stmt, break_imem is the CG assisting structure.
				 * "break" will cause the control to jump to the end of the fa-stmt, but this compiler
				 * cannot decide where is the end of the fa-stmt until it sees the actual end of the
				 * fa-stmt. So, in this assisting break_imem, the first elem is a temp index,
				 * every following elem stores a iMem where the above discussed jump should be generated. */
				count_stmt_set_break(sp->fa->ss, &break_count, break_flag); /* all direct breaks or breaks in if-stmt will be marked by this flag */
				//printf("-------%d\n", break_count);
				if(break_count>0){
					break_imem = (int*)malloc(sizeof(int)*(break_count+1)+1);
					if(break_imem==NULL){
#if ICE9_DEBUG
						ice9_log_level(ice9_engine->lgr, "stmt_cg, out of memory, fa", ICE9_LOG_LEVEL_ERROR);
#endif
						exit(1);
					}
					memset(break_imem, 0, sizeof(int)*(break_count+1)+1);
					(*break_imem)=0;
				}  /* end of if(break_count>0) */


				fa_index_entrance = get_current_icount();  /* the first instruction to be executed after fa's exps */

				/* if fa's exp is false, jump to end of fa-stmt; but right now, don't know where to jump, just record the iMem */
				fa_index_false = get_next_icount();

				stmt_set_cg(sp->fa->ss, break_flag, break_imem, rt_imem);  /* fa's stmt_set CG */

				/* update loop var, increase by 1 */
				rm_cg("LD", REG_AC1, sy1->cgp->d_index, REG_ZERO, "update loop var 1"); /*load loop var to REG_AC1*/

				rm_cg("LDA", REG_AC1, 1, REG_AC1, "update loop var2"); /*increase REG_AC1 by 1*/
				rm_cg("ST", REG_AC1, sy1->cgp->d_index, REG_ZERO, "update loop var3"); /*update loop var's dMem*/

				//emit_comment("*-->test1\n");
				//exp_cg(sp->fa->texp); /*evaluate texp again, result in REG_AC1*/
				rm_cg("LD", REG_AC1, exp_value_dmem, REG_ZERO, "load saved fa's texp's value to AC1");
				//emit_comment("*-->test2\n");

				rm_cg("LD", REG_AC2, sy1->cgp->d_index, REG_ZERO, "load loop var from dMem to AC2"); /*load loop var directly from dMem to AC2; cannot save it to another REG, might be overwritten in the above exp_cg(sp->fa->texp)*/
				ro_cg("SUB", REG_AC1, REG_AC1, REG_AC2, "evaluate fa's exps again");
				rm_cg("JGE", REG_AC1, fa_index_entrance-get_current_icount(), REG_PC, "fa, go back if loop condition true");

				/* now know where fa's stmt-set finishes, update all valid break-instruction */
				if(break_count>0){
					for(i=1;i<=break_count;i++){
						rm_cg_i(break_imem[i], "LDA", REG_PC, get_current_icount()-break_imem[i]-1, REG_PC, "break, jump out of this fa-stmt");
					}
					free(break_imem);
				}

				/* now know where fa ends, if fa's exp false, jump over */
				rm_cg_i(fa_index_false, "JLT", REG_AC1, get_current_icount()-fa_index_false-1, REG_PC, "if fa's exp false, jump over entire stmt-set");
			}

			stk_pop( ice9_engine->scope_stack ); /* pop current fa scope */
			current_scope = (_scope*)stk_peek( ice9_engine->scope_stack ); /* restore previous scope */
			break;
		case AST_NT_IF:
			exp_cg(sp->iif->exp);
			//tm_stack_push(); /* push the if's exp value to temp stack */ /*don't use stack to temporarily store exp, because possible "break" in the stmts, then pop will not be executed */
			exp_value_dmem = get_next_dcount(); /*allocate dedicated dMem to save if's exp value*/
			rm_cg("ST", REG_AC1, exp_value_dmem, REG_ZERO, "backup if's exp's value");

			/* if statement is false, jump to else-stmt or first stmt after if(there is no else branch);
			 * since right now, not know the place to jump, so just record the iMem of this jump */
			if_index_false=get_next_icount();

			stmt_set_cg( sp->iif->iss , brk_flag, brk_imem, rt_imem);  /* generate if's stmts */

			/* init assisting structure for CG of box-stmt: box_stmt_true_imem;
			 * in each box-stmt, if the exp is true, after finishing this box-stmt, the compiler should jump out of the entire if,
			 * it should know to which iMem it should jump; but this iMem cannot be determined until the compiler sees the end of
			 * the if-stmt.  So, in this assisting structure, the first elem stores an assisting index, then each following
			 * elem stores a iMem where the above discussed jump instruction should be generated. */
			if(sp->iif->bss!=NULL){
				count_box_stmt_set(sp->iif->bss, &box_stmt_count);
				box_stmt_true_imem = (int*)malloc(sizeof(int)*(box_stmt_count+1)+1);
				if( box_stmt_true_imem==NULL ){
#if ICE9_DEBUG
					ice9_log_level(ice9_engine->lgr, "stmt_cg, out of memeory, if", ICE9_LOG_LEVEL_ERROR);
#endif
					exit(1);
				}
				memset(box_stmt_true_imem, 0, sizeof(int)*(box_stmt_count+1)+1);
				*box_stmt_true_imem=0; /* set first elem to 0, in each box-stmt this elem is first increased, and used as index */
			}

			if( sp->iif->bss==NULL && sp->iif->ess==NULL ){  /* both box-stmt and else-stmt are NULL */
				//printf("------\n");
				/* if if's exp is false, and no box/else-stmt, jump out of if */
				rm_cg_i(if_index_false, "JEQ", REG_AC1, get_current_icount()-if_index_false-1, REG_PC, "if's exp is false, and no box/else-stmt, jump out of if");

				//tm_stack_pop(); /* just pop out stored if's exp value, not actually use this value in this case */
			}else{
				/* if's exp is false, jump to this box-stmt/else; the index of iMem should follow immediately after if's exp. */
				rm_cg_i(if_index_false, "JEQ", REG_AC1, get_current_icount()-if_index_false-1, REG_PC, "if's exp is false, jump to box-stmt/else");

				//exp_cg(sp->iif->exp); /* evaluate if's exp again, used in the next instruction */
				//tm_stack_pop_ac1(); /* restore the saved if's exp value; used in the next instruction */
				rm_cg("LD", REG_AC1, exp_value_dmem, REG_ZERO, "load saved if's exp's value to AC1");

				/* if's exp is true, and after if's stmt_set, should jump over possible box-stmt/else;
				* but right now, not know the place to jump, so just record the iMem of this jump;
				* this instruction should be the first one after if's stmt_set */
				if_index_true=get_next_icount();
				//printf("----------%d\n", if_index_true);

				if( sp->iif->bss!=NULL && sp->iif->ess==NULL ){ /* box-stmt not NULL, else-stmt NULL */

					box_stmt_set_cg(sp->iif->bss, box_stmt_true_imem, brk_flag, brk_imem, rt_imem); /* generate box-stmt code */

					/* now know the iMem of where box-stmts finishes, if's exp true, jump to this location, ignore box-stmt part */
					rm_cg_i(if_index_true, "JGT", REG_AC1, get_current_icount()-if_index_true-1, REG_PC, "if's exp is true, jump over box-stmt ");

					/* now know the iMem of where box-stmts finishes, update the iMems in box-stmts where the compiler should jump if box-stmt's exp is true */
					for(i=1;i<=box_stmt_count;i++){
						rm_cg_i(box_stmt_true_imem[i], "JGT", REG_AC1, get_current_icount()-box_stmt_true_imem[i]-1, REG_PC, "box-stmt exp true, jump out of entire if");
					}

				}else if( sp->iif->bss==NULL && sp->iif->ess!=NULL ){ /* box-stmt NULL, else-stmt not NULL*/
					//printf("------\n");
					stmt_set_cg( sp->iif->ess , brk_flag, brk_imem, rt_imem); /* generate else-stmt code */

					/* now know the iMem of where else finishes, if's exp true, jump to this location, ignore else part */
					rm_cg_i(if_index_true, "JGT", REG_AC1, get_current_icount()-if_index_true-1, REG_PC, "if's exp is true, jump over else-stmt ");
				}else{ /* both box-stmt and else-stmt are not NULL */
					box_stmt_set_cg(sp->iif->bss, box_stmt_true_imem, brk_flag, brk_imem, rt_imem); /* generate box-stmt code */

					stmt_set_cg( sp->iif->ess , brk_flag, brk_imem, rt_imem); /* generate else-stmt code */

					/* now know the iMem of where box-stmts/else finishes, update the iMems in box-stmts where the compiler should jump if box-stmt's exp is true */
					for(i=1;i<=box_stmt_count;i++){
						rm_cg_i(box_stmt_true_imem[i], "JGT", REG_AC1, get_current_icount()-box_stmt_true_imem[i]-1, REG_PC, "box-stmt exp true, jump out of entire if");
					}

					/* now know the iMem of where box-stmt/else finishes, if's exp true, jump to this location, ignore box-stmt/else part */
					rm_cg_i(if_index_true, "JGT", REG_AC1, get_current_icount()-if_index_true-1, REG_PC, "if's exp is true, jump over else-stmt ");
				}
			}

			if(sp->iif->bss!=NULL){
				free(box_stmt_true_imem);
			}

			break;
		} /* end of switch */
		sp = sp->next;
	} /* end of outter while */
}

/* will re-init scope stacks and symbol tables, so semantic check and code generation could be performed seperately */
void code_generation(char * fn){

	if(fn==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "code_generation, fn is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		return;
	}

	init_cg(fn); /* setup scope stack and symbol tables */

	stmt_decl_cg(ice9_engine->ast->stmt_declarations);

	rm_cg_i(exec_entrance, "LDC", REG_PC, get_current_icount(), 0, "jump to the starting stmt");

	stmt_cg(ice9_engine->ast->stmts, BREAK_IGNORE, NULL, NULL);

#if ICE9_DEBUG
	ro_cg("OUT", REG_SP, 0, 0, "final output REG_SP, testing");
	ro_cg("OUT", REG_FP, 0, 0, "final output REG_FP, testing");
	ro_cg("OUTNL", 0, 0, 0, "final output REG, testing");ro_cg("OUTNL", 0, 0, 0, "final output REG, testing");
#endif

	clean_cg(); /* clean up */
}
