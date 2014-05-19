/*
 * Author: Wenzhao Zhang;
 * UnityID: wzhang27;
 *
 * CSC512 P3
 *
 * Header for util.
 */

#ifndef ICE9_UTIL_H_
#define ICE9_UTIL_H_

#define ICE9_UTIL_DEBUG 0

#define HT_ARRAY_SIZE_T 10		/* tiny */
#define HT_ARRAY_SIZE_S 100		/* small */
#define HT_ARRAY_SIZE_M 1000	/* medium */
#define HT_ARRAY_SIZE_L 10000	/* large */

typedef void * i9_hash_table;  /* only for storing symbols */
typedef void * i9_stack;
typedef void * i9_stack_elem;

/* size: internal array size; tiny, small, medium, large */
i9_hash_table ht_create(int size);

/* return: 0 success, 1 error; */
int ht_insert(char * key, void * data, i9_hash_table ht);

/* return: found data's pointer: success; NULL: error; */
void * ht_search(char * key, i9_hash_table ht);

/* return: 0 success, 1 error; */
int ht_delete(char * key, void * data, i9_hash_table ht);

void ht_destruct(i9_hash_table ht);

/* search unmatched forward stmt, seems a little strange, too much coupled with ast struct */
void * ht_search_unmatched_fwd(i9_hash_table ht);

i9_stack stk_create();

int stk_push(void * data, i9_stack stk);

/* get acutal data */
void * stk_pop(i9_stack stk);
void * stk_peek(i9_stack stk);

/* get stack element, which is a wrapper of the actual data */
i9_stack_elem stk_peek_elem(i9_stack stk);
i9_stack_elem stk_get_next_elem(i9_stack_elem e);
void * stk_get_elem_data(i9_stack_elem e);  /* get actual data */

int stk_getsize(i9_stack stk);

void stk_destruct(i9_stack s);
void stk_elem_destruct(i9_stack_elem se);

#endif
