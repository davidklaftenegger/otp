#ifndef _CPP_DS_H
#define _CPP_DS_H

#include "kvset.h"

enum gi_type {
    SKIPLIST,
    TESTMAP,
    STLSET,
    STLMAP,
    ERROR_NO_TYPE
};

enum gi_type get_gi_subtype(Eterm e);

struct db_table_generic_interface;
KVSet* gi_create(struct db_table_generic_interface* tbl);

/* prototypes for the construction functions used in gi_create implementation */
#include "skiplist.h"
KVSet* new_cppset_default(void);
KVSet* create_stlset(void);
KVSet* create_stlmap(void);

/* commonly used by C-implemented datastructures */
int compare(Eterm * element, Eterm * key);
void * generic_interface_malloc(size_t size);
void generic_interface_free(void *);


#endif /* _CPP_DS_H */
