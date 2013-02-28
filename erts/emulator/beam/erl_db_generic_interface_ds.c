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


#include "erl_db_generic_interface.h"
#include "erl_db_generic_interface_ds.h"


/* add mapping for atom (string) to typefield in gi_type */
enum gi_type get_gi_subtype(Eterm e) {
    char* name = atom_name(e);
    if(!strcmp(name, "skiplist")) return SKIPLIST;
    else if(!strcmp(name, "testmap")) return TESTMAP;
    else if(!strcmp(name, "stlset")) return STLSET;
    else if(!strcmp(name, "stlmap")) return STLMAP;
    else if(!strcmp(name, "stlhashset")) return STLUNORDERED_SET;
    else if(!strcmp(name, "btreeset")) return BTREESET;
    else if(!strcmp(name, "btreeset4")) return BTREESET4;
    else if(!strcmp(name, "null")) return NULL_STORAGE;
    else return ERROR_NO_TYPE;
}

/* add construction code for new gi datatypes here */
KVSet* gi_create(DbTableGenericInterface* tbl) {
    KVSet* ds;
    struct gi_options_list* options_list = tbl->options;

    switch(tbl->type) {
	case SKIPLIST:
	    ds = new_skiplist((int (*)(void *, void *))compare,
                     generic_interface_free, 
                     generic_interface_malloc, 
                     sizeof(DbTerm) - sizeof(Eterm) + sizeof(Eterm) * tbl->common.keypos);
	    
	    break;
	case TESTMAP:
	    while(options_list) {
		printf("option is %s\n\r", options_list->option.first.name);
		options_list = options_list->next;
	    }
	    ds = new_cppset_default();

	    break;
	case STLSET:
	    ds = create_stlset();
	    break;
	case STLMAP:
	    ds = create_stlmap();
	    break;
	case STLUNORDERED_SET:
	    ds = create_stlunordered_set();
	    break;
	case BTREESET:
	    ds = create_btreeset();
	    break;
	case BTREESET4:
	    ds = create_btreeset4();
	    break;
	case NULL_STORAGE:
	    ds = create_null();
	    break;
	case ERROR_NO_TYPE:
	default:
	    /* this should never happen */
	    printf("eRROR ErROR ERrOR ERRoR ERROr\n\rExpect a segfault next.\n\r"); 
	    ds = NULL;
    }

    return ds;
}

int compare(Eterm * key1, Eterm * key2)
{
    return cmp_rel(*key2,
                   key2,
                   *key1, 
                   key1);
}

void * generic_interface_malloc(size_t size){
    return erts_alloc(ERTS_ALC_T_DB_TERM, size);
}

void generic_interface_free(void * data){
    erts_free(ERTS_ALC_T_DB_TERM, data);
}

