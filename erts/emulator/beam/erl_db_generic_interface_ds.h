#ifndef _CPP_DS_H
#define _CPP_DS_H

#include "kvset.h"

#include "skiplist.h"


enum gi_type {
    SKIPLIST,
    TESTMAP,
    ERROR_NO_TYPE
};

enum gi_type get_gi_subtype(Eterm e);

KVSet* new_cppset_default(void);

#endif /* _CPP_DS_H */
