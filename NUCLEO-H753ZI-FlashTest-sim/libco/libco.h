/*
  libco
  version: 0.17 (2015-06-18)
  author: byuu
  license: public domain
*/

#ifndef LIBCO_H
#define LIBCO_H

#ifdef LIBCO_C
  #ifdef LIBCO_MP
    #define thread_local __thread
  #else
    #define thread_local
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* cothread_t;

cothread_t co_active();
cothread_t co_create(unsigned int, void (*)(void));

// AP specific function, that accepts a pointer to a stack and its size
cothread_t co_create_ex(void *stack, unsigned int size, void (*entrypoint)(void));

void co_delete(cothread_t);
void co_switch(cothread_t);

#ifdef __cplusplus
}
#endif

/* ifndef LIBCO_H */
#endif
