#include <stdlib.h>

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

DECL(Goal);
DECL(AndGoal);
DECL(OrGoal);
DECL(ExistsGoal);
DECL(ForallGoal);
DECL(IfGoal);

DECL(Clause);
DECL(HornClause);
DECL(AndClause);
DECL(ForallClause);

DECL(Atom);
DECL(Unification);
DECL(Relation);


STRUCT(Variable)     { Clause* c; };

STRUCT(Universe) {
  Universe* parent;
  Variable* v;
};

STRUCT(Atom)         { char* name; };
STRUCT(AndGoal)      { Goal* g1; Goal* g2; };
STRUCT(OrGoal)       { Goal* g1; Goal* g2; };
STRUCT(ExistsGoal)   { Universe u; Goal* g; };
STRUCT(ForallGoal)   { Universe u; Goal* g; };
STRUCT(IfGoal)       { Clause* cond; Goal* then; };
STRUCT(Goal) {
  union {
    Atom a;
    AndGoal ag;
    OrGoal og;
    ExistsGoal eg;
    ForallGoal fg;
    IfGoal ig;
  };
  unsigned char l;
};

STRUCT(HornClause)   { Atom* a; };
STRUCT(AndClause)    { Clause* c1; Clause* c2; };
STRUCT(ForallClause) { Universe u; Clause* c; };
STRUCT(Clause) {
  union {
    Atom a;
    HornClause hc;
    AndClause ac;
    ForallClause fc;
  }
  unsigned char l;
}

typedef size_t Universe;

int main() {}
