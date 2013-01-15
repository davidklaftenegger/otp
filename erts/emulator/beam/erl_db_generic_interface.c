
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_process.h"
#include "error.h"
#define ERTS_WANT_DB_INTERNAL__
#include "erl_db.h"
#include "bif.h"
#include "big.h"
#include "export.h"
#include "erl_binary.h"
#include "skiplist.h"


#include "erl_debug.h"


//Public interface
int db_create_generic_interface(Process *p, DbTable* tb);

int db_first_generic_interface(Process* p, 
                           DbTable* tb, /* [in out] */ 
                           Eterm* ret   /* [out] */);
int db_next_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm key,   /* [in] */
                          Eterm* ret /* [out] */);
int db_last_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm* ret   /* [out] */);
int db_prev_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm key, 
                          Eterm* ret);
int db_put_generic_interface(DbTable* tb, /* [in out] */ 
                         Eterm obj,
                         int key_clash_fail); /* DB_ERROR_BADKEY if key exists */ 
int db_get_generic_interface(Process* p, 
                         DbTable* tb, /* [in out] */ 
                         Eterm key, 
                         Eterm* ret);
int db_get_element_generic_interface(Process* p, 
                                 DbTable* tb, /* [in out] */ 
                                 Eterm key, 
                                 int index, 
                                 Eterm* ret);
int db_member_generic_interface(DbTable* tb, /* [in out] */ 
                            Eterm key, 
                            Eterm* ret);
int db_erase_generic_interface(DbTable* tb,  /* [in out] */ 
                           Eterm key, 
                           Eterm* ret);
int db_erase_object_generic_interface(DbTable* tb, /* [in out] */ 
                                  Eterm obj,
                                  Eterm* ret);
int db_slot_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */ 
                          Eterm slot, 
                          Eterm* ret);
int db_select_chunk_generic_interface(Process* p, 
                                  DbTable* tb, /* [in out] */ 
                                  Eterm pattern,
                                  Sint chunk_size,
                                  int reverse,
                                  Eterm* ret);
int db_select_generic_interface(Process* p, 
                            DbTable* tb, /* [in out] */ 
                            Eterm pattern,
                            int reverse,
                            Eterm* ret);
int db_select_delete_generic_interface(Process* p, 
                                   DbTable* tb, /* [in out] */ 
                                   Eterm pattern,
                                   Eterm* ret);
int db_select_continue_generic_interface(Process* p, 
                                     DbTable* tb, /* [in out] */ 
                                     Eterm continuation,
                                     Eterm* ret);
int db_select_delete_continue_generic_interface(Process* p, 
                                            DbTable* tb, /* [in out] */ 
                                            Eterm continuation,
                                            Eterm* ret);
int db_select_count_generic_interface(Process* p, 
                                  DbTable* tb, /* [in out] */ 
                                  Eterm pattern, 
                                  Eterm* ret);
int db_select_count_continue_generic_interface(Process* p, 
                                           DbTable* tb, /* [in out] */ 
                                           Eterm continuation, 
                                           Eterm* ret);

int db_delete_all_objects_generic_interface(Process* p,
                                        DbTable* db /* [in out] */ );

int db_free_table_generic_interface(DbTable* db /* [in out] */ );
int db_free_table_continue_generic_interface(DbTable* db); /* [in out] */  
    
void db_print_generic_interface(int to, 
                            void* to_arg, 
                            int show, 
                            DbTable* tb /* [in out] */ );

void db_foreach_offheap_generic_interface(DbTable* db,  /* [in out] */ 
                                      void (*func)(ErlOffHeap *, void *),
                                      void *arg);
void db_check_table_generic_interface(DbTable* tb);

/* Lookup a dbterm for updating. Return false if not found.
 */
int db_lookup_dbterm_generic_interface(DbTable*, Eterm key, 
                                   DbUpdateHandle* handle); /* [out] */

/* Must be called for each db_lookup_dbterm that returned true,
** even if dbterm was not updated.
*/
void db_finalize_dbterm_generic_interface(DbUpdateHandle* handle);


DbTableMethod db_hash;
/*
** External interface 
*/
DbTableMethod db_generic_interface =
    {
        db_create_generic_interface,
        db_first_generic_interface,
        db_next_generic_interface,
        db_first_generic_interface,
        db_next_generic_interface,
        db_put_generic_interface,
        db_get_generic_interface,
        db_get_element_generic_interface,
        db_member_generic_interface,
        db_erase_generic_interface,
        db_erase_object_generic_interface,
        db_slot_generic_interface,
        db_select_chunk_generic_interface,
        db_select_generic_interface,
        db_select_delete_generic_interface,
        db_select_continue_generic_interface,
        db_select_delete_continue_generic_interface,
        db_select_count_generic_interface,
        db_select_count_continue_generic_interface,
        db_delete_all_objects_generic_interface,
        db_free_table_generic_interface,
        db_free_table_continue_generic_interface,
        db_print_generic_interface,
        db_foreach_offheap_generic_interface,
#ifdef HARDDEBUG
        db_check_table_generic_interface,
#else
        NULL,
#endif
        db_lookup_dbterm_generic_interface,
        db_finalize_dbterm_generic_interface
    };

//Internal functions prototypes
int compare(Eterm * element, Eterm * key);


//Function declarations
void db_initialize_generic_interface(){

}


static ERTS_INLINE void free_term(DbTable *tb, DbTerm* p)
{
    db_free_term(tb, p, 0);
}

static ERTS_INLINE DbTerm * new_dbterm(DbTable *tb, Eterm obj)
{
    DbTerm * p;
    if (tb->common.compress) {
	p = db_store_term_comp(&tb->common, NULL, 0, obj);
    }
    else {
	p = db_store_term(&tb->common, NULL, 0, obj);
    }
    return p;
}

int compare(Eterm * key1, Eterm * key2)
{
    return cmp_rel(*key2,
                   key2,
                   *key1, 
                   key1);
}


int db_create_generic_interface(Process *p, DbTable *tbl)
{
    DbTableGenericInterface *tb = &tbl->generic_interface;
    KVSet* ds;
    struct gi_options_list* ol_ptr;

    switch(tb->type) {
	case SKIPLIST:
	   printf("THIS IS A SKIPLIST\n"); 
	    ds = 
		new_skiplist((int (*)(void *, void *))compare,
			     free, 
			     malloc, 
			     sizeof(DbTerm) - sizeof(Eterm) + sizeof(Eterm) * tbl->common.keypos);

	    break;
	case TESTMAP:
	    ol_ptr = tb->options;
	    while(ol_ptr) {
		printf("\noption is %s\n", ol_ptr->option.first.name);
		ol_ptr = ol_ptr->next;
	    }
	    ds = new_cppset_default();

	    break;
	case ERROR_NO_TYPE:
	default:
	    /* this should never happen */
	    printf("eRROR ErROR ERrOR ERRoR ERROr\nExpect a segfault next.\n"); 
	    ds = NULL;
    }
    tb->kvset = ds;
    
    /* free the gi_options linked list again */
    while(tb->options) {
	struct gi_options_list* delme = tb->options;
	tb->options = tb->options->next;
	switch(delme->option.type) {
	    case ATOM:
		free(delme->option.second.name);
	    case SETTING:
	    case INTEGER:
		free(delme->option.first.name);
		break;
	}
	free(delme);
    }

    if(ds) return DB_ERROR_NONE;
    else return DB_ERROR_UNSPEC;
}

int db_first_generic_interface(Process* p, 
                           DbTable* tb, /* [in out] */ 
                           Eterm* ret   /* [out] */)
{
    D printf("CALLING db_first_generic_interface \n");
    return DB_ERROR_NONE;
}

int db_next_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm key,   /* [in] */
                          Eterm* ret /* [out] */)
{
    D printf("CALLING db_next_generic_interface \n");
    return DB_ERROR_NONE;
}


int db_last_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm* ret   /* [out] */)
{
    D printf("CALLING db_last_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_prev_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */
                          Eterm key, 
                          Eterm* ret)
{
    D printf("CALLING db_prev_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_put_generic_interface(DbTable* tbl, /* [in out] */ 
                         Eterm obj,
                         int key_clash_fail)
{

    void * element_to_put = new_dbterm(tbl, obj);

    KVSet *tb = tbl->generic_interface.kvset;

    void * element_to_free = tb->funs.put(tb, element_to_put);
    
    if(NULL != element_to_free) {
        free_term(tbl, element_to_free);
    }

    return DB_ERROR_NONE;
}
int db_get_generic_interface(Process* p, 
                         DbTable* tbl, /* [in out] */ 
                         Eterm key, 
                         Eterm* ret)
{
    Eterm copy;
    Eterm *hp, *hend;

    KVSet *tb = tbl->generic_interface.kvset;

    DbTerm * dbterm = (DbTerm *)tb->funs.lookup(tb, &key);

    if(NULL == dbterm){
        *ret = NIL;
    } else {
	hp = HAlloc(p, dbterm->size + 2);
	hend = hp + dbterm->size + 2;
	copy = db_copy_object_from_ets(&tbl->common, dbterm, &hp, &MSO(p));
	*ret = CONS(hp, copy, NIL);
	hp += 2;
	HRelease(p,hend,hp);
    }

    return DB_ERROR_NONE;

}
int db_get_element_generic_interface(Process* p, 
                                 DbTable* tb, /* [in out] */ 
                                 Eterm key, 
                                 int index, 
                                 Eterm* ret)
{
    D printf("CALLING db_get_element_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_member_generic_interface(DbTable* tb, /* [in out] */ 
                            Eterm key, 
                            Eterm* ret)
{
    D printf("CALLING db_member_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_erase_generic_interface(DbTable* tbl,  /* [in out] */ 
                           Eterm key, 
                           Eterm* ret)
{
    KVSet *tb = tbl->generic_interface.kvset;
    DbTerm *result = (DbTerm *)tb->funs.remove(tb, &key);

    *ret = am_true;

    if (NULL != result) {
	free_term(tbl, result);
    }

    return DB_ERROR_NONE;
}
int db_erase_object_generic_interface(DbTable* tb, /* [in out] */ 
                                  Eterm obj,
                                  Eterm* ret)
{
    D printf("CALLING db_erase_object_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_slot_generic_interface(Process* p, 
                          DbTable* tb, /* [in out] */ 
                          Eterm slot, 
                          Eterm* ret)
{
    D printf("CALLING db_slot_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_chunk_generic_interface(Process* p, 
                                  DbTable* tb, /* [in out] */ 
                                  Eterm pattern,
                                  Sint chunk_size,
                                  int reverse,
                                  Eterm* ret)
{
    D printf("CALLING db_select_chunk_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_generic_interface(Process* p, 
                            DbTable* tb, /* [in out] */ 
                            Eterm pattern,
                            int reverse,
                            Eterm* ret)
{
    D printf("CALLING db_select_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_delete_generic_interface(Process* p, 
                                   DbTable* tb, /* [in out] */ 
                                   Eterm pattern,
                                   Eterm* ret)
{
    D printf("CALLING db_select_delete_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_continue_generic_interface(Process* p, 
                                     DbTable* tb, /* [in out] */ 
                                     Eterm continuation,
                                     Eterm* ret)
{
    D printf("CALLING db_select_continue_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_delete_continue_generic_interface(Process* p, 
                                            DbTable* tb, /* [in out] */ 
                                            Eterm continuation,
                                            Eterm* ret)
{
    D printf("CALLING db_select_delete_continue_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_count_generic_interface(Process* p, 
                                  DbTable* tb, /* [in out] */ 
                                  Eterm pattern, 
                                  Eterm* ret)
{
    D printf("CALLING db_select_count_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_select_count_continue_generic_interface(Process* p, 
                                           DbTable* tb, /* [in out] */ 
                                           Eterm continuation, 
                                           Eterm* ret)
{
    D printf("CALLING db_select_count_continue_generic_interface \n");
    return DB_ERROR_NONE;
}

int db_delete_all_objects_generic_interface(Process* p,
                                        DbTable* db /* [in out] */ )
{
    D printf("CALLING db_delete_all_objects_generic_interface \n");
    return DB_ERROR_NONE;
}

int db_free_table_generic_interface(DbTable* db /* [in out] */ )
{
    D printf("CALLING db_free_table_generic_interface \n");
    return DB_ERROR_NONE;
}
int db_free_table_continue_generic_interface(DbTable* tbl)
{
    KVSet *tb = tbl->generic_interface.kvset;

    tb->funs.delete_table(tb, (void (*)(void *, void* ))free_term, (void*)tbl);

    return 1;
}
    
void db_print_generic_interface(int to, 
                            void* to_arg, 
                            int show, 
                            DbTable* tb /* [in out] */ )
{
        D printf("CALLING db_print_generic_interface \n");
}

void db_foreach_offheap_generic_interface(DbTable* db,  /* [in out] */ 
                                      void (*func)(ErlOffHeap *, void *),
                                      void *arg)
{
    D printf("CALLING db_foreach_offheap_generic_interface \n");

}

void db_check_table_generic_interface(DbTable* tb)
{
    D printf("CALLING db_check_table_generic_interface \n");

}

/* Lookup a dbterm for updating. Return false if not found.
 */
int db_lookup_dbterm_generic_interface(DbTable* tbl, 
                                       Eterm key, 
                                       DbUpdateHandle* handle)
{
    D printf("CALLING db_lookup_dbterm_generic_interface \n");
    return DB_ERROR_NONE;
}

/* Must be called for each db_lookup_dbterm that returned true,
** even if dbterm was not updated.
*/
void db_finalize_dbterm_generic_interface(DbUpdateHandle* handle)
{
    D printf("CALLING db_finalize_dbterm_generic_interface \n");

}

/* helper for getting atom name out of an Eterm */
char* atom_name(Eterm e) {
    Atom* a = atom_tab(atom_val(e));
    char* res = malloc(a->len+1);
    strncpy(res, (char*)a->name, a->len);
    res[a->len] = '\0';
    return res;
}

enum gi_type get_gi_subtype(Eterm e) {
    char* name = atom_name(e);
    if(!strcmp(name, "skiplist")) return SKIPLIST;
    else if(!strcmp(name, "testmap")) return TESTMAP;
    else return ERROR_NO_TYPE;
}
