#ifndef _DB_SUBTABLE_HASH_H
#define _DB_SUBTABLE_HASH_H

#include "erl_db_util.h" /* DbTerm & DbTableCommon */
#include "erl_db_generic_interface_ds.h" /* gi_type & gi_options_list */
#include "kvset.h"

#define KJELL_DEBUG 1

#ifdef KJELL_DEBUG
 #define D if(1) 
#else
 #define D if(0) 
#endif

char* atom_name(Eterm e);
void db_initialize_generic_interface(void);

enum gi_option_type {
    SETTING,
    ATOM,
    INTEGER
};

union gi_option_value {
    char * name;
    long number;
};

struct gi_option {
    enum gi_option_type type;
    union gi_option_value first;
    union gi_option_value second;
};

struct gi_options_list;
struct gi_options_list {
    struct gi_option option;
    struct gi_options_list* next;
};

typedef struct db_table_generic_interface {
    DbTableCommon common;

    /* allow differentiating different implementations in construction code */
    enum gi_type type;
    /* allow passing of additional parameters to construction code */
    struct gi_options_list* options;
    
    KVSet* kvset;
} DbTableGenericInterface;


#endif


