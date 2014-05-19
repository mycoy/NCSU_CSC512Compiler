/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P2
 *
 * ice9.
 */
#include <stdlib.h>
#include "ice9.h"

extern _ice9_core * ice9_engine;

void print_ast_exp(_ast_exp * exp){
	_ast_exp * te=NULL;

	if(exp==NULL)
		return;

	switch(exp->ntype){
	case AST_NT_LVALUE:
		printf("  AST_NT_LVALUE{id(%s) ", exp->id);

		if( (te=exp->lexp)!=NULL ){
			while(te!=NULL){
				printf(" [");
				print_ast_exp(te);
				printf("] ");
				te=te->next;
			}
		}
		printf(" }  ");
		break;
	case AST_NT_INT:
		printf(" AST_NT_INT{(%d)} ", exp->ivalue);
		break;
	case AST_NT_TRUE:
		printf(" AST_NT_TRUE ");
		break;
	case AST_NT_FALSE:
		printf(" AST_NT_FALSE ");
		break;
	case AST_NT_STR:
		printf(" AST_NT_STR{(%s)} ", exp->svalue);
		break;
	case AST_NT_READ:
		printf(" AST_NT_READ ");
		break;
	case AST_NT_UMINUS:
		printf("  AST_NT_UMINUS{ ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_QUEST:
		printf("  AST_NT_QUEST{ ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_EPROC:
		printf("  AST_NT_EPROC{fname(%s) ", exp->id);
		/*if(exp->lexp!=NULL)
			print_ast_exp(exp->lexp);*/
		if( (te=exp->lexp)!=NULL ){
			while(te!=NULL){
				printf(" , ");
				print_ast_exp(te);
				te=te->next;
			}
		}
		printf(" }  ");
		break;
	case AST_NT_PLUS:
		printf("  AST_NT_PLUS{ ");
		print_ast_exp(exp->lexp);
		printf(" + ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_MINUS:
		printf("  AST_NT_MINUS{ ");
		print_ast_exp(exp->lexp);
		printf(" - ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_MUL:
		printf("  AST_NT_MUL{ ");
		print_ast_exp(exp->lexp);
		printf(" * ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_DIV:
		printf("  AST_NT_DIV{ ");
		print_ast_exp(exp->lexp);
		printf(" / ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_MOD:
		printf("  AST_NT_MOD{ ");
		print_ast_exp(exp->lexp);
		printf(" % ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_EQU:
		printf("  AST_NT_EQU{ ");
		print_ast_exp(exp->lexp);
		printf(" = ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_NE:
		printf("  AST_NT_NE{ ");
		print_ast_exp(exp->lexp);
		printf(" != ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_GT:
		printf("  AST_NT_GT{ ");
		print_ast_exp(exp->lexp);
		printf(" > ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_LT:
		printf("  AST_NT_LT{ ");
		print_ast_exp(exp->lexp);
		printf(" < ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_GE:
		printf("  AST_NT_GE{ ");
		print_ast_exp(exp->lexp);
		printf(" >= ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_LE:
		printf("  AST_NT_LE{ ");
		print_ast_exp(exp->lexp);
		printf(" <= ");
		print_ast_exp(exp->rexp);
		printf(" }  ");
		break;
	case AST_NT_PAREN:
		printf("  AST_NT_PAREN{( ");
		print_ast_exp(exp->lexp);
		printf(" )}  ");
		break;
	}
}

void print_ast_sub_proc_decl(_ast_sub_proc_decl * spd){
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;

	if(spd==NULL)
		return;

	if (spd->ntype == AST_NT_TYPE) {
		//printf("===============\n\n");
		printf("  proc-decl(AST_NT_TYPE)  ");
		//printf("===============\n\n");
		printf(" id(%s) typeid(%s) ", spd->type->id, spd->type->typeid->id);
		//printf("===============\n\n");
		if (spd->type->array != NULL) {
			printf("   ");
			ap = spd->type->array;
			while (ap != NULL) {
				printf("[%d]", ap->demension);
				ap = ap->next;
			}
		}
	}

	if (spd->ntype == AST_NT_VAR) {
		printf("   proc-decl(AST_NT_VAR)  ");
		sv = spd->var;
		while (sv != NULL) {
			idl = sv->idl;
			printf("  idlist(");
			while (idl != NULL) {
				printf("%s ", idl->id);
				idl = idl->next;
			}
			printf(") ");

			printf("  typeid(%s) ", sv->typeid->id);
			ap = sv->array;
			while (ap != NULL) {
				printf("[%d]", ap->demension);
				ap = ap->next;
			}
			printf(";	");
			sv = sv->next;
		}
	}
}

void print_ast_proc_decl(_ast_proc_decl * pd){
	if(pd==NULL)
		return;
	if(pd->lpd!=NULL){
		print_ast_proc_decl(pd->lpd->lpd);
		print_ast_sub_proc_decl(pd->lpd->rspd);
	}
	print_ast_sub_proc_decl(pd->rspd);
}

void print_ast_stmt_decl(_ast_stmt_declaration * sdp){
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;
	_ast_forward * fw;
	_ast_subdeclist * sdl;
	//_ast_assign * ass;
	//_ast_exp * exp;

	while (sdp != NULL) {

		if (sdp->ntype == AST_NT_TYPE) {
			printf("stmt_decl lineno:%d, ", sdp->lineno);
			printf("  stmt-type(AST_NT_TYPE)  ");
			printf("\n");

			printf("  type_decl's id(%s) typeid(%s) ", sdp->type->id,
					sdp->type->typeid->id);
			if (sdp->type->array != NULL) {
				printf("   ");
				ap = sdp->type->array;
				while (ap != NULL) {
					printf("[%d]", ap->demension);
					ap = ap->next;
				}
			}
		}

		if (sdp->ntype == AST_NT_VAR) {
			printf("stmt_decl lineno:%d, ", sdp->lineno);
			printf("  stmt-type(AST_NT_VAR)  ");
			printf("\n");

			sv = sdp->var;
			while (sv != NULL) {
				idl = sv->idl;
				printf("  idlist(");
				while (idl != NULL) {
					printf("%s ", idl->id);
					idl = idl->next;
				}
				printf(") ");

				printf("  typeid(%s) ", sv->typeid->id);
				ap = sv->array;
				while (ap != NULL) {
					printf("[%d]", ap->demension);
					ap = ap->next;
				}
				printf(";	");
				sv = sv->next;
			}
		}

		if (sdp->ntype == AST_NT_FORWARD) {
			printf("stmt_decl lineno:%d, ", sdp->lineno);
			printf("  stmt-type(AST_NT_FORWARD)  ");
			printf("\n");

			fw = sdp->forward;
			sdl = fw->sdl;
			printf("  fname(%s), ", fw->fname);
			if (fw->rtype != NULL)
				printf(" return-type(%s)   ", fw->rtype->id);
			sdl = fw->sdl;
			while (sdl != NULL) {
				idl = sdl->idl;
				printf("idlist(");
				while (idl != NULL) {
					printf("%s ", idl->id);
					idl = idl->next;
				}
				printf(") ");

				printf("typeid(%s) ", sdl->typeid->id);
				printf(";	");
				sdl = sdl->next;
			}
		}

		if (sdp->ntype == AST_NT_PROC) {
			printf("stmt_decl lineno:%d, ", sdp->proc->lineno);
			printf("  stmt-type(AST_NT_PROC)  proc's-name:%s  ",sdp->proc->id);
			printf("\n");

			if(sdp->proc->sdl!=NULL){
				printf("  proc's parameters:   ");
				sdl = sdp->proc->sdl;
				while (sdl != NULL) {
					idl = sdl->idl;
					printf("idlist(");
					while (idl != NULL) {
						printf("%s ", idl->id);
						idl = idl->next;
					}
					printf(") ");

					printf("typeid(%s) ", sdl->typeid->id);
					printf(";	");
					sdl = sdl->next;
				}
				printf("\n");
			}

			if(sdp->proc->rtype!=NULL){

				printf("  proc's return type(%s)  ", sdp->proc->rtype->id);
			}

			if(sdp->proc->proc_decl!=NULL){
				printf("  proc's decl{  ");
				print_ast_proc_decl(sdp->proc->proc_decl);
				printf("   }  ");
			}

			if(sdp->proc->ss!=NULL){
				printf("  proc's stmt_set{  ");
				print_ast_stmt_set(sdp->proc->ss);
				printf("   }  ");
			}

			printf("\n");
		}

		printf("\n");
		sdp = sdp->next;
	}
}

void print_ast_stmt_set(_ast_stmt_set * ssp){
	if(ssp==NULL)
		return;

	if(ssp->lss!=NULL){
		print_ast_stmt_set(ssp->lss->lss);
		print_ast_stmt(ssp->lss->rs);
	}
	print_ast_stmt(ssp->rs);
}

void print_ast_box_stmt(_ast_box_stmt * bs){
	if(bs==NULL)
		return;
	printf("  box_stmt's start--->{ \n");
	printf("  box_stmt's exp->  ");
	print_ast_exp(bs->exp);
	printf("  box_stmt's stmt_set->  ");
	print_ast_stmt_set(bs->ss);
	printf("  box_stmt's end}<--- \n");
}

void print_ast_box_stmt_set(_ast_box_stmt_set * bss){
	if(bss==NULL)
		return;

	if(bss->lbss!=NULL){
		print_ast_box_stmt_set(bss->lbss->lbss);
		print_ast_box_stmt(bss->lbss->rbs);
	}
	print_ast_box_stmt(bss->rbs);
}

void print_ast_stmt(_ast_stmt * sp){
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;
	_ast_forward * fw;
	_ast_subdeclist * sdl;
	_ast_assign * ass;
	_ast_exp * exp;

	while (sp != NULL) {

		switch (sp->ntype) {
		case AST_NT_EXP:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_EXP)  ");
			printf("\n");
			print_ast_exp(sp->exp);
			printf("\n");
			break;
		case AST_NT_ASSIGN:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_ASSIGN)  ");
			printf("\n");
			ass = sp->assign;
			printf("  id(%s) ", ass->id);

			if ((exp = ass->lexp) != NULL) {  //assign's left side is an array
				while (exp != NULL) {
					printf(" [ ");
					print_ast_exp(exp);
					printf(" ] ");
					exp = exp->next;
				}
			}

			printf("  :=  ");
			if (ass->rexp != NULL)
				print_ast_exp(ass->rexp);
			printf("\n");
			break;
		case AST_NT_WRITE:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_WRITE) %s newline  ",
					sp->write->newline == 1 ? "with" : "without");
			print_ast_exp(sp->write->exp);
			break;
		case AST_NT_BREAK:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_BREAK)  ");
			break;
		case AST_NT_EXIT:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_EXIT)  ");
			break;
		case AST_NT_RETURN:
			printf("stmt lineno:%d, ", sp->lineno);
			printf("  stmt-type(AST_NT_RETURN)  ");
			break;
		case AST_NT_DO:
			printf("stmt lineno:%d, ", sp->ddo->exp->lineno);
			printf("  stmt-type(AST_NT_DO)  ");
			printf(" do's condition-> ");
			print_ast_exp(sp->ddo->exp);
			printf(" do's body start--->{\n");
			print_ast_stmt_set(sp->ddo->ss);
			printf(" do's body end}<---\n");
			break;
		case AST_NT_FA:
			printf("stmt lineno:%d, ", sp->fa->fexp->lineno);
			printf("  stmt-type(AST_NT_FA),  condition variable(%s)  ", sp->fa->id);
			printf(" fa's start conditoin->  ");
			print_ast_exp(sp->fa->fexp);
			printf(" fa's end conditoin->  ");
			print_ast_exp(sp->fa->texp);
			printf(" fa's body start--->{ \n");
			print_ast_stmt_set(sp->fa->ss);
			printf(" fa's body end}<--- \n");
			break;
		case AST_NT_IF:
			printf("stmt lineno:%d, ", sp->iif->exp->lineno);
			printf("  stmt-type(AST_NT_IF)  ");
			printf(" if's condition-> ");
			print_ast_exp(sp->iif->exp);
			printf(" if's stmt_set start--->{\n");
			print_ast_stmt_set(sp->iif->iss);
			printf(" if's stmt_set end}<---\n");

			if( sp->iif->bss!=NULL) {
				printf(" if's box_stmt_set start--->{ \n");
				print_ast_box_stmt_set(sp->iif->bss);
				printf(" if's box_stmt_set end}<--- \n");
			}
			if(sp->iif->ess!=NULL){
				printf(" else's stmt_set start--->{ \n");
				print_ast_stmt_set(sp->iif->ess);
				printf(" else's stmt_set end}<--- \n");
			}

			break;
		}
		printf("\n");
		sp = sp->next;
	}
}

/* for debug */
void print_ast(){
	_ast_stmt_declaration * sdp;
	_ast_stmt * sp;
	_ast_array *ap;
	_ast_subvar *sv;
	_ast_idlist * idl;
	_ast_forward * fw;
	_ast_subdeclist * sdl;
	_ast_assign * ass;
	_ast_exp * exp;

	sdp=ice9_engine->ast->stmt_declarations;
	sp = ice9_engine->ast->stmts;

	print_ast_stmt_decl(sdp);
	print_ast_stmt(sp);

}
