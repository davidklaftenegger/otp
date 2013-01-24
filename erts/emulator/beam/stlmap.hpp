#ifndef STLMAP_HPP
#define STLMAP_HPP

/**@file
 * @brief STL std::set as a backend for ETS
 */

#include<map>

#define nullptr std::make_pair<KeyType, ValueType>(nullptr,nullptr)

template<typename KeyType = long*, class StdFuns = standard_functions<KeyType>, typename ValueType = KeyType>
class stlmap {
	class Cmp {
		public: bool operator()(KeyType k1, KeyType k2) const {
			return StdFuns::compare(k1, k2);
		}
	};
	typedef std::map<KeyType, ValueType, Cmp> Map;
	Map m;
	typedef typename Map::value_type StoreType;

	public:
		stlmap() : m() {}
		StoreType put(StoreType key)     {
			auto old = m.find(key.first);
			StoreType oldval;
			if (old != m.end()) {
				oldval = *old;
			} else {
				oldval = nullptr;
			}
			m[key.first] = key.second;
			return oldval;
		}
		bool put_new(StoreType key)    {
			bool r = m.count(key.first);
			if(!r) m.insert(key);
			return !r;
		}
		StoreType remove(KeyType key)  {
			auto it = m.find(key);
			if(it != m.end()) {
				// TODO: return old val??
				m.erase(it);
			}
			return nullptr;
		}
		StoreType lookup(KeyType key) {
			auto it = m.find(key);
			if(it == m.end()) {
				return nullptr;
			}
			else {
				return *it;
			}
		}
		bool member(KeyType key) {
			return m.count(key);
		}
		StoreType first() {
			if(m.begin() != m.end())
				return *m.begin();
			else
				return nullptr;
		}
		StoreType last() {
			if(m.rbegin() != m.rend())
				return *m.rbegin();
			else
				return nullptr;
		}
		StoreType next(KeyType key) {
			auto it = m.lower_bound(key);
			if(it != m.end()) {
				it = std::next(it);
				if(it != m.end()) {
					return *it;
				}
			}
			return nullptr;
		}
		StoreType previous(KeyType key){
			auto it = m.lower_bound(key);
			if(it != m.end()) {
				it = std::prev(it);
				if(it != m.end()) {
					return *it;
				}
			}
			return nullptr;
		}
};

#undef nullptr

#endif // STLMAP_HPP
