
#define PROLOG_STACK_SIZE 1024

#define PROLOG_ARENA_SIZE 4096 * 64

#ifndef PROLOG_CATCH_OOM
#define PROLOG_CATCH_OOM 1
#endif

#define PROLOG_PRIVATE static inline

#ifndef PROLOG_EXPORT
#define PROLOG_EXPORT 1
#endif
#if     PROLOG_EXPORT
#define PROLOG_PUBLIC
#else
#define PROLOG_PUBLIC PROLOG_PRIVATE
#endif

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
