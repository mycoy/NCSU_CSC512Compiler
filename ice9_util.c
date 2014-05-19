/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * util.
 */

#include <stdlib.h>
#include <string.h>
#include "ice9_util.h"
#include "ice9.h"  /* should not be here if redesigned */
/*
extern struct _symbol_st;
extern struct _ast_array_st;
extern struct _ast_forward_st;
extern struct _ast_proc_st;*/

/* ----------start define hash-table structs---------- */
typedef struct _ht_list_elem_st{
	char * key;  /* hash key, unique, but the actual hash value might be duplicated */
	void * data;  /* will be _symbol struct */
	struct _ht_list_elem_st * next;
}_ht_list_elem;

typedef struct _ht_list_st{
	int elem_number; /* number of elements in the list */
	_ht_list_elem * l_head;
	_ht_list_elem * l_tail;

}_ht_list;

typedef struct _hash_table_st {
	_ht_list ** array;
	int size; /* array's length, not actual number of elements in the hash table.  tiny, small, medium, large */
} _hash_table;
/* ----------end define hash-table structs---------- */


/* ----------start define stack structs---------- */
typedef struct _stack_elem_st{ /* used to handle scope in ice9 */
	void * data; /* will be _scope struct */
	struct _stack_elem_st * topper;
	struct _stack_elem_st * lowwer;
}_stack_elem;

typedef struct _stack_st{
	int size;
	_stack_elem * top;
}_stack;

/* ----------end define stack structs---------- */



/* ----------start define hash-table functions---------- */
/* reference: http://blog.csdn.net/gdujian0119/article/details/6777239
 * http://www.cse.yorku.ca/~oz/hash.html */
unsigned int SDBMHash(char *str) {
	unsigned int hash = 0;

	while (*str) {
		hash = (*str++) + (hash << 6) + (hash << 16) - hash;
	}

	return (hash & 0x7FFFFFFF);
}


_ht_list * create_htlist(){
	_ht_list * l_t = NULL;

	l_t = (_ht_list*)malloc(sizeof(_ht_list));

	if(l_t==NULL){
#if LIST_DEBUG
		printf("create_htlist, memory allocation error\n");
#endif
		exit(1);
	}
	l_t->elem_number = 0;
	l_t->l_head = NULL;
	l_t->l_tail = NULL;

	return l_t;
}

/* if using "yytext", remember to duplicate the string before passing it to this function */
_ht_list_elem * create_htlist_elem(char* key, void * data){
	_ht_list_elem * le_t = NULL;

	if(key==NULL || data==NULL){
#if LIST_DEBUG
		printf("create_htlist_elem, key or data is NULL\n");
#endif
		return NULL;
	}

	le_t = (_ht_list_elem*)malloc( sizeof(_ht_list_elem) );
	if(le_t==NULL){
#if LIST_DEBUG
		printf("create_htlist_elem, memory allocation error\n");
#endif
		exit(1);
	}

	le_t->key = key;
	le_t->data = data;
	le_t->next = NULL;

	return le_t;
}

int en_htlist(_ht_list_elem * e_t, _ht_list * l_t){
	_ht_list * l = NULL;
	_ht_list_elem * le = NULL;

	if(e_t==NULL || l_t==NULL){
#if LIST_DEBUG
		printf("en_htlist, list_elem or list is NULL.\n");
#endif
		return 1;
	}

    l = l_t;
    le = e_t;
    if(l->elem_number==0){ /* if list empty */
    	l->l_head = l->l_tail = le;
    }else{
    	l->l_tail->next = le;
    	l->l_tail = le;
    }
    l->elem_number++;

    return 0;
}

int del_htlist_elem(void * le_t, _ht_list * l_t){
	_ht_list * l = NULL;
	_ht_list_elem * lp1=NULL, * lp2=NULL, * to_del=NULL;
	int i, size;
	void * le = NULL;

	if(le_t==NULL || l_t==NULL){
#if LIST_DEBUG
		printf("del_htlist_elem, list_elem or list is NULL\n");
#endif
		return 1;
	}

    l = l_t;
    le = le_t;

    size = l->elem_number;
    lp1 = l->l_head;
    for(i=0; i< size; i++){
    	if( lp1->data == le ){ /* if found */
    		to_del=lp1;
    		if(lp1==l->l_head){ /* if to_del is the head */
    			if(l->elem_number==1){ /* if only one elem in the list */
    				l->l_head = l->l_tail = NULL;
    			}else{
    				l->l_head = l->l_head->next;
    			}
    			break;
    		} else if(lp1==l->l_tail){
    			l->l_tail = lp2;
    			lp2->next = NULL; /* added at 20:20PM Dec6 2013 */
    			break;
    		}else{
    			lp2->next = lp1->next;
    			break;
    		}
    	}
    	lp2=lp1;
    	lp1 = lp1->next;
    } /* end for */

    if(to_del!=NULL){
    	to_del->next = NULL; /* cut relation with the list */
    	l->elem_number--;
    }

    return to_del!=NULL ? 0 : 1;
}



i9_hash_table ht_create(int size){
	_hash_table * ht = NULL;

	if(size<=0){
#if ICE9_UTIL_DEBUG
		printf("ht_create, size<=0 .\n");
#endif
		return NULL;
	}

	ht = (_hash_table*)malloc(sizeof(_hash_table));
	if(ht==NULL){
#if ICE9_UTIL_DEBUG
		printf("ht_create, out of memory1.\n");
#endif
		exit(1);
	}

	ht->array = (_ht_list**)malloc( sizeof(_ht_list*)*size + 1 );
	if(ht->array==NULL){
#if ICE9_UTIL_DEBUG
		printf("ht_create, out of memory2.\n");
#endif
		exit(1);
	}
	memset(ht->array, 0, sizeof(_ht_list*)*size + 1);
	ht->size = size;

	return (i9_hash_table)ht;
}

int ht_insert(char * key, void * data, i9_hash_table t){
	int ht_index=0;
	_hash_table * ht = NULL;

	if(key==NULL || data==NULL || t==NULL){
#if ICE9_UTIL_DEBUG
		printf("ht_insert, key or data or t is NULL.\n");
#endif
		return 1;
	}
	ht = (_hash_table*)t;
	ht_index = SDBMHash(key) % ht->size;

	if(ht->array[ht_index]==NULL){
		ht->array[ht_index] = create_htlist();
	}
	en_htlist( create_htlist_elem(key, data), ht->array[ht_index] );

	return 0;
}

void * ht_search(char * key, i9_hash_table t){
	int ht_index=0;
	_hash_table * ht = NULL;
	_ht_list_elem * ht_le = NULL;
	//printf("%s\n", key);
	if(key==NULL || t==NULL){
#if ICE9_UTIL_DEBUG
		printf("ht_search, key or t is NULL.\n");
#endif
		return NULL;
	}
	ht = (_hash_table*)t;
	ht_index = SDBMHash(key) % ht->size;

	if(ht->array[ht_index]==NULL){
		//printf("ht_search, key not found1.\n");
		return NULL;
	}

	ht_le = ht->array[ht_index]->l_head;
	//printf("---\n");
	while(ht_le!=NULL){

		if( strcmp(key, ht_le->key)==0 ){
			return ht_le->data;
		}
		ht_le = ht_le->next;
	}

	return NULL;
}

int ht_delete(char * key, void * data, i9_hash_table t){
	_ht_list_elem * le = NULL;
	int ht_index=0;
	_hash_table * ht = NULL;
	_ht_list_elem * ht_le = NULL;

	if(key==NULL || data==NULL || t==NULL){
#if ICE9_UTIL_DEBUG
		printf("ht_delete, key or data or t is NULL.\n");
#endif
		return 1;
	}

	ht = (_hash_table*)t;
	ht_index = SDBMHash(key) % ht->size;
	if(ht->array[ht_index]==NULL)
		return 1;

	return del_htlist_elem(data, ht->array[ht_index]);
}

void ht_destruct(i9_hash_table h){
	_hash_table * ht = NULL;
	_ht_list_elem * le1 = NULL, * le2=NULL;
	int i=0, s=0;
	if(h==NULL)
		return;

	ht = (_hash_table*)h;
	s = ht->size;
	for(i=0;i<s;i++){
		if(ht->array[i]==NULL)
			continue;

		le1 = ht->array[i]->l_head;
		while (le1 != NULL) { /* iterate through an element of the ht, */
			le2 = le1;
			le1 = le1->next;

			le2->data = NULL;
			le2->key = NULL;
			le2->next = NULL;
			free(le2);  /* free list element(_symbol) */
		}
		free(ht->array[i]); /* free list itself */
		ht->array[i]=NULL;
	}

	free(ht);
}

void * ht_search_unmatched_fwd(i9_hash_table h){
	_ht_list_elem * le = NULL;
	int i=0;
	_hash_table * ht = NULL;
	struct _symbol_st * sy=NULL;

	if(h==NULL)
		return NULL;

	ht = (_hash_table *)h;
	for(i=0;i<ht->size;i++){
		if( ht->array[i]!=NULL ){
			le =  ht->array[i]->l_head;
			while(le!=NULL){
				sy = (struct _symbol_st*)le->data;
				if(sy->is_fwd==1 && sy->fwd_unmatched==1)
					return sy;
				le=le->next;
			}
		}
	}  /* end of for */
	return NULL;
}

/* for debug */
void print_ht(i9_hash_table t){
	int i=0, size=0;
	_hash_table * ht = NULL;
	_ht_list_elem * ht_le = NULL;

	if(t==NULL)
		return;

	ht = (_hash_table*)t;
	size = ht->size;

	for(i=0; i<size; i++){
		if(ht->array[i]==NULL)
			continue;
		printf("\n");
		ht_le = ht->array[i]->l_head;

		while(ht_le!=NULL){
			printf("[key:%s, data:%s]   ", ht_le->key, ht_le->data);
			ht_le = ht_le->next;
		}
	}
}
/* ----------end define hash-table functions---------- */



/* ----------start define stack functions---------- */
_stack_elem * stk_create_elem(void * data){
	_stack_elem * e = NULL;

	if(data==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_create_elem, data or stk is NULL.\n");
#endif
		return NULL;
	}

	e = (_stack_elem*)malloc(sizeof(_stack_elem));
	if(e==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_create_elem, out of memeory.\n");
#endif
		exit(1);
	}

	e->topper = NULL;
	e->lowwer = NULL;
	e->data=data;

	return e;
}

i9_stack stk_create(){
	_stack * stk = NULL;

	stk = (_stack*)malloc(sizeof(_stack));
	if(stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_create, out of memeory.\n");
#endif
		exit(1);
	}

	stk->top=NULL;
	stk->size=0;

	return (i9_stack)stk;
}

int stk_push(void * data, i9_stack stk){
	_stack * s =  NULL;
	_stack_elem * e = NULL;
	if(data==NULL || stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_push, data or stk is NULL.\n");
#endif
		exit(1);
	}

	s=(_stack*)stk;
	e = stk_create_elem(data);
	if(e==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_push, cannot create stk elem.\n");
#endif
		exit(1);
	}

	if(s->top!=NULL){
		e->lowwer = s->top;
		s->top->topper=e;
	}
	s->top=e;
	s->size++;

	return 0;
}

void * stk_pop(i9_stack stk){
	_stack * s =  NULL;
	_stack_elem * e = NULL;
	void * data=NULL;
	if(stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_pop, stk is NULL.\n");
#endif
		exit(1);
	}
	s=(_stack*)stk;
	if(s->size==0){
#if ICE9_UTIL_DEBUG
		printf("stk_pop, stk is empty.\n");
#endif
		return NULL;
	}

	e=s->top;
	data=e->data;
	if(s->size==1){
		s->top=NULL;

	}else{  /* the second elem on the top of stack becomes the top */
		s->top->lowwer->topper=NULL;
		s->top = s->top->lowwer;
	}
	stk_elem_destruct(e);
	s->size--;

	return data;
}

void * stk_peek(i9_stack stk){
	_stack * s =  NULL;
	if(stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_peek, stk is NULL.\n");
#endif
		exit(1);
	}
	s=(_stack*)stk;
	if(s->size==0){
#if ICE9_UTIL_DEBUG
		printf("stk_peek, stk is empty.\n");
#endif
		return NULL;
	}

	return s->top->data;
}

i9_stack_elem stk_peek_elem(i9_stack stk){
	_stack * s =  NULL;
	if(stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_peek_elem, stk is NULL.\n");
#endif
		exit(1);
	}
	s=(_stack*)stk;
	if(s->size==0){
#if ICE9_UTIL_DEBUG
		printf("stk_peek_elem, stk is empty.\n");
#endif
		return NULL;
	}

	return (i9_stack_elem)s->top;
}

i9_stack_elem stk_get_next_elem(i9_stack_elem e){
	_stack_elem * elem = NULL;

	if(e==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_get_next, e is NULL.\n");
#endif
		exit(1);
	}
	elem = (_stack_elem*)e;


	return (i9_stack_elem)elem->lowwer;
}

void * stk_get_elem_data(i9_stack_elem e){
	_stack_elem * elem = NULL;

	if(e==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_get_elem_data, e is NULL.\n");
#endif
		exit(1);
	}
	elem = (_stack_elem*)e;

	return elem->data;
}

int stk_getsize(i9_stack stk){
	_stack * s =  NULL;
	if(stk==NULL){
#if ICE9_UTIL_DEBUG
		printf("stk_getsize, stk is NULL.\n");
#endif
		exit(1);
	}
	s=(_stack*)stk;
	//printf("-------%d----\n", s->size);
	return s->size;
}

void stk_destruct(i9_stack s){
	_stack * stk = NULL;

	if(s==NULL)
		return;

	stk=(_stack*)s;
	stk->top=NULL;

	free(stk);
}

void stk_elem_destruct(i9_stack_elem se){
	_stack_elem * e = NULL;

	if(se==NULL)
		return;

	e = (_stack_elem *)se;

	e->data=NULL;
	e->lowwer=NULL;
	e->topper=NULL;
	free(e);  /*_scope*/

	return;
}
/* ----------end define stack functions---------- */



//testing hash functions
/*int main(int argc, char ** argv){
	i9_hash_table ht;

	ht = ht_create(HT_ARRAY_SIZE_S);
	ht_insert("abc", "1", ht);
	ht_insert("abcd", "2", ht);
	ht_insert("abc", "3", ht);
	ht_insert("abcde", "4", ht);
	ht_insert("abc", "5", ht);
	ht_insert("abcdef", "6", ht);
	ht_insert("abcd", "7", ht);
	ht_insert("abce", "8", ht);
	ht_insert("abc", "9", ht);
	//ht_destruct(ht);ht=NULL;
	print_ht(ht);
	printf("www\n\n");


	ht_delete("abcdef", ht_search("abcdef", ht), ht);
	ht_delete("abcdef", ht_search("abcdef", ht), ht);
	print_ht(ht);


	ht_insert("abcdef", "6", ht);
	printf("\n\n");
	print_ht(ht);


	ht_delete("abc", ht_search("abc", ht), ht);
	ht_delete("abc", ht_search("abc", ht), ht);
	printf("\n\n");
	print_ht(ht);


	ht_insert("abc", "10", ht);
	printf("\n\n");
	print_ht(ht);

}*/


//testing stack functions
/* int main(int argc, char ** argv){
	i9_stack stk = NULL;
	int size;
	i9_stack_elem ew = NULL;
	_stack_elem * e= NULL;

	stk = stk_create();

	stk_push("a", stk);
	stk_push("b", stk);
	stk_push("c", stk);
	stk_push("d", stk);
	//stk_destruct(stk);stk=NULL;
	size=stk_getsize(stk);
	while(stk_getsize(stk)!=0){
		printf(" %s ", (char*)stk_pop(stk));
	}

	stk_push("4", stk);
	stk_push("3", stk);
	stk_push("2", stk);
	stk_push("1", stk);

	e = (_stack_elem*)stk_peek_elem(stk);
	while(e!=NULL){
		printf(" %s ", (char*)e->data);
		e=e->lowwer;
	}

	ew = stk_peek_elem(stk);
	while(ew!=NULL){
		printf(" %s ", (char*) ((_stack_elem*)ew)->data);
		ew = stk_get_next(ew);
	}

} */

