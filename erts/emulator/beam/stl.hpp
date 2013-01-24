#ifndef STL_HPP
#define STL_HPP

typedef long Eterm;
typedef struct db_term {
    void* first_oh;
    unsigned int size;
#ifdef DEBUG_CLONE
    Eterm* debug_clone;
#endif
    Eterm tpl[1];
} DbTerm;

#endif // STL_HPP
