#include <stdlib.h>
#include <stdio.h>

#define DECL(Name)           \
  struct Name;               \
  typedef struct Name Name
#define UDECL(Name)          \
  union Name;                \
  typedef union Name Name
#define STRUCT(Name)         \
  struct Name;               \
  typedef struct Name Name;  \
  struct Name
#define UNION(Name)          \
  union Name;                \
  typedef struct Name Name;  \
  union Name

#define PAGE_SIZE 4096
#define PROLOG_ARENA_SIZE PAGE_SIZE * 64
#define PROLOG_STACK_SIZE 1024

STRUCT(Variable) {
  char* name;
  char* bound;
};
STRUCT(Atom) {
  char* name;
};
DECL(Compound) {
  char* name;
  size_t arity;
  Term** components;
};
STRUCT(Term) {
  union {
#define PROLOG_KIND_ATOM     1
    Atom a;
#define PROLOG_KIND_VARIABLE 2
    Variable v;
#define PROLOG_KIND_COMPOUND 3
    Compound c;
  };
  unsigned char kind;
};

STRUCT(PrologContext) {
  struct {
    char buf[PROLOG_ARENA_SIZE];
    size_t bump;
  } arena;
  struct {
    Term** facts;
    size_t size;
    size_t cap;
  } db;
  size_t idcnt;
};

static inline size_t _prolog_align(size_t n, size_t align) {
    return (n + align - 1) & -align;
}

static inline char* prolog_alloc(PrologContext* ctx, size_t bytes, size_t align) {
  size_t from = _prolog_align(ctx->arena.bump, align);
  ctx->arena.bump = from + bytes;
#ifndef PROLOG_CATCH_OOM
#define PROLOG_CATCH_OOM 1
#endif
#if PROLOG_CATCH_OOM
  if ((from + bytes) >= PROLOG_ARENA_SIZE)
    fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  return ctx->arena.buf + from;
}

PrologContext* prolog_init(PrologContext* ctx) {
  ctx->bump = 0;
  ctx->db.facts = NULL;
  ctx->db.size = 0;
  ctx->db.cap = 0;
  ctx->idcnt = 0;
  return ctx;
}

Term* prolog_newVariable(PrologContext* ctx, char* name) {
  Term* t = (Term*)prolog_alloc(ctx, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(ctx, strlen(name), 1);
  t.kind = PROLOG_KIND_VARIABLE;
  t.v.name = strcpy(n, name);
  t.v.bound = NULL;
  return t;
}

Term* prolog_newAtom(PrologContext* ctx, char* name) {
  Term* t = (Term*)prolog_alloc(ctx, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(ctx, strlen(name), 1);
  t.kind = PROLOG_KIND_ATOM;
  t.a.name = strcpy(n, name);
  return t;
}

Term* prolog_newVariable(PrologContext* ctx, char* name, size_t arity, ...) {
  Term* t = (Term*)prolog_alloc(ctx, sizeof(Term), _Alignof(Term));
  char* n = (char*)prolog_alloc(ctx, strlen(name), 1);
  Term** c = (Term**)prolog_alloc(ctx, sizeof(Term*) * arity, _Alignof(Term*));
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

void prolog_addFact(PrologContext* ctx, Term* fact) {
  if (!ctx->db.facts) {
    ctx->db.facts = (Term**)malloc(sizeof(Term) * 64);
    ctx->db.cap = 64;
#if PROLOG_CATCH_OOM
    if (!ctx->db.facts)
      fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  } else if (ctx->db.size >= ctx->db.cap) {
    ctx->db.facts = (Term**)realloc(sizeof(Term) * (ctx->db.cap *= 2))
#if PROLOG_CATCH_OOM
    if (!ctx->db.facts)
      fprintf(stderr, "Prolog OOM.\n"), exit(1);
#endif
  }
  ctx->db.facts[ctx->db.size++] = fact;
}


typedef struct {
#define PROLOG_TRUE  1
#define PROLOG_FALSE 0
#define PROLOG_UNK  -1
  int status;
} PrologUnificationResult;

PrologUnificationResult prolog_unify(PrologContext* ctx, Term* t1, Term* t2) {

  int status = 0;

  typedef struct { Term* x; Term* y; } TermPair;
  TermPair stack[PROLOG_STACK_SIZE];
  stack[0].x = t1, stack[0].y = t2;
  size_t stack_height = 1;

  while ((stack_height) && (stack_height < PROLOG_STACK_HEIGHT)) {
    TermPair tp = stack[--stack_height];

    if ((tp.x.kind == PROLOG_KIND_VARIABLE) & (tp.y.kind != PROLOG_KIND_VARIABLE)) {

    }
    else if ((tp.x.kind == PROLOG_KIND_VARIABLE) & (tp.x.kind != PROLOG_KIND_VARIABLE)) {

    }
    else if (((tp.x.kind == PROLOG_KIND_VARIABLE) | (tp.x.kind == PROLOG_KIND_ATOM)) &
             ((tp.y.kind == PROLOG_KIND_VARIABLE) | (tp.y.kind == PROLOG_KIND_ATOM))) {
      char* xname = (tp.x.kind == PROLOG_KIND_VARIABLE) ? tp.x.v.name : tp.x.a.name;
      char* yname = (tp.y.kind == PROLOG_KIND_VARIABLE) ? tp.y.v.name : tp.y.a.name;
      if (xname == yname /* intentional ptr comp */)
        continue;
    }
    else if (((tp.x.kind == PROLOG_KIND_COMPOUND) & (tp.y.kind == PROLOG_KIND_COMPOUND))
             && (tp.x.c.arity) && (tp.x.c.arity == tp.y.c.arity)) {
      for (size_t i = 0; i < tp.x.c.arity; i++) {
        stack[stack_height++] = (TermPair){tp.x.c.components[i], tp.y.c.components[i]};
      }
    }
    else {
      status = PROLOG_FALSE;
      break;
    }

  }
}
