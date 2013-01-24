#include "kvset.hpp"
#include "stl.hpp"
#include "stlset.hpp"
#include "stlmap.hpp"

/** @file
 * @brief provide ETS generic_interface backends using STL containers
 * @note this has to be compiled using a C++ compiler, but functions are to be used in C
 */

#define cmp_rel(A,A_BASE,B,B_BASE) cmp(A,B)
extern "C" int cmp(Eterm, Eterm);

bool eterm_compare(Eterm* key1, Eterm* key2) {
	return cmp(*key1, *key2) < 0;
}

class DbTermPairExtractor {
	public:
		DbTerm* operator()(std::pair<Eterm*, DbTerm*> p) {
			return p.second;
		}
};

class EtermExtractor {
	public:
		Eterm* operator()(DbTerm* term) {
			auto a = &term->tpl[1];
			return a;
		}
};

class EtermPacker {
	public:
		DbTerm* operator()(Eterm* term) {
			// NOTE: results in unsafe pointer AND doesn't work for keypos != 1
			DbTerm* a = reinterpret_cast<DbTerm*>(term)-1;
			return a;
		}
		typedef DbTerm* key_type;
};

class DbTermPairPacker {
	public:
		std::pair<Eterm*, DbTerm*> operator()(DbTerm* term) {
			return std::make_pair(&term->tpl[1], term);
		}
		typedef DbTerm* key_type;
};

bool dbterm_compare(DbTerm* key1, DbTerm* key2) {
	auto a = key1->tpl[1];
	auto b = key2->tpl[1];
	return cmp(a, b) < 0;
}


extern "C" KVSet* create_stlset() {
	typedef KVcompare<DbTerm*, dbterm_compare> C;
	typedef standard_functions<DbTerm*, C> S;
	auto ptr = make_kv_set<stlset, Eterm*, S, DbTerm*, EtermPacker>();
	return ptr;
}

extern "C" KVSet* create_stlmap() {
	typedef KVcompare<Eterm*, eterm_compare, EtermExtractor> KVC;
	typedef standard_functions<Eterm*, KVC> S;
	//typedef std::pair<Eterm, DbTerm*> KVPair;
	auto ptr = make_kv_set<stlmap, Eterm*, S, DbTerm*, NullPacker<Eterm*>, DbTermPairPacker, DbTermPairExtractor>();
	return ptr;
}
