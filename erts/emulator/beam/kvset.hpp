#ifndef KVSET_HPP
#define KVSET_HPP KVSET_HPP

#include <cstdlib>
#include "kvset.h"

/**
 * @internal
 * @brief stub comparison function
 * @param K key type
 * @param A ignored
 * @param a first key
 * @param b second key
 * @warning as keys are supposed to be pointers this will always return garbage
 */
template<typename K, typename  A>
int default_compare(K a, K b, A) { return b - a; }

template<typename T>
class Null {
	public:
		T operator()(T t) {
			return t;
		}
		typedef T key_type;
};
template<typename T> using NullExtractor = Null<T>;
template<typename T> using NullPacker = Null<T>;

/**
 * @internal
 * @brief type disambiguation for specialization below
 */
template<typename T1, typename T2>
class comparison_types {
	typedef T1 type1;
	typedef T2 type2;
};

/**
 * @brief a comparator object
 * @param KeyType the Type on which comparison works
 * @param S a function bundle that specifies the comparision on KeyType
 * @param Extractor a functor that transforms compatible types into KeyType
 */
template<
	typename KeyType,
	bool (*Compare)(KeyType, KeyType),
	class Extractor = NullExtractor<KeyType>
>
class KVcompare {
	public:
		/**
		 * @brief comparison operator()
		 * 
		 * This uses one of the four implementations below,
		 * relying on Extractor as needed
		 */
		template<typename T1, typename T2>
		bool operator()(T1 t1, T2 t2) const {
			return compare(t1, t2, comparison_types<T1, T2>());
		}
	private:
		bool compare(KeyType k1, KeyType k2, comparison_types<KeyType, KeyType>) const {
			return Compare(k1, k2);
		}
/*		template<typename T>
		bool compare(T v1, KeyType k2, comparison_types<T, KeyType>) const {
			return Compare(Extractor(v1), k2);
		}
		template<typename T>
		bool compare(KeyType k1, T v2, comparison_types<KeyType, T>) const {
			return Compare(k1, Extractor(v2));
		}
		template<typename T1, typename T2>
		bool compare(T1 v1, T2 v2, comparison_types<T1, T2>) const {
			return Compare(Extractor(v1), Extractor(v2));
		}
*/
};


/**
 * @internal
 * @brief coding shorthand for compare, allocation and freeing functions
 * @param KeyType type of key for comparison
 * @param Compare comparison function
 * @param Malloc memory allocation function
 * @param Free memory freeing function
 */
template <
	typename KeyType,
	typename Comparator = KVcompare<KeyType, default_compare<KeyType, void*>>,
	void* (*Malloc)(size_t) = std::malloc,
	void (*Free)(void*) = std::free
>
struct standard_functions {
	/** @brief comparision function wrapper */
	static bool compare(KeyType a, KeyType b) {
		return (Comparator()(a, b));
	}
	
	/** @brief freeing function wrapper */
	static void free(void* data) { Free(data); }

	/** @brief memory allocation function wrapper */
	static void* alloc(size_t size) { return Malloc(size); }
};

/**
 * @internal
 * @brief typesafe wrapper if kv_set
 * @param T real type of the type specific data (void* in C version)
 * @see kv_set
 */
template<class T>
struct kv_set_t {
	KVSetFunctions funs;
	unsigned int key_offset;
	T type_specific_data;
};

/**
 * @internal
 * @brief wrapper to C interface
 * @param Instance a kv_set_instance, target of delegation
 * 
 * This class specifies functions in accordance with what the C interface expects,
 * but delegates all calls to the member functions of kv_set_instance.
 * This is required so that normal C++ coding can be used transparently to the C interface.
 */
template <typename Instance>
class kv_set_classfuns {
	typedef typename Instance::key_type KeyType;
	typedef typename Instance::value_type ValueType;
	typedef typename Instance::instance_type Obj;
	public:
		static void* put(kv_set* s, void* key) {
			auto result = reinterpret_cast<kv_set_t<Obj>*>(s)->
				type_specific_data.
				put(
					reinterpret_cast<ValueType>(key)
				);
			return reinterpret_cast<void*>(result);
		}
		static int put_new(kv_set* s, void* key) {
			auto result = reinterpret_cast<kv_set_t<Obj>*>(s)->
				type_specific_data.
				put_new(
					reinterpret_cast<ValueType>(key)
				);
			return result;
		}
		static void* remove(kv_set* s, void* key) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.remove(reinterpret_cast<KeyType>(key)));
		}
		static void* lookup(kv_set* s, void* key) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.lookup(reinterpret_cast<KeyType>(key)));
		}
		static int member(kv_set* s, void* key) {
			return reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.member(reinterpret_cast<KeyType>(key));
		}
		static void* first(kv_set* s) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.first());
		}
		static void* last(kv_set* s) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.last());
		}
		static void* next(kv_set* s, void* key) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.next(reinterpret_cast<KeyType>(key)));
		}
		static void* previous(kv_set* s, void* key) {
			return reinterpret_cast<void*>(reinterpret_cast<kv_set_t<Obj>*>(s)->type_specific_data.previous(reinterpret_cast<KeyType>(key)));
		}
};


/**
 * @internal
 * @brief convenience wrapper to datastructure implementations
 * @param DataType the mapping datastructure in use
 * @param KeyType key for the DataType
 * @param StdFuns comparison, allocation, and freeing functions
 *
 * This wrapper simply adds a few convenience typedefs to the DataType implementation.
 */
template <
	template<typename, class, typename> class DataType,
	typename KeyType,
	class StdFuns = standard_functions<KeyType>,
	typename ValueType = KeyType,
	class KPacker = NullPacker<KeyType>,
	class VPacker = NullPacker<ValueType>,
	class VExtractor = NullExtractor<ValueType>
>
class kv_set_instance {
	typedef DataType<typename KPacker::key_type, StdFuns, ValueType> Impl;
	public:
		kv_set_instance() : inst() {}
		ValueType put(ValueType key) {
			return VExtractor()(inst.put(VPacker()(key)));
		}
		bool put_new(ValueType key) {
			return inst.put_new(VPacker()(key));
		}
		ValueType remove(KeyType key) {
			return VExtractor()(inst.remove(KPacker()(key)));
		}
		ValueType lookup(KeyType key) {
			return VExtractor()(inst.lookup(KPacker()(key)));
		}
		bool member(KeyType key)       { return inst.member(KPacker()(key)); }
		ValueType first() {
			return VExtractor()(inst.first());
		}
		ValueType last() {
			return VExtractor()(inst.last());
		}
		ValueType next(KeyType key) {
			// TODO check Key/Value input parameter
			return VExtractor()(inst.next(KPacker()(key)));
		}
		ValueType previous(KeyType key) {
			// TODO see line above.
			return VExtractor()(inst.previous(KPacker()(key)));
		}
		
		typedef kv_set_classfuns<kv_set_instance<DataType, KeyType, StdFuns, ValueType, KPacker, VPacker, VExtractor>> classfuns;
		typedef StdFuns stdfuns;
		typedef KeyType key_type;
		typedef ValueType value_type;
		typedef kv_set_instance<DataType, KeyType, StdFuns, ValueType, KPacker, VPacker, VExtractor> instance_type;
	private:
		Impl inst;
};

/**
 * @internal
 * @brief generic cleanup code
 * @param set the table to delete
 * @param f ignored (for now)
 * @param context ignored 
 * @todo delete elements in the table before deleting table
 *
 * Deletes a mapping datastructure and frees any allocated memory.
 * This cannot be a member function of the structure to be freed,
 * so it is implemented as a separate template function.
 */
template <
	template<typename, class, typename> class Impl,
	typename K,
	class S,
	typename V = K,
	class KPacker = NullPacker<K>,
	class VPacker = NullPacker<V>,
	class VExtractor = NullExtractor<V>
>
void delete_table(kv_set* set, void (*f)(void* context, void* element), void* context) {
	typedef kv_set_instance<Impl, K, S, V, KPacker, VPacker, VExtractor> FS;
	//TODO delete elements in table here
	reinterpret_cast<kv_set_t<FS>*>(set)->type_specific_data.~FS();
	FS::stdfuns::free(set);
}


/**
 * @brief create a kv_set (usable in C) from a C++ datastructure
 * @param Impl the C++ datastructure
 * @param K the mapping key
 * @param S comparison, memory allocation and freeing functions
 * @return a kv_set wrapping the specified C++ datastructure
 *
 * Use this in the form
 * auto kvs = make_kv_set<MyDataStructure>();
 * to create a kv_set of MyDataStructure.
 * Remember that you have to collect the allocated memory using
 * destroy_kv_set(kvs);
 */
template <template<typename, class, typename> class Impl, typename K = long*, class S = standard_functions<K>, typename V = K, class KPacker = NullPacker<K>, class VPacker = NullPacker<V>, class VExtractor = NullExtractor<V>>
kv_set* make_kv_set() {
	typedef kv_set_instance<Impl, K, S, V, KPacker, VPacker, VExtractor> FS;
	void* memory = FS::stdfuns::alloc(sizeof(KVSet) + sizeof(FS));
	kv_set* r = static_cast<kv_set*>(memory);
	new (&r->type_specific_data) FS;
	r->funs.delete_table = &delete_table<Impl, K, S, V, KPacker, VPacker, VExtractor>;
	r->funs.put = &FS::classfuns::put;
	r->funs.put_new = &FS::classfuns::put_new;
	r->funs.remove = &FS::classfuns::remove;
	r->funs.lookup = &FS::classfuns::lookup;
	r->funs.member = &FS::classfuns::member;
	r->funs.first = &FS::classfuns::first;
	r->funs.last = &FS::classfuns::last;
	r->funs.next = &FS::classfuns::next;
	r->funs.previous = &FS::classfuns::previous;
	
	return r;
}

/**
 * @brief deallocate a kv_set constructed around a C++ datastructure
 * @param s the kv_set to destroy
 */
void destroy_kv_set(kv_set* s);

#endif
