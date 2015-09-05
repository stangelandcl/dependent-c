#ifndef DEPENDENT_C_TYPE_H
#define DEPENDENT_C_TYPE_H

#include <stdbool.h>

#include "dependent-c/ast.h"
#include "dependent-c/general.h"

/* Analyze the dependencies in the type signatures of top level declarations,
 * returning true and the order if successful. If there is a cyclical
 * dependency in the types a message is printed to stderr and false is
 * returned.
 */
bool top_level_topological_sort(size_t len, const TopLevel top_levels[len],
    size_t order[len]);

bool type_check(Context *context, const Expr *expr, const Expr *type);
bool type_infer(Context *context, const Expr *expr, Expr *result);
bool type_equal(Context *context, const Expr *type1, const Expr *type2);
bool type_eval(Context *context, const Expr *type, Expr *result);

bool type_check_top_level(Context *context, const TopLevel *top_level);

#endif /* DEPENDENT_C_TYPE_H */
