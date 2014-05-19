/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * ice9.
 */

#include <stdlib.h>
#include <string.h>
#include "ice9.h"
extern char *yytext;

_ice9_core * ice9_engine=NULL;


void check_ice9_engine(){
	if(ice9_engine==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "check_ice9_engine, call ice9_init() first", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}
}


void ice9_init(){
	ice9_engine = (_ice9_core*)malloc(sizeof(_ice9_core));

	if(ice9_engine==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "ice9_init, out of memory1", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ice9_engine->ast = (_ast_root*)malloc(sizeof(_ast_root));
	if(ice9_engine->ast==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "ice9_init, out of memor2", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ice9_engine->ast->stmt_declarations=NULL;
	ice9_engine->ast->stmt_declarations_tail=NULL;
	ice9_engine->ast->stmts=NULL;
	ice9_engine->ast->stmts_tail=NULL;

	/* init temp list */
	ice9_engine->ast->t.array = ice9_engine->ast->t.array_tail = NULL;
	ice9_engine->ast->t.idlist = ice9_engine->ast->t.idlist_tail = NULL;
	ice9_engine->ast->t.subvar = ice9_engine->ast->t.subvar_tail = NULL;
	ice9_engine->ast->t.subdeclist = ice9_engine->ast->t.subdeclist_tail = NULL;
	ice9_engine->ast->t.exp = ice9_engine->ast->t.exp_tail = NULL;
	//ice9_engine->ast->t.lv = NULL;
	//ice9_engine->ast->t.lv_exp_tail = NULL;
	//ice9_engine->ast->t.lv_exp_parsing=0;
	ice9_engine->ast->t.exparr = ice9_engine->ast->t.exparr_tail = NULL;
	//ice9_engine->ast->t.exparrass=NULL;
	//ice9_engine->ast->t.explv = ice9_engine->ast->t.explv_tail = NULL;
	//ice9_engine->ast->t.stmt_set = ice9_engine->ast->t.stmt_set_tail = NULL;


	ice9_engine->lgr=create_logger(ICE9_LOG_OUT_STD, ICE9_LOG_LEVEL_INFO, NULL);

	if(ice9_engine->lgr==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "ice9_init, out of memor3", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

}

_ast_type * na_type(char * id, _ast_typeid * typeid, int lineno){
	_ast_type * type;

	if(id==NULL || typeid==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_type, id or typeid is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	type = (_ast_type*)malloc(sizeof(_ast_type));
	if(type==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_type, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	type->id = id;
	type->typeid = typeid;
	type->lineno = lineno;

	/* append to temp list */
	if(ice9_engine->ast->t.array!=NULL){
		/* printf("---%d---",array->demension); */
		type->array = ice9_engine->ast->t.array;
		ice9_engine->ast->t.array = ice9_engine->ast->t.array_tail=NULL;
	}else{
		type->array=NULL; /* clear temp list */
	}

	return type;
}

_ast_typeid * na_typeid(char * id, int lineno){
	_ast_typeid * typeid;

	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_typeid, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	typeid = (_ast_typeid*)malloc(sizeof(_ast_typeid));
	if(typeid==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_typeid, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	typeid->id = id;
	typeid->lineno = lineno;

	return typeid;
}

_ast_array * na_array(int dimension, int lineno){
	_ast_array * na;

	na = (_ast_array *)malloc(sizeof(_ast_array));
	if(na==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_array, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	na->demension = dimension;
	na->lineno = lineno;
	na->next = NULL;

	/* append to temp list */
	if(ice9_engine->ast->t.array==NULL){
		ice9_engine->ast->t.array = ice9_engine->ast->t.array_tail = na;
	}else{
		ice9_engine->ast->t.array_tail->next = na;
		ice9_engine->ast->t.array_tail = na;
	}

	return na;
}

_ast_idlist * na_idlist(char * id, int lineno){
	_ast_idlist * idl=NULL;
	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_idlist, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	idl = (_ast_idlist*)malloc(sizeof(_ast_idlist));
	if(idl==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_idlist, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	idl->id = id;
	idl->lineno = lineno;
	idl->next=NULL;

	/* append to temp list */
	if(ice9_engine->ast->t.idlist==NULL){
		ice9_engine->ast->t.idlist = ice9_engine->ast->t.idlist_tail = idl;
	}else{
		idl->next = ice9_engine->ast->t.idlist;
		ice9_engine->ast->t.idlist = idl;
	}

	return idl;
}

_ast_subvar * na_subvar( _ast_typeid * typeid, int lineno){
	_ast_subvar * sv = NULL;
	if(typeid==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_subvar, typeid is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sv = (_ast_subvar *)malloc(sizeof(_ast_subvar));
	if(sv==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_subvar, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sv->lineno = lineno;
	sv->typeid = typeid;
	sv->next = NULL;

	/* idlist is mandatory in var */
	sv->idl = ice9_engine->ast->t.idlist;
	ice9_engine->ast->t.idlist = ice9_engine->ast->t.idlist_tail = NULL;

	/* array is optional in var */
	if(ice9_engine->ast->t.array!=NULL){
		sv->array = ice9_engine->ast->t.array;
		ice9_engine->ast->t.array = ice9_engine->ast->t.array_tail=NULL;
	}else{
		sv->array=NULL; /* clear temp list */
	}

	ice9_engine->ast->t.idlist = ice9_engine->ast->t.idlist_tail = NULL; /* clear temp list */

	/* append to temp list */
	if(ice9_engine->ast->t.subvar==NULL){
		ice9_engine->ast->t.subvar = ice9_engine->ast->t.subvar_tail = sv;
	}else{
		ice9_engine->ast->t.subvar_tail->next = sv;
		ice9_engine->ast->t.subvar_tail = sv;
	}

	return sv;
}

_ast_subdeclist * na_subdelist(_ast_typeid * typeid, int lineno){
	_ast_subdeclist * sdl = NULL;

	if(typeid==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_subdelist, typeid is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sdl = (_ast_subdeclist*)malloc(sizeof(_ast_subdeclist));
	if(sdl==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_subdelist, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sdl->typeid = typeid;
	sdl->lineno=lineno;

	/* idlist is mandatory in declist */
	sdl->idl = ice9_engine->ast->t.idlist;
	ice9_engine->ast->t.idlist = ice9_engine->ast->t.idlist_tail = NULL;

	/* append to temp list */
	if(ice9_engine->ast->t.subdeclist==NULL){
		ice9_engine->ast->t.subdeclist = ice9_engine->ast->t.subdeclist_tail = sdl;
	}else{
		ice9_engine->ast->t.subdeclist_tail->next = sdl;
		ice9_engine->ast->t.subdeclist_tail = sdl;
	}

	return sdl;
}

_ast_forward * na_forward(char * fname, _ast_typeid * rtype /* this is optional, could be NULL */, int lineno){
	_ast_forward * f = NULL;

	if(fname==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_forward, fname is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	f = (_ast_forward*)malloc(sizeof(_ast_forward));
	if(f==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_forward, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	f->fname = fname;
	f->rtype = rtype;
	f->lineno = lineno;

	/* declist is optional */
	if(ice9_engine->ast->t.subdeclist!=NULL){
		f->sdl = ice9_engine->ast->t.subdeclist;
		ice9_engine->ast->t.subdeclist = ice9_engine->ast->t.subdeclist_tail = NULL;  /* clear temp list */
	}else{
		f->sdl = NULL;
	}

	return f;
}

_ast_proc * na_proc(char * id, _ast_typeid * rtype, _ast_proc_decl * proc_decl, _ast_stmt_set * ss, int lineno){
	_ast_proc * proc;

	if(id==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_proc, id is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	proc = (_ast_proc*)malloc(sizeof(_ast_proc));
	if(proc==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_proc, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	proc->id=id;
	proc->lineno=lineno;
	proc->rtype=rtype;
	proc->proc_decl=proc_decl;
	proc->ss=ss;
	if(ice9_engine->ast->t.subdeclist!=NULL){
		proc->sdl = ice9_engine->ast->t.subdeclist;
		ice9_engine->ast->t.subdeclist = ice9_engine->ast->t.subdeclist_tail = NULL;  /* clear temp list */
	}else{
		proc->sdl = NULL;
	}

	return proc;
}

_ast_exp * na_exp(int ntype, int ivalue, char * svalue, char * id, _ast_exp * lexp, _ast_exp * rexp, int lineno){
	_ast_exp * exp = NULL;

	if(ntype<AST_NT_LVALUE || ntype>AST_NT_PAREN){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_exp, illegal ntype", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	exp=(_ast_exp*)malloc(sizeof(_ast_exp));
	if(exp==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_exp, out of memory1", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}
	exp->id=id;
	exp->ivalue=ivalue;
	exp->lexp=lexp;
	exp->rexp=rexp;
	exp->lineno=lineno;
	exp->ntype=ntype;
	/* exp->rexp=rexp; */
	exp->next=NULL;
	//exp->lvalue=NULL;

	if(svalue!=NULL){
		exp->svalue = (char*)malloc(sizeof(char)*(strlen(svalue)-1));
		if(exp->svalue==NULL){
#if ICE9_DEBUG
			ice9_log_level(ice9_engine->lgr, "na_exp, out of memory2", ICE9_LOG_LEVEL_ERROR);
#endif
			exit(1);
		}
		memset(exp->svalue, 0, sizeof(char)*(strlen(svalue)-1));
		memcpy(exp->svalue, svalue+1, strlen(svalue)-2);
	}

	if(ntype==AST_NT_LVALUE  && lexp!=NULL){
		exp->lexp->next=ice9_engine->ast->t.exparr;
		ice9_engine->ast->t.exparr = ice9_engine->ast->t.exparr_tail = NULL; /* clear temp list */
	}

	/* func call with parameters */
	if(ntype==AST_NT_EPROC && lexp!=NULL){
		/* exp->rexp=ice9_engine->ast->t.exp; */
		exp->lexp->next=ice9_engine->ast->t.exp;
		ice9_engine->ast->t.exp = ice9_engine->ast->t.exp_tail = NULL; /* clear temp list */
	}

#if ICE9_DEBUG
		//ice9_log_level(ice9_engine->lgr, "na_exp, finish", ICE9_LOG_LEVEL_DEBUG);
#endif

	return exp;
}

_ast_assign * na_assign(char * id, _ast_exp * lexp, _ast_exp * rexp, int lineno){
	_ast_assign * ass = NULL;

	if(id==NULL || rexp==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_assign, id or rexp is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ass = (_ast_assign*)malloc(sizeof(_ast_assign));
	if(ass==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_assign, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ass->id=id;
	ass->lexp=lexp;
	ass->rexp=rexp;
	ass->lineno=lineno;

	return ass;
}

_ast_assign * na_assign2(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * rexp, int lineno){
	_ast_assign * ass = NULL;
	ass = na_assign(id, lexp1, rexp, lineno);
	ass->lexp->next = lexp2;
//printf("********** %na_assign2, id:%s****\n", lexp2->id);
	return ass;
}
_ast_assign * na_assign3(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * lexp3, _ast_exp * rexp, int lineno){
	_ast_assign * ass = NULL;
	ass = na_assign(id, lexp1, rexp, lineno);
	ass->lexp->next = lexp2;
	lexp2->next=lexp3;

	return ass;
}
_ast_assign * na_assign4(char * id, _ast_exp * lexp1, _ast_exp * lexp2, _ast_exp * lexp3, _ast_exp * lexp4, _ast_exp * rexp, int lineno){
	_ast_assign * ass = NULL;
	ass = na_assign(id, lexp1, rexp, lineno);
	ass->lexp->next = lexp2;
	lexp2->next=lexp3;
	lexp3->next=lexp4;

	return ass;
}

_ast_write * na_write(int newline, _ast_exp * e, int lineno){
	_ast_write * w=NULL;
	if(e==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_write, e is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	w = (_ast_write*)malloc(sizeof(_ast_write));
	if(w==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_write, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	w->newline = newline;
	w->exp = e;
	w->lineno=lineno;

	return w;
}

_ast_do * na_do(_ast_exp * exp, _ast_stmt_set * ss, int lineno){
	_ast_do * d=NULL;

#if ICE9_DEBUG
	//ice9_log_level(ice9_engine->lgr, "na_do, start", ICE9_LOG_LEVEL_DEBUG);
#endif

	if(exp==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_do, exp is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	d=(_ast_do*)malloc(sizeof(_ast_do));
	if(d==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_do, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	d->lineno;
	d->exp=exp;
	d->ss=ss;

#if ICE9_DEBUG
	//ice9_log_level(ice9_engine->lgr, "na_do, end", ICE9_LOG_LEVEL_DEBUG);
#endif
	return d;
}

_ast_fa * na_fa(char * id, _ast_exp * fexp, _ast_exp * texp, _ast_stmt_set * ss, int lineno){
	_ast_fa * f = NULL;

	if(id==NULL || fexp==NULL || texp==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_fa, id or fexp or texp is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	f = (_ast_fa*)malloc(sizeof(_ast_fa));
	if(f==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_fa, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	f->id=id;
	f->fexp=fexp;
	f->texp=texp;
	f->ss=ss;
	f->lineno=lineno;

	return f;
}

_ast_if * na_if(_ast_exp * exp, _ast_stmt_set * iss, _ast_box_stmt_set * bss, _ast_stmt_set * ess, int lineno){
	_ast_if * i = NULL;

	if(exp==NULL || iss==NULL ){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_if, exp or iss is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	i = (_ast_if*)malloc(sizeof(_ast_if));
	if(i==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_if, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	i->exp = exp;
	i->iss = iss;
	i->bss=bss;
	i->ess=ess;
	i->lineno=lineno;

	return i;
}

_ast_sub_proc_decl * na_sub_proc_decl(int ntype, _ast_type * type, int lineno){
	_ast_sub_proc_decl * spd = NULL;

	spd = (_ast_sub_proc_decl*)malloc(sizeof(_ast_sub_proc_decl));
	if(spd==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_sub_proc_decl, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	spd->ntype = ntype;
	spd->lineno=lineno;
	spd->type=type;
	spd->var=ice9_engine->ast->t.subvar;
	ice9_engine->ast->t.subvar = ice9_engine->ast->t.subvar_tail = NULL; /* clear temp list */

	return spd;
}

_ast_box_stmt * na_box_stmt(_ast_exp * exp, _ast_stmt_set * ss, int lineno){
	_ast_box_stmt * bs = NULL;

	if(exp==NULL || ss==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_box_stmt, exp or ss is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	bs = (_ast_box_stmt*)malloc(sizeof(_ast_box_stmt));

	bs->lineno;
	bs->exp=exp;
	bs->ss=ss;

	return bs;
}

void append_exp(_ast_exp * e){
	if(e==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "append_exp, e is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	if(ice9_engine->ast->t.exp==NULL){
		ice9_engine->ast->t.exp = ice9_engine->ast->t.exp_tail = e;
	}else{
		e->next = ice9_engine->ast->t.exp;
		ice9_engine->ast->t.exp = e;
	}
}

void append_exparr(_ast_exp * e){
	if(e==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "append_exparr, e is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	if(ice9_engine->ast->t.exparr==NULL){
		ice9_engine->ast->t.exparr = ice9_engine->ast->t.exparr_tail = e;
	}else{
		e->next = ice9_engine->ast->t.exparr;
		ice9_engine->ast->t.exparr = e;
	}
}

void append_stmt_decl_2root(_ast_stmt_declaration * sd){
	if(sd==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "append_stmt_decl_2root, sd is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	check_ice9_engine();

	if(ice9_engine->ast->stmt_declarations==NULL)
		ice9_engine->ast->stmt_declarations = ice9_engine->ast->stmt_declarations_tail = sd;
	else{
		ice9_engine->ast->stmt_declarations_tail->next = sd;
		ice9_engine->ast->stmt_declarations_tail = sd;
	}
}

void append_stmt_2root(_ast_stmt * s){
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "append_stmt_2root, s is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	check_ice9_engine();

	if(ice9_engine->ast->stmts==NULL)
		ice9_engine->ast->stmts = ice9_engine->ast->stmts_tail = s;
	else{
		ice9_engine->ast->stmts_tail->next = s;
		ice9_engine->ast->stmts_tail = s;
	}

#if ICE9_DEBUG
		//ice9_log_level(ice9_engine->lgr, "append_stmt_2root, finish", ICE9_LOG_LEVEL_DEBUG);
#endif
}

/*void append_stmt_set(_ast_stmt * s){
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "append_stmt_2root, s is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	check_ice9_engine();

	if(ice9_engine->ast->t.stmt_set==NULL)
		ice9_engine->ast->t.stmt_set = ice9_engine->ast->t.stmt_set_tail = s;
	else{
		ice9_engine->ast->t.stmt_set_tail->next = s;
		ice9_engine->ast->t.stmt_set_tail = s;
	}
}*/

_ast_stmt_declaration * _create_stmt_decl(){
	_ast_stmt_declaration * sd;

	sd = (_ast_stmt_declaration*)malloc(sizeof(_ast_stmt_declaration));
	if (sd == NULL) {
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "_create_stmt_decl, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sd->forward=NULL;
	sd->lineno=-1;
	sd->next=NULL;
	sd->ntype=-1;
	sd->proc=NULL;
	sd->type=NULL;
	sd->var=NULL;

	return sd;
}

_ast_stmt_declaration * na_stmt_decl_type(_ast_type * type, int lineno){
	_ast_stmt_declaration * sd;

	if(type==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_decl_type, type is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sd = _create_stmt_decl();

	sd->ntype = AST_NT_TYPE;
	sd->type = type;
	sd->lineno = lineno;

	return sd;
}

_ast_stmt_declaration * na_stmt_decl_var(int lineno){
	_ast_stmt_declaration * sd;

	sd = _create_stmt_decl();

	sd->ntype = AST_NT_VAR;
	sd->lineno = lineno;
	sd->var = ice9_engine->ast->t.subvar;
	ice9_engine->ast->t.subvar = ice9_engine->ast->t.subvar_tail = NULL; /* clear temp list */

	return sd;
}

_ast_stmt_declaration * na_stmt_decl_forward(_ast_forward * f, int lineno){
	_ast_stmt_declaration * sd;

	if(f==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_decl_forward, f is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sd = _create_stmt_decl();

	sd->ntype = AST_NT_FORWARD;
	sd->lineno = lineno;
	sd->forward = f;

	return sd;
}

_ast_stmt_declaration * na_stmt_decl_proc(_ast_proc * p, int lineno){
	_ast_stmt_declaration * sd;

	if(p==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_decl_proc, p is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	sd = _create_stmt_decl();
	sd->ntype = AST_NT_PROC;
	sd->lineno = lineno;
	sd->proc=p;

	return sd;
}

_ast_stmt * _create_stmt(){
	_ast_stmt * stmt = NULL;

	stmt = (_ast_stmt*)malloc(sizeof(_ast_stmt));
	if(stmt==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "_create_stmt, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt->assign=NULL;
	stmt->ddo=NULL;
	stmt->exp=NULL;
	stmt->fa=NULL;
	stmt->iif=NULL;
	stmt->lineno=-1;
	stmt->next=NULL;
	stmt->ntype=-1;
	stmt->write=NULL;

	stmt->brk_flag=BREAK_INIT;
	//stmt->rt_flag=RETURN_INIT;

	return stmt;
}

_ast_stmt * na_stmt_exp(_ast_exp * e, int lineno){
	_ast_stmt * stmt = NULL;

	if(e==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_exp, e is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_EXP;
	stmt->exp=e;
	stmt->lineno=lineno;

#if ICE9_DEBUG
	//ice9_log_level(ice9_engine->lgr, "na_stmt_exp, finish", ICE9_LOG_LEVEL_DEBUG);
#endif

	return stmt;
}

_ast_stmt * na_stmt_assign(_ast_assign * a, int lineno){
	_ast_stmt * stmt = NULL;

	if(a==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_assign, a is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_ASSIGN;
	stmt->assign=a;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt * na_stmt_write(_ast_write * w, int lineno){
	_ast_stmt * stmt = NULL;

	if(w==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_write, w is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_WRITE;
	stmt->write=w;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt * na_stmt_break(int lineno){
	_ast_stmt * stmt = NULL;

	stmt = _create_stmt();
	stmt->ntype=AST_NT_BREAK;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt * na_stmt_exit(int lineno){
	_ast_stmt * stmt = NULL;

	stmt = _create_stmt();
	stmt->ntype=AST_NT_EXIT;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt * na_stmt_return(int lineno){
	_ast_stmt * stmt = NULL;

	stmt = _create_stmt();
	stmt->ntype=AST_NT_RETURN;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt * na_stmt_do(_ast_do * d, int lineno){
	_ast_stmt * stmt = NULL;

	if(d==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_do, d is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_DO;
	stmt->lineno=lineno;
	stmt->ddo = d;

#if ICE9_DEBUG
		//ice9_log_level(ice9_engine->lgr, "na_stmt_do, end", ICE9_LOG_LEVEL_DEBUG);
#endif

	return stmt;
}

_ast_stmt * na_stmt_fa(_ast_fa * f, int lineno){
	_ast_stmt * stmt = NULL;

	if(f==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_fa, f is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_FA;
	stmt->lineno=lineno;
	stmt->fa=f;

	return stmt;
}

_ast_stmt * na_stmt_if(_ast_if * i, int lineno){
	_ast_stmt * stmt = NULL;

	if(i==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_if, i is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	stmt = _create_stmt();
	stmt->ntype=AST_NT_IF;
	stmt->lineno=lineno;
	stmt->iif=i;

	return stmt;
}

_ast_stmt * na_stmt_empty(int lineno){
	_ast_stmt * stmt = NULL;

	stmt = _create_stmt();
	stmt->ntype=AST_NT_EMPTY;
	stmt->lineno=lineno;

	return stmt;
}

_ast_stmt_set * na_stmt_set(_ast_stmt_set * lss, _ast_stmt * rs, int lineno){
	_ast_stmt_set * ss = NULL;

	if(rs==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_set, rs is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ss = (_ast_stmt_set*)malloc(sizeof(_ast_stmt_set));
	if(ss==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_stmt_set, out of memeory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	ss->lss=lss;
	ss->rs=rs;
	ss->lineno=lineno;

#if ICE9_DEBUG
		//ice9_log_level(ice9_engine->lgr, "na_stmt_set, end", ICE9_LOG_LEVEL_DEBUG);
#endif

	return ss;
}

_ast_box_stmt_set * na_box_stmt_set(_ast_box_stmt_set * lbss, _ast_box_stmt * rbs, int lineno){
	_ast_box_stmt_set * bss = NULL;

	if(rbs==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_box_stmt_set, rbs is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	bss = (_ast_box_stmt_set*)malloc(sizeof(_ast_box_stmt_set));
	if(bss==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_box_stmt_set, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	bss->lbss=lbss;
	bss->rbs=rbs;
	bss->lineno=lineno;

	return bss;
}

_ast_proc_decl * na_proc_decl(_ast_proc_decl * lpd, _ast_sub_proc_decl * rspd, int lineno){
	_ast_proc_decl * pd = NULL;

	if(rspd==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_proc_decl, rspd is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	pd = (_ast_proc_decl*)malloc(sizeof(_ast_proc_decl));
	if(pd==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "na_proc_decl, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	pd->lineno=lineno;
	pd->lpd=lpd;
	pd->rspd=rspd;

	return pd;
}

_symbol * n_symbol(char * name){
	_symbol * s = NULL;

	if(name==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "n_symbol, name is NULL", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	s = (_symbol*)malloc(sizeof(_symbol));
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "n_symbol, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	//s->stype=stype;
	s->name = name; /* strdup ?? */

	s->array=NULL;
	s->var_type=NULL;
	s->p_type=NULL;
	s->fwd_unmatched=0;
	s->fwd=NULL;
	s->is_fa_loop_var=0;
	s->proc_rtype=NULL;
	s->proc=NULL;
	s->is_fwd=0;
	s->is_proc=0;

	s->cgp=NULL; /* to assist code generation */

	return s;
}

void destruct_symbol(_symbol * s){
	if(s==NULL)
		return;

	free(s);
}

_scope * n_scope(int stype, int ss, int init_type, int init_var, int init_proc){
	_scope * s = NULL;

	s = (_scope*)malloc(sizeof(_scope));
	if(s==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "n_scope, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	s->stype = stype;
	s->proc_table=s->var_table=s->type_table=NULL;
	if(init_proc)
		s->proc_table = ht_create(ss);
	if(init_type)
		s->type_table = ht_create(ss);
	if(init_var)
		s->var_table = ht_create(ss);

	return s;
}

void destruct_scope(_scope * s){

	if(s==NULL)
		return;
}

_cg_temp * n_cg_temp(){
	_cg_temp * cgp=NULL;
	cgp = (_cg_temp*)malloc(sizeof(_cg_temp));
	if(cgp==NULL){
#if ICE9_DEBUG
		ice9_log_level(ice9_engine->lgr, "n_cg_temp, out of memory", ICE9_LOG_LEVEL_ERROR);
#endif
		exit(1);
	}

	cgp->d_index=0;
	cgp->i_index=0;
	cgp->local=0;
	cgp->ar_size=0;
	cgp->lvar_proc=NULL;
	cgp->lvar_fp_offset=0;
	cgp->arr_dim=NULL;
	cgp->arr_dim_sum=0;
	cgp->str_len = 0;
	cgp->str_start_dmem = 0;
	//cgp->exp_value_dmem=0;
	return cgp;
}
