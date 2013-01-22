#include "kvset.hpp"
#include "testmap.hpp"

/** @file
 * @brief provide C interface to testmap
 * @note this has to be compiled using a C++ compiler, but functions are to be used in C
 */

extern "C" {

#define cmp_rel(A,A_BASE,B,B_BASE) cmp(A,B)
int cmp(Eterm, Eterm);

int testcompare(DbTerm* key1, DbTerm* key2, void*) {
	auto a = key1->tpl[1];
	auto b = key2->tpl[1];
//	std::printf("a: %li - b: %li\n\r", a, b);
	return cmp(a, b);
/*    return cmp_rel(*key2,
                   key2,
                   *key1, 
                   key1);
		   */
}

KVSet* new_cppset_default() {
	typedef standard_functions<DbTerm*, void*, testcompare> S;
	auto ptr = make_kv_set<testmap, DbTerm*, S>();
	return ptr;
}

} /* extern "C" */
