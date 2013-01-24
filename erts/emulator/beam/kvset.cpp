#include "kvset.hpp"

void destroy_kv_set(kv_set* s) {
	s->funs.delete_table(s, nullptr, nullptr);
}
