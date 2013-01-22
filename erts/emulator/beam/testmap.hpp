#ifndef TESTMAP_HPP
#define TESTMAP_HPP

/**@file
 * @brief a testing data structure for ETS, implemented in C++
 * 
 * Note this is just a proof of concept
 */

#include<set>

typedef long Eterm;
typedef struct db_term {
    void* first_oh;
    unsigned int size;
#ifdef DEBUG_CLONE
    Eterm* debug_clone;
#endif
    Eterm tpl[1];
} DbTerm;

/* this is a horribly way to make this work:
 * the pointer sometimes actually points to an Eterm instead of a DbTerm,
 * but the comparison function requires a DbTerm to do access the Eterm on.
 * To make this work, the pointer is adjusted so that the following pointer-arithmetic operation to access the Eterm will yield the key pointer again.
 * NOTE THIS WILL BREAK WITH KEYPOS != 1
 */
DbTerm* do_horrible_thing(DbTerm* key) {
	DbTerm* mkey = key-1;
	return mkey;
}

template<typename KeyType = long*, class StdFuns = standard_functions<KeyType>>
class testmap {
	class Cmp {
		public: bool operator()(KeyType k1, KeyType k2) const {
			return StdFuns::compare(k1, k2);
		}
	};

	std::set<KeyType, Cmp> m;
	public:
		testmap() : m() {}
		KeyType put(KeyType key)     {
			auto p = m.insert(key);
			if(!p.second) {
				auto r = *p.first;
				m.erase(p.first);
				m.insert(key);
				return r;
			}
			return nullptr;
		}
		bool put_new(KeyType key)    {
			bool r = m.count(key);
			if(!r) m.insert(key);
			return !r;
		}
		KeyType remove(KeyType key)  {
			auto mkey = do_horrible_thing(key);
			auto it = m.find(mkey);
			if(it != m.end()) {
				m.erase(it);
			}
			return nullptr;
		}
		KeyType lookup(KeyType key) {
			auto mkey = do_horrible_thing(key);
			auto it = m.find(mkey);
			if(it == m.end()) {
				return nullptr;
			}
			else {
				return *it;
			}
		}
		bool member(KeyType key) {
			auto mkey = do_horrible_thing(key);
			return m.count(mkey);
		}
		KeyType first() {
			if(m.begin() != m.end())
				return *m.begin();
			else
				return nullptr;
		}
		KeyType last() {
			if(m.rbegin() != m.rend())
				return *m.rbegin();
			else
				return nullptr;
		}
		KeyType next(KeyType key) {
			auto mkey = do_horrible_thing(key);
			auto it = m.lower_bound(mkey);
			if(it != m.end()) {
				it = std::next(it);
				if(it != m.end()) {
					return *it;
				}
			}
			return nullptr;
		}
		KeyType previous(KeyType key){
			auto mkey = do_horrible_thing(key);
			auto it = m.lower_bound(mkey);
			if(it != m.end()) {
				it = std::prev(it);
				if(it != m.end()) {
					return *it;
				}
			}
			return nullptr;
		}
};


#endif // TESTMAP_HPP
