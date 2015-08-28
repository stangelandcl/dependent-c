#ifndef DEPENDENT_C_AST
#define DEPENDENT_C_AST

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/***** Literals **************************************************************/
typedef enum {
    // The type of types
      LIT_TYPE
    // Types representing integrals with a certain number of bits.
    , LIT_VOID
    , LIT_U8,       LIT_S8
    , LIT_U16,      LIT_S16
    , LIT_U32,      LIT_S32
    , LIT_U64,      LIT_S64
    // Literal integers
    , LIT_INTEGRAL
} LiteralTag;

typedef struct {
    LiteralTag tag;
    union {
        uint64_t integral;
    } data;
} Literal;

/***** Expressions ***********************************************************/
typedef enum {
    // Misc.
      EXPR_LITERAL      // type, u8, s64, 42, etc
    , EXPR_IDENT        // foo, bar, baz, etc

    // Function type and destructor. Constructor is a declaration, not an
    // expression.
    , EXPR_FUNC_TYPE    // Expr '(' { Expr | Expr Ident }(',') ')'
    , EXPR_CALL         // Expr '(' { Expr }(',') ')'

    // Product/Union type, constructor, and destructor.
    , EXPR_STRUCT       // 'struct' '{' { Expr Ident ';' }() '}'
    , EXPR_UNION        // 'union' '{' { Expr Ident ';' }() '}'
    , EXPR_PACK         // '(' Expr ')' '{' { '.' Ident '=' Expr }(',') '}'
    , EXPR_MEMBER       // Expr '.' Ident

    // Pointer type, constructor, and destructor.
    , EXPR_POINTER      // Expr '*'
    , EXPR_REFERENCE    // '&' Expr
    , EXPR_DEREFERENCE  // '*' Expr

    // Ambiguous nodes
    , EXPR_FUNC_TYPE_OR_CALL
} ExprTag;

typedef struct Expr Expr;
struct Expr {
    ExprTag tag;
    union {
        Literal literal;
        const char *ident;

        struct {
            Expr *ret_type;
            size_t num_params;
            Expr *param_types;
            const char **param_names; // Values may be NULL if params not named.
        } func_type;
        struct {
            Expr *func;
            size_t num_args;
            Expr *args;
        } call;

        struct {
            size_t num_fields;
            Expr *field_types;
            const char **field_names;
        } struct_;
        struct {
            size_t num_fields;
            Expr *field_types;
            const char **field_names;
        } union_;
        struct {
            Expr *type;
            size_t num_assigns;
            const char **field_names;
            Expr *assigns;
        } pack;
        struct {
            Expr *record;
            const char *field;
        } member;

        Expr *pointer;
        Expr *reference;
        Expr *dereference;

        struct {
            Expr *ret_type_or_func;
            size_t num_params_or_args;
            Expr *param_types_or_args;
            const char **param_names;
        } func_type_or_call;
    } data;
};

/***** Statements ************************************************************/
typedef enum {
      STATEMENT_EMPTY
    , STATEMENT_EXPR
    , STATEMENT_BLOCK
    , STATEMENT_DECL
} StatementTag;

typedef struct Statement Statement;
struct Statement {
    StatementTag tag;
    union {
        Expr expr;

        struct {
            size_t num_statements;
            Statement *statements;
        } block;

        struct {
            Expr type;
            const char *name;
            bool is_initialized;
            Expr initial_value;
        } decl;
    } data;
};

/***** Top-Level Definitions *************************************************/
typedef enum {
      TOP_LEVEL_FUNC
} TopLevelTag;

typedef struct {
    TopLevelTag tag;
    const char *name;
    union {
        struct {
            Expr ret_type;
            size_t num_params;
            Expr *param_types;
            const char **param_names;
            size_t num_statements;
            Statement *statements;
        } func;
    } data;
} TopLevel;

typedef struct {
    size_t num_top_levels;
    TopLevel *top_levels;
} TranslationUnit;

/* Free any resources associated with an expression. */
void expr_free(Expr expr);
void expr_pprint(FILE *to, int nesting, Expr expr);

/* Determine if two expressions are exactly equivalent. Does not take into
 * account alpha equivalence.
 */
bool expr_equal(Expr x, Expr y);

/* Make a deep copy of an expression. */
Expr expr_copy(Expr x);

/* Free any resources associated with an expression. */
void statement_free(Statement statement);
void statement_pprint(FILE *to, int nesting, Statement statement);

/* Free any resources associated with a top level definition. */
void top_level_free(TopLevel top_level);
void top_level_pprint(FILE *to, TopLevel top_level);

/* Free any resources associated with a translation unit. */
void translation_unit_free(TranslationUnit unit);
void translation_unit_pprint(FILE *to, TranslationUnit unit);

#endif /* DEPENDENT_C_AST */
