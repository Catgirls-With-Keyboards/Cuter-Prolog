#include <stdlib.h>
#include <stdio.h>

#include "prolog_config.h"

/*
 * # USING THIS LIBRARY:
 *
 * First, create a `PrologDatabase` with `prolog_database_init()`.
 * Then, add facts (`Term`s) to it with `prolog_addFact()`.
 * To create `Term`s, call `prolog_newAtom()`, `prolog_newVariable()`,
 * and `prolog_newCompund()` using &(database->arena).
 *
 * Once your database is built, create a `PrologGoal` with
 * `prolog_goal_init()`. Now add to your query the same way you added to your
 * database, using the same functions you used to construct `Term`s. Pass
 * `&(query->arena)` as the first argument. Then add what you want to prove to
 * the query with `prolog_addToQuery()`.
 *
 * Once you're ready, call `prolog_resolve()`. This will attempt to prove the
 * theorem by unifying the query against facts in the database.
 *
 * You can also call `prolog_unify()`. It will return if both terms can be
 * true, and which variables need to be bound to what for that to happen.
 * If all you need to do is unify some terms, then `prolog_unify()` does
 * not actually require that you create a database.
 *
 * Finally, clean up the memory. The `PrologArena`s inside `PrologDatabase`
 * and `PrologGoal` are fixed size, and are allocated by the user. You are
 * the one who knows how to clean them up. do not need to be cleaned up. They
 * hold all the memory used by the `prolog_newX()` functions used to create
 * `Term`s, and so those don't need to be cleaned up either. They're cleaned
 * up implicitly when the database or goal you created is freed. However, the
 * results of `prolog_unify()` and `prolog_resolve()` do allocate memory to
 * store their result lists. Destroy them with `prolog_unificationDestroy()`
 * and `prolog_resolutionDestroy()` respectively.
 *
 * Memory (specifically bound variables) inside the database and query are
 * modified by `prolog_resolve()`, and `prolog_unify()` modifies the arenas
 * that back each `Term` attempting to unify. Therefore `PrologResolution` and
 * `PrologUnification` that you receive back respectively contain pointers
 * into the `PrologQuery` and `PrologDatabase`. This means that you should not
 * run `prolog_resolve()` or `prolog_unify()` on the same query or same
 * database across multiple threads, and you should not destroy either until
 * you're done with the result. You can however choose to give each thread
 * its own query and its own copy of the database.
 */

DECL(Term);
STRUCT(Variable) {
  char* name;
  Term* bound;
};
STRUCT(Atom) {
  char* name;
};
STRUCT(Compound) {
  char* name;
  size_t arity;
  Term** components;
};
STRUCT(Term) {
  union {
    Atom a;
    Variable v;
    Compound c;
  };
#define PROLOG_KIND_ATOM     1
#define PROLOG_KIND_VARIABLE 2
#define PROLOG_KIND_COMPOUND 3
  unsigned char kind;
};

STRUCT(PrologArena) {
  char buf[PROLOG_ARENA_SIZE];
  size_t bump;
} arena;

STRUCT(PrologDatabase) {
  PrologArena arena;
  struct {
    Term** facts;
    size_t size;
    size_t cap;
  };
};

STRUCT(PrologGoal) {
  PrologArena arena;
};


PROLOG_PRIVATE size_t _prolog_align(size_t n, size_t align) { return (n + align - 1) & -align; }

PROLOG_PRIVATE char* prolog_alloc(PrologArena* a, size_t bytes, size_t align) {
  size_t from = _prolog_align(a->bump, align);
  a->bump = from + bytes;
#if PROLOG_CATCH_OOM
  if ((from + bytes) >= PROLOG_ARENA_SIZE)
    fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  return a->buf + from;
}

PROLOG_PUBLIC PrologDatabase* prolog_database_init(PrologDatabase* db) {
  db->bump = 0;
  db->facts = NULL;
  db->size = 0;
  db->cap = 0;
  return ctx;
}

PROLOG_PUBLIC Term* prolog_newVariable(PrologArena* a, char* name) {
  Term* t = (Term*)prolog_alloc(a, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(a, strlen(name), 1);
  t.kind = PROLOG_KIND_VARIABLE;
  t.v.name = strcpy(n, name);
  t.v.bound = NULL;
  return t;
}

PROLOG_PUBLIC Term* prolog_newAtom(PrologArena* a, char* name) {
  Term* t = (Term*)prolog_alloc(a, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(a, strlen(name), 1);
  t.kind = PROLOG_KIND_ATOM;
  t.a.name = strcpy(n, name);
  return t;
}

PROLOG_PUBLIC Term* prolog_newCompound(PrologArena* ctx, char* name, size_t arity, ...) {
  Term* t = (Term*)prolog_alloc(a, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(a, strlen(name), 1);
  Term** c = (Term**)prolog_alloc(a, sizeof(Term*) * arity, _Alignof(Term*));
  t.kind = PROLOG_KIND_COMPOUND;
  t.c.arity = arity;
  t.c.name = strcpy(n, name);
  t.c.components = c;
  va_list argp;
  va_start(argp, arity);
  for (size_t i = 0; i < arity; i++)
    t.c.components[i] = va_arg(argp, Term*);
  va_end(argp);
  return t;
}

PROLOG_PUBLIC void prolog_addFact(PrologDatabase* db, Term* fact) {
  if (!db->facts) {
    db->facts = (Term**)malloc(sizeof(Term) * 64);
    db->cap = 64;
#if PROLOG_CATCH_OOM
    if (!db->facts)
      fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  } else if (db->size >= db->cap) {
    db->facts = (Term**)realloc(sizeof(Term) * (db->cap *= 2))
#if PROLOG_CATCH_OOM
    if (!db->facts)
      fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  }
  db->facts[db->size++] = fact;
}

STRUCT(PrologUnification) {
  int success;
  TermPair* unifiers;
  size_t size;
  size_t _cap;
};

PROLOG_PUBLIC void prolog_unificationDestroy(PrologUnification* u) {
  if (u->unifiers)
    free(u->unifiers);
  u->unifiers = NULL;
  u->size = 0;
  u->_cap = 0;
}

PROLOG_PRIVATE void _prolog_replaceVars(Term* in, Term* with) {
  if (in->kind == PROLOG_KIND_VARIABLE)
    in->v.bound = with;
  else if (in->kind == PROLOG_KIND_COMPOUND)
    for (size_t i = 0; i < in->c.arity; i++)
      _prolog_replaceVars(in->components[i], with);
  else { /* Do nothing for Atoms */ (void)with; }
}

PROLOG_PRIVATE void _prolog_push_unifier(PrologUnification* u, Term* a, Term* b) {
  if (u->size >= u->_cap) {
    u->unifiers = (Term**)realloc(u->unifiers, sizeof(Term*) * (u->_cap *= 2));
#if PROLOG_CATCH_OOM
    if (!u->unifiers)
      fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  }
  u->unifiers[u->size++] = (TermPair){a, b};
}

PROLOG_PUBLIC PrologUnification prolog_unify(PrologContext* ctx, Term* t1, Term* t2) {
  typedef struct { Term* x; Term* y; } TermPair;
  PrologUnification q = {1, (TermPair**)malloc(sizeof(TermPair) * 64), 0, 64};
#if PROLOG_CATCH_OOM
  if (!q.unifiers)
    fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  TermPair stack[PROLOG_STACK_SIZE];
  stack[0].x = t1, stack[0].y = t2;
  size_t stack_height = 1;
  while (stack_height) {
    if (stack_height >= PROLOG_STACK_HEIGHT)
      fprintf("Prolog stack overflow.\n"), exit(1);
    TermPair tp = stack[--stack_height];
    if (((tp.x.kind == PROLOG_KIND_VARIABLE) & (tp.y.kind != PROLOG_KIND_VARIABLE)) |
        ((tp.x.kind == PROLOG_KIND_VARIABLE) & (tp.x.kind != PROLOG_KIND_VARIABLE))) {
      Term* var = (tp.x.kind == PROLOG_KIND_VARIABLE) ? tp.x : tp.y;
      /* Substitute variable in stack */
      for (size_t i = 0; i < stack_height; i++) {
        _prolog_replaceVars(stack[i].x, var);
        _prolog_replaceVars(stack[i].y, var);
      }
      _prolog_push_unifier(&q, tp.x, tp.y);
    }
    else if (((tp.x.kind == PROLOG_KIND_VARIABLE) | (tp.x.kind == PROLOG_KIND_ATOM)) &
             ((tp.y.kind == PROLOG_KIND_VARIABLE) | (tp.y.kind == PROLOG_KIND_ATOM))) {
      Term* xbound = (tp.x.kind == PROLOG_KIND_VARIABLE) ? tp.x.v.bound : tp.x.a.bound;
      Term* ybound = (tp.y.kind == PROLOG_KIND_VARIABLE) ? tp.y.v.bound : tp.y.a.bound;
      /* intentional ptr comp, unify if x and y bound to same term, or both to no term. */
      if (xbound == ybound)
        continue;
    }
    else if (((tp.x.kind == PROLOG_KIND_COMPOUND) & (tp.y.kind == PROLOG_KIND_COMPOUND))
             && (tp.x.c.arity) && (tp.x.c.arity == tp.y.c.arity)) {
      /* Unify all subterms */
      for (size_t i = 0; i < tp.x.c.arity; i++)
        stack[stack_height++] = (TermPair){tp.x.c.components[i], tp.y.c.components[i]};
    }
    else {
      q.success = 0;
      break;
    }
  }
  if (!q.success)
    prolog_unificationDestroy(&q);
  return q;
}

PROLOG_PRIVATE _prolog_backtrack(PrologContext* ctx) {

}

PROLOG_PUBLIC prolog_resolve(PrologContext* ctx, Goal* query) {

}
