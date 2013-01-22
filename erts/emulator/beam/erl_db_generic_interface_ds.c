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

enum gi_type get_gi_subtype(Eterm e) {
    char* name = atom_name(e);
    if(!strcmp(name, "skiplist")) return SKIPLIST;
    else if(!strcmp(name, "testmap")) return TESTMAP;
    else return ERROR_NO_TYPE;
}
