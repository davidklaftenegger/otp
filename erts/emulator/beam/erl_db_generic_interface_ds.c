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
    else return ERROR_NO_TYPE;
}

/* add construction code for new gi datatypes here */
KVSet* gi_create(DbTableGenericInterface* tbl) {
    KVSet* ds;
    struct gi_options_list* options_list = tbl->options;

    switch(tbl->type) {
	case SKIPLIST:
	   printf("THIS IS A SKIPLIST\n\r"); 
	   ds = new_skiplist((int (*)(void *, void *))compare,
			     free, 
			     malloc, 
			     sizeof(DbTerm) - sizeof(Eterm) + sizeof(Eterm) * tbl->common.keypos);

	    break;
	case TESTMAP:
	   printf("THIS IS NOT A SKIPLIST, tbl keypos: %d\n\r", tbl->common.keypos); 
	    while(options_list) {
		printf("option is %s\n\r", options_list->option.first.name);
		options_list = options_list->next;
	    }
	    ds = new_cppset_default();

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


