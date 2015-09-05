#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "dependent-c/ast.h"
#include "dependent-c/general.h"
#include "dependent-c/memory.h"
#include "dependent-c/symbol_table.h"

/***** Expression Management *************************************************/
static bool literal_equal(Literal x, Literal y) {
    if (x.tag != y.tag) {
        return false;
    }

    switch (x.tag) {
      case LIT_TYPE:
      case LIT_VOID:
      case LIT_U8:
      case LIT_S8:
      case LIT_U16:
      case LIT_S16:
      case LIT_U32:
      case LIT_S32:
      case LIT_U64:
      case LIT_S64:
      case LIT_BOOL:
        return true;

      case LIT_INTEGRAL:
        return x.integral == y.integral;

      case LIT_BOOLEAN:
        return x.boolean == y.boolean;
    }
}

bool expr_equal(const Expr *x, const Expr *y) {
    if (x->tag != y->tag) {
        return false;
    }

    switch (x->tag) {
      case EXPR_LITERAL:
        return literal_equal(x->literal, y->literal);

      case EXPR_IDENT:
        return x->ident == y->ident;

      case EXPR_BIN_OP:
        return x->bin_op.op != y->bin_op.op
            && expr_equal(x->bin_op.expr1, y->bin_op.expr1)
            && expr_equal(x->bin_op.expr2, y->bin_op.expr2);

      case EXPR_IFTHENELSE:
        return expr_equal(x->ifthenelse.predicate, y->ifthenelse.predicate)
            && expr_equal(x->ifthenelse.then_, y->ifthenelse.then_)
            && expr_equal(x->ifthenelse.else_, y->ifthenelse.else_);

      case EXPR_FUNC_TYPE:
        if (!expr_equal(x->func_type.ret_type, y->func_type.ret_type)
                || x->func_type.num_params != y->func_type.num_params) {
            return false;
        }
        for (size_t i = 0; i < x->func_type.num_params; i++) {
            if (!expr_equal(&x->func_type.param_types[i],
                        &y->func_type.param_types[i])
                    || x->func_type.param_names[i]
                        != y->func_type.param_names[i]) {
                return false;
            }
        }
        return true;

      case EXPR_LAMBDA:
        if (x->lambda.num_params != y->lambda.num_params) {
            return false;
        }
        for (size_t i = 0; i < x->lambda.num_params; i++) {
            if (!expr_equal(x->lambda.param_types, y->lambda.param_types)
                    || x->lambda.param_names[i] != y->lambda.param_names[i]) {
                return false;
            }
        }
        return expr_equal(x->lambda.body, y->lambda.body);

      case EXPR_CALL:
        if (!expr_equal(x->call.func, y->call.func)
                || x->call.num_args != y->call.num_args) {
            return false;
        }
        for (size_t i = 0; i < x->call.num_args; i++) {
            if (!expr_equal(&x->call.args[i], &y->call.args[i])) {
                return false;
            }
        }
        return true;

      case EXPR_STRUCT:
        if (x->struct_.num_fields != y->struct_.num_fields) {
            return false;
        }
        for (size_t i = 0; i < x->struct_.num_fields; i++) {
            if (!expr_equal(&x->struct_.field_types[i],
                        &y->struct_.field_types[i])
                    || x->struct_.field_names[i]
                        != y->struct_.field_names[i]) {
                return false;
            }
        }
        return true;

      case EXPR_UNION:
        if (x->union_.num_fields != y->union_.num_fields) {
            return false;
        }
        for (size_t i = 0; i < x->union_.num_fields; i++) {
            if (!expr_equal(&x->union_.field_types[i],
                        &y->union_.field_types[i])
                    || x->union_.field_names[i]
                        != y->union_.field_names[i]) {
                return false;
            }
        }
        return true;

      case EXPR_PACK:
        if (!expr_equal(x->pack.type, y->pack.type)
                || x->pack.num_assigns != y->pack.num_assigns) {
            return false;
        }
        for (size_t i = 0; i < x->pack.num_assigns; i++) {
            if (x->pack.field_names[i] != y->pack.field_names[i]
                    || !expr_equal(&x->pack.assigns[i], &y->pack.assigns[i])) {
                return false;
            }
        }
        return true;

      case EXPR_MEMBER:
        if (!expr_equal(x->member.record, y->member.record)
                || x->member.field != y->member.field) {
            return false;
        }
        return true;

      case EXPR_POINTER:
      case EXPR_REFERENCE:
      case EXPR_DEREFERENCE:
        return expr_equal(x->pointer, y->pointer);

      case EXPR_STATEMENT:
        // TODO
        return false;
    }
}

static Literal literal_copy(const Literal *x) {
    switch (x->tag) {
      case LIT_TYPE:
      case LIT_VOID:
      case LIT_U8:      case LIT_S8:
      case LIT_U16:     case LIT_S16:
      case LIT_U32:     case LIT_S32:
      case LIT_U64:     case LIT_S64:
      case LIT_BOOL:
      case LIT_INTEGRAL:
      case LIT_BOOLEAN:
        return *x;
    }
}

Expr expr_copy(const Expr *x) {
    Expr y = {.location = x->location, .tag = x->tag};

    switch (x->tag) {
      case EXPR_LITERAL:
        y.literal = literal_copy(&x->literal);
        break;

      case EXPR_IDENT:
        y.ident = x->ident;
        break;

      case EXPR_BIN_OP:
        y.bin_op.op = x->bin_op.op;
        alloc(y.bin_op.expr1);
        *y.bin_op.expr1 = expr_copy(x->bin_op.expr1);
        alloc(y.bin_op.expr2);
        *y.bin_op.expr2 = expr_copy(x->bin_op.expr2);
        break;

      case EXPR_IFTHENELSE:
        alloc(y.ifthenelse.predicate);
        *y.ifthenelse.predicate = expr_copy(x->ifthenelse.predicate);
        alloc(y.ifthenelse.then_);
        *y.ifthenelse.then_ = expr_copy(x->ifthenelse.then_);
        alloc(y.ifthenelse.else_);
        *y.ifthenelse.else_ = expr_copy(x->ifthenelse.else_);
        break;

      case EXPR_FUNC_TYPE:
        alloc(y.func_type.ret_type);
        *y.func_type.ret_type = expr_copy(x->func_type.ret_type);
        y.func_type.num_params = x->func_type.num_params;
        alloc_array(y.func_type.param_types, y.func_type.num_params);
        alloc_array(y.func_type.param_names, y.func_type.num_params);
        for (size_t i = 0; i < y.func_type.num_params; i++) {
            y.func_type.param_types[i] = expr_copy(&x->func_type.param_types[i]);
            y.func_type.param_names[i] = x->func_type.param_names[i];
        }
        break;

      case EXPR_LAMBDA:
        y.lambda.num_params = x->lambda.num_params;
        alloc_array(y.lambda.param_types, y.lambda.num_params);
        alloc_array(y.lambda.param_names, y.lambda.num_params);
        for (size_t i = 0; i < y.lambda.num_params; i++) {
            y.lambda.param_types[i] = expr_copy(&x->lambda.param_types[i]);
            y.lambda.param_names[i] = x->lambda.param_names[i];
        }
        alloc(y.lambda.body);
        *y.lambda.body = expr_copy(x->lambda.body);
        break;

      case EXPR_CALL:
        alloc(y.call.func);
        *y.call.func = expr_copy(x->call.func);
        y.call.num_args = x->call.num_args;
        alloc_array(y.call.args, y.call.num_args);
        for (size_t i = 0; i < y.call.num_args; i++) {
            y.call.args[i] = expr_copy(&x->call.args[i]);
        }
        break;

      case EXPR_STRUCT:
        y.struct_.num_fields = x->struct_.num_fields;
        alloc_array(y.struct_.field_types, y.struct_.num_fields);
        alloc_array(y.struct_.field_names, y.struct_.num_fields);
        for (size_t i = 0; i < y.struct_.num_fields; i++) {
            y.struct_.field_types[i] = expr_copy(&x->struct_.field_types[i]);
            y.struct_.field_names[i] = x->struct_.field_names[i];
        }
        break;

      case EXPR_UNION:
        y.union_.num_fields = x->union_.num_fields;
        alloc_array(y.union_.field_types, y.union_.num_fields);
        alloc_array(y.union_.field_names, y.union_.num_fields);
        for (size_t i = 0; i < y.union_.num_fields; i++) {
            y.union_.field_types[i] = expr_copy(&x->union_.field_types[i]);
            y.union_.field_names[i] = x->union_.field_names[i];
        }
        break;

      case EXPR_PACK:
        alloc(y.pack.type);
        *y.pack.type = expr_copy(x->pack.type);
        y.pack.num_assigns = x->pack.num_assigns;
        alloc_array(y.pack.field_names, y.pack.num_assigns);
        alloc_array(y.pack.assigns, y.pack.num_assigns);
        for (size_t i = 0; i < y.pack.num_assigns; i++) {
            y.pack.field_names[i] = x->pack.field_names[i];
            y.pack.assigns[i] = expr_copy(&x->pack.assigns[i]);
        }
        break;

      case EXPR_MEMBER:
        alloc(y.member.record);
        *y.member.record = expr_copy(x->member.record);
        y.member.field = x->member.field;
        break;

      case EXPR_POINTER:
      case EXPR_REFERENCE:
      case EXPR_DEREFERENCE:
        alloc(y.pointer);
        *y.pointer = expr_copy(x->pointer);
        break;

      case EXPR_STATEMENT:
        alloc(y.statement);
        *y.statement = statement_copy(x->statement);
        break;
    }

    return y;
}

void expr_free_vars(const Expr *expr, SymbolSet *free_vars) {
    SymbolSet free_vars_temp[1];

    switch (expr->tag) {
      case EXPR_LITERAL:
        *free_vars = symbol_set_empty();
        break;

      case EXPR_IDENT:
        *free_vars = symbol_set_empty();
        symbol_set_add(free_vars, expr->ident);
        break;

      case EXPR_BIN_OP:
        expr_free_vars(expr->bin_op.expr1, free_vars);
        expr_free_vars(expr->bin_op.expr2, free_vars_temp);
        symbol_set_union(free_vars, free_vars_temp);
        break;

      case EXPR_IFTHENELSE:
        expr_free_vars(expr->ifthenelse.predicate, free_vars);
        expr_free_vars(expr->ifthenelse.then_, free_vars_temp);
        symbol_set_union(free_vars, free_vars_temp);
        expr_free_vars(expr->ifthenelse.else_, free_vars_temp);
        symbol_set_union(free_vars, free_vars_temp);
        break;

      case EXPR_FUNC_TYPE:
        expr_free_vars(expr->func_type.ret_type, free_vars);
        for (size_t i = 0; i < expr->func_type.num_params; i++) {
            if (expr->func_type.param_names[i] != NULL) {
                symbol_set_delete(free_vars, expr->func_type.param_names[i]);
            }
        }
        for (size_t i = 0; i < expr->func_type.num_params; i++) {
            expr_free_vars(&expr->func_type.param_types[i], free_vars_temp);
            for (size_t j = 0; j < i; j++) {
                if (expr->func_type.param_names[j] != NULL) {
                    symbol_set_delete(free_vars_temp,
                        expr->func_type.param_names[j]);
                }
            }
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_LAMBDA:
        expr_free_vars(expr->lambda.body, free_vars);
        for (size_t i = 0; i < expr->lambda.num_params; i++) {
            symbol_set_delete(free_vars, expr->lambda.param_names[i]);
        }
        for (size_t i = 0; i < expr->lambda.num_params; i++) {
            expr_free_vars(&expr->lambda.param_types[i], free_vars_temp);
            for (size_t j = 0; j < i; j++) {
                symbol_set_delete(free_vars_temp, expr->lambda.param_names[j]);
            }
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_CALL:
        expr_free_vars(expr->call.func, free_vars);
        for (size_t i = 0; i < expr->call.num_args; i++) {
            expr_free_vars(&expr->call.args[i], free_vars_temp);
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_STRUCT:
        *free_vars = symbol_set_empty();
        for (size_t i = 0; i < expr->struct_.num_fields; i++) {
            expr_free_vars(&expr->struct_.field_types[i], free_vars_temp);
            for (size_t j = 0; j < i; j++) {
                symbol_set_delete(free_vars_temp, expr->struct_.field_names[j]);
            }
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_UNION:
        *free_vars = symbol_set_empty();
        for (size_t i = 0; i < expr->union_.num_fields; i++) {
            expr_free_vars(&expr->union_.field_types[i], free_vars_temp);
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_PACK:
        expr_free_vars(expr->pack.type, free_vars);
        for (size_t i = 0; i < expr->pack.num_assigns; i++) {
            expr_free_vars(&expr->pack.assigns[i], free_vars_temp);
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case EXPR_MEMBER:
        expr_free_vars(expr->member.record, free_vars);
        break;

      case EXPR_POINTER:
      case EXPR_REFERENCE:
      case EXPR_DEREFERENCE:
        expr_free_vars(expr->pointer, free_vars);
        break;

      case EXPR_STATEMENT:
        statement_free_vars(expr->statement, free_vars);
    }
}

static bool expr_func_type_subst(Context *context, Expr *expr,
        const char *name, const Expr *replacement) {
    assert(expr->tag == EXPR_FUNC_TYPE);
    bool ret_val = false;

    SymbolSet free_vars;
    expr_free_vars(replacement, &free_vars);

    for (size_t i = 0; i < expr->func_type.num_params; i++) {
        if (!expr_subst(context, &expr->func_type.param_types[i],
                name, replacement)) {
            goto end_of_function;
        }
        const char *old_param_name = expr->func_type.param_names[i];

        if (old_param_name != NULL) {
            if (old_param_name == name) {
                ret_val = true;
                goto end_of_function;
            }

            if (symbol_set_contains(&free_vars, old_param_name)) {
                const char *new_param_name = symbol_gensym(&context->interns,
                    old_param_name);
                Expr new_replacement = (Expr){
                      .tag = EXPR_IDENT
                    , .ident = new_param_name
                };
                expr->func_type.param_names[i] = new_param_name;

                for (size_t j = i + 1; j < expr->func_type.num_params; j++) {
                    if (!expr_subst(context,
                            &expr->func_type.param_types[i],
                            old_param_name, &new_replacement)) {
                        goto end_of_function;
                    }
                }
                if (!expr_subst(context, expr->func_type.ret_type,
                        old_param_name, &new_replacement)) {
                    goto end_of_function;
                }
            }
        }
    }
    if (!expr_subst(context, expr->func_type.ret_type, name, replacement)) {
        goto end_of_function;
    }

    ret_val = true;

end_of_function:
    symbol_set_free(&free_vars);
    return ret_val;
}

static bool expr_lambda_subst(Context *context, Expr *expr,
        const char *name, const Expr *replacement) {
    assert(expr->tag == EXPR_LAMBDA);
    bool ret_val = false;

    SymbolSet free_vars[1];
    expr_free_vars(replacement, free_vars);

    for (size_t i = 0; i < expr->lambda.num_params; i++) {
        if (!expr_subst(context, &expr->lambda.param_types[i],
                name, replacement)) {
            goto end_of_function;
        }

        const char *old_param_name = expr->lambda.param_names[i];

        if (old_param_name == name) {
            ret_val = true;
            goto end_of_function;
        }

        if (symbol_set_contains(free_vars, old_param_name)) {
            const char *new_param_name = symbol_gensym(&context->interns,
                old_param_name);
            Expr new_replacement = (Expr){
                  .tag = EXPR_IDENT
                , .ident = new_param_name
            };
            expr->lambda.param_names[i] = new_param_name;

            for (size_t j = i + 1; j < expr->func_type.num_params; j++) {
                if (!expr_subst(context,
                        &expr->lambda.param_types[i],
                        old_param_name, &new_replacement)) {
                    goto end_of_function;
                }
            }
            if (!expr_subst(context, expr->lambda.body,
                    old_param_name, &new_replacement)) {
                goto end_of_function;
            }
        }
    }

    if (!expr_subst(context, expr->lambda.body, name, replacement)) {
        goto end_of_function;
    }

    ret_val = true;

end_of_function:
    symbol_set_free(free_vars);
    return ret_val;
}

static bool expr_struct_subst(Context *context, Expr *expr,
        const char *name, const Expr *replacement) {
    assert(expr->tag == EXPR_STRUCT);
    bool ret_val = false;

    SymbolSet free_vars;
    expr_free_vars(replacement, &free_vars);

    for (size_t i = 0; i < expr->struct_.num_fields; i++) {
        if (!expr_subst(context, &expr->struct_.field_types[i],
                name, replacement)) {
            goto end_of_function;
        }
        const char *old_field_name = expr->struct_.field_names[i];

        if (old_field_name == name) {
            ret_val = true;
            goto end_of_function;
        }

        if (symbol_set_contains(&free_vars, old_field_name)) {
            goto end_of_function;
        }
    }

    ret_val = true;

end_of_function:
    symbol_set_free(&free_vars);
    return ret_val;
}

// This one's a bit weird since a pack could either be a struct or a union.
// In the case of a union we should ignore the field names entirely and just
// substitute through all assignments. In the case of a struct we need to look
// at field names to see if there is shadowing or capturing going on.
//
// Problem is, we don't know whether we're dealing with a struct or union here.
//
// Solution: Since unions *should* only have one assignment, use the struct
// algorithm, which coincides with the union one when only one field is being
// assigned. When type checking we should ensure that this in indeed the case
// by making sure union packings have exactly one assignment.
static bool expr_pack_subst(Context *context, Expr *expr,
        const char *name, const Expr *replacement) {
    assert(expr->tag == EXPR_PACK);
    bool ret_val = false;

    SymbolSet free_vars;
    expr_free_vars(replacement, &free_vars);

    for (size_t i = 0; i < expr->pack.num_assigns; i++) {
        if (!expr_subst(context, &expr->pack.assigns[i], name, replacement)) {
            goto end_of_function;
        }
        const char *old_field_name = expr->pack.field_names[i];

        if (old_field_name == name) {
            ret_val = true;
            goto end_of_function;
        }

        if (symbol_set_contains(&free_vars, old_field_name)) {
            goto end_of_function;
        }
    }

    ret_val = true;

end_of_function:
    symbol_set_free(&free_vars);
    return ret_val;
}

bool expr_subst(Context *context, Expr *expr,
        const char *name, const Expr *replacement) {
    switch (expr->tag) {
      case EXPR_LITERAL:
        return true;

      case EXPR_IDENT:
        expr_free(expr);
        *expr = expr_copy(replacement);
        return true;

      case EXPR_BIN_OP:
        return expr_subst(context, expr->bin_op.expr1, name, replacement)
            && expr_subst(context, expr->bin_op.expr2, name, replacement);

      case EXPR_IFTHENELSE:
        return expr_subst(context, expr->ifthenelse.predicate, name, replacement)
            && expr_subst(context, expr->ifthenelse.then_, name, replacement)
            && expr_subst(context, expr->ifthenelse.else_, name, replacement);

      case EXPR_FUNC_TYPE:
        return expr_func_type_subst(context, expr, name, replacement);

      case EXPR_LAMBDA:
        return expr_lambda_subst(context, expr, name, replacement);

      case EXPR_CALL:
        if (!expr_subst(context, expr->call.func, name, replacement)) {
            return false;
        }
        for (size_t i = 0; i < expr->call.num_args; i++) {
            if (!expr_subst(context, &expr->call.args[i], name, replacement)) {
                return false;
            }
        }
        return true;

      case EXPR_STRUCT:
        return expr_struct_subst(context, expr, name, replacement);

      case EXPR_UNION:
        for (size_t i = 0; i < expr->union_.num_fields; i++) {
            if (!expr_subst(context, &expr->union_.field_types[i],
                    name, replacement)) {
                return false;
            }
        }
        return true;

      case EXPR_PACK:
        return expr_pack_subst(context, expr, name, replacement);

      case EXPR_MEMBER:
        return expr_subst(context, expr->member.record, name, replacement);

      case EXPR_POINTER:
      case EXPR_REFERENCE:
      case EXPR_DEREFERENCE:
        return expr_subst(context, expr->pointer, name, replacement);

      case EXPR_STATEMENT:
        return statement_subst(context, expr->statement, name, replacement);
    }
}

/***** Statement Management **************************************************/
Statement statement_copy(const Statement *statement) {
    Statement result = {.tag = statement->tag, .location = statement->location};

    switch (statement->tag) {
      case STATEMENT_EMPTY:
        break;

      case STATEMENT_EXPR:
      case STATEMENT_RETURN:
        result.expr = expr_copy(&statement->expr);
        break;

      case STATEMENT_BLOCK:
        result.block = block_copy(&statement->block);
        break;

      case STATEMENT_DECL:
        result.decl.type = expr_copy(&statement->decl.type);
        result.decl.name = statement->decl.name;
        result.decl.is_initialized = statement->decl.is_initialized;
        if (result.decl.is_initialized) {
            result.decl.initial_value =
                expr_copy(&statement->decl.initial_value);
        }
        break;

      case STATEMENT_IFTHENELSE:
        result.ifthenelse.num_ifs = statement->ifthenelse.num_ifs;
        alloc_array(result.ifthenelse.ifs, result.ifthenelse.num_ifs);
        for (size_t i = 0; i < result.ifthenelse.num_ifs; i++) {
            result.ifthenelse.ifs[i] = expr_copy(&statement->ifthenelse.ifs[i]);
        }
        alloc_array(result.ifthenelse.thens, result.ifthenelse.num_ifs);
        for (size_t i = 0; i < result.ifthenelse.num_ifs; i++) {
            result.ifthenelse.thens[i] =
                block_copy(&statement->ifthenelse.thens[i]);
        }
        result.ifthenelse.else_ = block_copy(&statement->ifthenelse.else_);
        break;
    }

    return result;
}

void statement_free_vars(const Statement *statement, SymbolSet *free_vars) {
    SymbolSet free_vars_temp[1];

    switch (statement->tag) {
      case STATEMENT_EMPTY:
        *free_vars = symbol_set_empty();
        break;

      case STATEMENT_EXPR:
      case STATEMENT_RETURN:
        expr_free_vars(&statement->expr, free_vars);
        break;

      case STATEMENT_BLOCK:
        block_free_vars(&statement->block, free_vars);
        break;

      case STATEMENT_DECL:
        expr_free_vars(&statement->decl.type, free_vars);
        if (statement->decl.is_initialized) {
            expr_free_vars(&statement->decl.initial_value, free_vars_temp);
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;

      case STATEMENT_IFTHENELSE:
        block_free_vars(&statement->ifthenelse.else_, free_vars);
        for (size_t i = 0; i < statement->ifthenelse.num_ifs; i++) {
            expr_free_vars(&statement->ifthenelse.ifs[i], free_vars_temp);
            symbol_set_union(free_vars, free_vars_temp);
            block_free_vars(&statement->ifthenelse.thens[i], free_vars);
            symbol_set_union(free_vars, free_vars_temp);
        }
        break;
    }
}

bool statement_subst(Context *context, Statement *statement,
        const char *name, const Expr *replacement) {
    switch (statement->tag) {
      case STATEMENT_EMPTY:
        return true;

      case STATEMENT_EXPR:
      case STATEMENT_RETURN:
        return expr_subst(context, &statement->expr, name, replacement);

      case STATEMENT_BLOCK:
        return block_subst(context, &statement->block, name, replacement);

      case STATEMENT_DECL:
        if (!expr_subst(context, &statement->decl.type, name, replacement)) {
            return false;
        }
        if (statement->decl.is_initialized) {
            return expr_subst(context, &statement->decl.initial_value,
                name, replacement);
        } else {
            return true;
        }

      case STATEMENT_IFTHENELSE:
        for (size_t i = 0; i < statement->ifthenelse.num_ifs; i++) {
            if (!expr_subst(context, &statement->ifthenelse.ifs[i],
                        name, replacement)
                    || !block_subst(context, &statement->ifthenelse.thens[i],
                        name, replacement)) {
                return false;
            }
        }
        return block_subst(context, &statement->ifthenelse.else_,
            name, replacement);
    }
}

Block block_copy(const Block *block) {
    Block result = {.num_statements = block->num_statements};
    alloc_array(result.statements, result.num_statements);

    for (size_t i = 0; i < result.num_statements; i++) {
        result.statements[i] = statement_copy(&block->statements[i]);
    }

    return result;
}

void block_free_vars(const Block *block, SymbolSet *free_vars) {
    SymbolSet free_vars_temp[1];
    *free_vars = symbol_set_empty();

    for (size_t i = 0; i < block->num_statements; i++) {
        const Statement *statement = &block->statements[
            block->num_statements - i - 1];

        switch (statement->tag) {
          case STATEMENT_DECL:
            symbol_set_delete(free_vars, statement->decl.name);
            break;

          case STATEMENT_EMPTY:
          case STATEMENT_EXPR:
          case STATEMENT_RETURN:
          case STATEMENT_BLOCK:
          case STATEMENT_IFTHENELSE:
            break;
        }

        statement_free_vars(statement, free_vars_temp);
        symbol_set_union(free_vars, free_vars_temp);
    }
}

bool block_subst(Context *context, Block *block,
        const char *name, const Expr *replacement) {
    for (size_t i = 0; i < block->num_statements; i++) {
        if (!statement_subst(context, &block->statements[i],
                name, replacement)) {
            return false;
        }
    }

    return true;
}

/***** Freeing ast nodes *****************************************************/
void expr_free(Expr *expr) {
    switch (expr->tag) {
      case EXPR_LITERAL:
      case EXPR_IDENT:
        break;

      case EXPR_BIN_OP:
        expr_free(expr->bin_op.expr1);
        dealloc(expr->bin_op.expr1);
        expr_free(expr->bin_op.expr2);
        dealloc(expr->bin_op.expr2);
        break;

      case EXPR_IFTHENELSE:
        expr_free(expr->ifthenelse.predicate);
        dealloc(expr->ifthenelse.predicate);
        expr_free(expr->ifthenelse.then_);
        dealloc(expr->ifthenelse.then_);
        expr_free(expr->ifthenelse.else_);
        dealloc(expr->ifthenelse.else_);
        break;

      case EXPR_FUNC_TYPE:
        expr_free(expr->func_type.ret_type);
        dealloc(expr->func_type.ret_type);
        for (size_t i = 0; i < expr->func_type.num_params; i++) {
            expr_free(&expr->func_type.param_types[i]);
        }
        dealloc(expr->func_type.param_types);
        dealloc(expr->func_type.param_names);
        break;

      case EXPR_LAMBDA:
        for (size_t i = 0; i < expr->lambda.num_params; i++) {
            expr_free(&expr->lambda.param_types[i]);
        }
        dealloc(expr->lambda.param_types);
        dealloc(expr->lambda.param_names);
        expr_free(expr->lambda.body);
        dealloc(expr->lambda.body);
        break;

      case EXPR_CALL:
        expr_free(expr->call.func);
        dealloc(expr->call.func);
        for (size_t i = 0; i < expr->call.num_args; i++) {
            expr_free(&expr->call.args[i]);
        }
        dealloc(expr->call.args);
        break;

      case EXPR_STRUCT:
        for (size_t i = 0; i < expr->struct_.num_fields; i++) {
            expr_free(&expr->struct_.field_types[i]);
        }
        dealloc(expr->struct_.field_types);
        dealloc(expr->struct_.field_names);
        break;

      case EXPR_UNION:
        for (size_t i = 0; i < expr->union_.num_fields; i++) {
            expr_free(&expr->union_.field_types[i]);
        }
        dealloc(expr->union_.field_types);
        dealloc(expr->union_.field_names);
        break;

      case EXPR_PACK:
        expr_free(expr->pack.type);
        dealloc(expr->pack.type);
        for (size_t i = 0; i < expr->pack.num_assigns; i++) {
            expr_free(&expr->pack.assigns[i]);
        }
        dealloc(expr->pack.field_names);
        dealloc(expr->pack.assigns);
        break;

      case EXPR_MEMBER:
        expr_free(expr->member.record);
        dealloc(expr->member.record);
        break;

      case EXPR_POINTER:
      case EXPR_REFERENCE:
      case EXPR_DEREFERENCE:
        expr_free(expr->pointer);
        dealloc(expr->pointer);
        break;

      case EXPR_STATEMENT:
        statement_free(expr->statement);
        dealloc(expr->statement);
        break;
    }
    memset(expr, 0, sizeof *expr);
}

void statement_free(Statement *statement) {
    switch (statement->tag) {
      case STATEMENT_EMPTY:
        break;

      case STATEMENT_EXPR:
      case STATEMENT_RETURN:
        expr_free(&statement->expr);
        break;

      case STATEMENT_BLOCK:
        block_free(&statement->block);
        break;

      case STATEMENT_DECL:
        expr_free(&statement->decl.type);
        if (statement->decl.is_initialized) {
            expr_free(&statement->decl.initial_value);
        }
        break;

      case STATEMENT_IFTHENELSE:
        for (size_t i = 0; i < statement->ifthenelse.num_ifs; i++) {
            expr_free(&statement->ifthenelse.ifs[i]);
            block_free(&statement->ifthenelse.thens[i]);
        }
        dealloc(statement->ifthenelse.ifs);
        dealloc(statement->ifthenelse.thens);
        block_free(&statement->ifthenelse.else_);
        break;
    }
    memset(statement, 0, sizeof *statement);
}

void block_free(Block *block) {
    for (size_t i = 0; i < block->num_statements; i++) {
        statement_free(&block->statements[i]);
    }
    dealloc(block->statements);
    memset(block, 0, sizeof *block);
}

void top_level_free(TopLevel *top_level) {
    switch (top_level->tag) {
      case TOP_LEVEL_FUNC:
        expr_free(&top_level->func.ret_type);
        for (size_t i = 0; i < top_level->func.num_params; i++) {
            expr_free(&top_level->func.param_types[i]);
        }
        dealloc(top_level->func.param_types);
        dealloc(top_level->func.param_names);
        expr_free(&top_level->func.body);
        break;
    }
    memset(top_level, 0, sizeof *top_level);
}

void translation_unit_free(TranslationUnit *unit) {
    for (size_t i = 0; i < unit->num_top_levels; i++) {
        top_level_free(&unit->top_levels[i]);
    }
    dealloc(unit->top_levels);
    memset(unit, 0, sizeof *unit);
}

/***** Pretty-printing ast nodes *********************************************/
static void print_indentation_whitespace(FILE *to, int nesting) {
    for (int i = 0; i < nesting; i++) {
        fprintf(to, "    ");
    }
}

#define tag_to_string(tag, str) case tag: fprintf(to, str); break;
static void literal_pprint(FILE *to, const Literal *literal) {
    switch (literal->tag) {
      tag_to_string(LIT_TYPE, "type")
      tag_to_string(LIT_VOID, "void")
      tag_to_string(LIT_U8, "u8")
      tag_to_string(LIT_S8, "s8")
      tag_to_string(LIT_U16, "u16")
      tag_to_string(LIT_S16, "s16")
      tag_to_string(LIT_U32, "u32")
      tag_to_string(LIT_S32, "s32")
      tag_to_string(LIT_U64, "u64")
      tag_to_string(LIT_S64, "s64")
      tag_to_string(LIT_BOOL, "bool")

      case LIT_INTEGRAL:
        fprintf(to, "%" PRIu64, literal->integral);
        break;

      case LIT_BOOLEAN:
        fprintf(to, literal->boolean ? "true" : "false");
        break;
    }
}

static void bin_op_pprint(FILE *to, BinaryOp bin_op) {
    switch (bin_op) {
      tag_to_string(BIN_OP_EQ,      " == ")
      tag_to_string(BIN_OP_NE,      " != ")
      tag_to_string(BIN_OP_LT,      " < ")
      tag_to_string(BIN_OP_LTE,     " <= ")
      tag_to_string(BIN_OP_GT,      " > ")
      tag_to_string(BIN_OP_GTE,     " >= ")
      tag_to_string(BIN_OP_ADD,     " + ")
      tag_to_string(BIN_OP_SUB,     " - ")
      tag_to_string(BIN_OP_ANDTHEN, " >> ")
    }
}
#undef tag_to_string

static void expr_pprint_(FILE *to, const Expr *expr) {
    bool simple = expr->tag == EXPR_LITERAL || expr->tag == EXPR_IDENT
            || expr->tag == EXPR_STRUCT || expr->tag == EXPR_UNION;

    if (!simple) putc('(', to);
    expr_pprint(to, expr);
    if (!simple) putc(')', to);
}

void expr_pprint(FILE *to, const Expr *expr) {
    switch (expr->tag) {
      case EXPR_LITERAL:
        literal_pprint(to, &expr->literal);
        break;

      case EXPR_IDENT:
        fputs(expr->ident, to);
        break;

      case EXPR_BIN_OP:
        expr_pprint_(to, expr->bin_op.expr1);
        bin_op_pprint(to, expr->bin_op.op);
        expr_pprint_(to, expr->bin_op.expr2);
        break;

      case EXPR_IFTHENELSE:
        fprintf(to, "if ");
        expr_pprint(to, expr->ifthenelse.predicate);
        fprintf(to, " then ");
        expr_pprint(to, expr->ifthenelse.then_);
        fprintf(to, " else ");
        expr_pprint(to, expr->ifthenelse.else_);
        break;

      case EXPR_FUNC_TYPE:
        expr_pprint_(to, expr->func_type.ret_type);
        putc('[', to);
        for (size_t i = 0; i < expr->func_type.num_params; i++) {
            if (i > 0) {
                fprintf(to, ", ");
            }

            expr_pprint(to, &expr->func_type.param_types[i]);
            if (expr->func_type.param_names[i] != NULL) {
                fprintf(to, " %s", expr->func_type.param_names[i]);
            }
        }
        putc(']', to);
        break;

      case EXPR_LAMBDA:
        fprintf(to, "\\(");
        for (size_t i = 0; i < expr->lambda.num_params; i++) {
            if (i > 0) {
                fprintf(to, ", ");
            }

            expr_pprint(to, &expr->lambda.param_types[i]);
            fprintf(to, " %s", expr->lambda.param_names[i]);
        }
        fprintf(to, ") -> ");
        expr_pprint(to, expr->lambda.body);
        break;

      case EXPR_CALL:
        expr_pprint_(to, expr->call.func);
        putc('(', to);
        for (size_t i = 0; i < expr->call.num_args; i++) {
            if (i > 0) {
                fprintf(to, ", ");
            }

            expr_pprint(to, &expr->call.args[i]);
        }
        putc(')', to);
        break;

      case EXPR_STRUCT:
        fprintf(to, "struct { ");
        for (size_t i = 0; i < expr->struct_.num_fields; i++) {
            expr_pprint(to, &expr->struct_.field_types[i]);
            fprintf(to, " %s; ", expr->struct_.field_names[i]);
        }
        putc('}', to);
        break;

      case EXPR_UNION:
        fprintf(to, "union { ");
        for (size_t i = 0; i < expr->union_.num_fields; i++) {
            expr_pprint(to, &expr->union_.field_types[i]);
            fprintf(to, " %s; ", expr->union_.field_names[i]);
        }
        putc('}', to);
        break;

      case EXPR_PACK:
        putc('[', to);
        expr_pprint_(to, expr->pack.type);
        fprintf(to, "]{");
        for (size_t i = 0; i < expr->pack.num_assigns; i++) {
            if (i > 0) {
                fprintf(to, ", ");
            }

            fprintf(to, ".%s = ", expr->pack.field_names[i]);
            expr_pprint(to, &expr->pack.assigns[i]);
        }
        putc('}', to);
        break;

      case EXPR_MEMBER:
        expr_pprint_(to, expr->member.record);
        fprintf(to, ".%s", expr->member.field);
        break;

      case EXPR_POINTER:
        expr_pprint_(to, expr->pointer);
        putc('*', to);
        break;

      case EXPR_REFERENCE:
        putc('&', to);
        expr_pprint_(to, expr->pointer);
        break;

      case EXPR_DEREFERENCE:
        putc('*', to);
        expr_pprint_(to, expr->pointer);
        break;

      case EXPR_STATEMENT:
        putc('[', to);
        statement_pprint(to, 0, expr->statement);
        putc(']', to);
        break;
    }
}

void statement_pprint(FILE *to, int nesting, const Statement *statement) {
    print_indentation_whitespace(to, nesting);

    switch (statement->tag) {
      case STATEMENT_EMPTY:
        fprintf(to, ";\n");
        break;

      case STATEMENT_RETURN:
        fprintf(to, "return ");
        // Fallthrough

      case STATEMENT_EXPR:
        expr_pprint(to, &statement->expr);
        fprintf(to, ";\n");
        break;

      case STATEMENT_BLOCK:
        fprintf(to, "{\n");
        block_pprint(to, nesting + 1, &statement->block);
        print_indentation_whitespace(to, nesting);
        fprintf(to, "}\n");
        break;

      case STATEMENT_DECL:
        expr_pprint(to, &statement->decl.type);
        fprintf(to, " %s", statement->decl.name);
        if (statement->decl.is_initialized) {
            fprintf(to, " = ");
            expr_pprint(to, &statement->decl.initial_value);
        }
        fprintf(to, ";\n");
        break;

      case STATEMENT_IFTHENELSE:
        for (size_t i = 0; i < statement->ifthenelse.num_ifs; i++) {
            if (i != 0) {
                print_indentation_whitespace(to, nesting);
                fprintf(to, "} else ");
            }
            fprintf(to, "if (");
            expr_pprint(to, &statement->ifthenelse.ifs[i]);
            fprintf(to, ") {\n");
            block_pprint(to, nesting + 1, &statement->ifthenelse.thens[i]);
        }
        print_indentation_whitespace(to, nesting);
        fprintf(to, "} else {\n");
        block_pprint(to, nesting + 1, &statement->ifthenelse.else_);
        print_indentation_whitespace(to, nesting);
        fprintf(to, "}\n");
        break;
    }
}

void block_pprint(FILE *to, int nesting, const Block *block) {
    for (size_t i = 0; i < block->num_statements; i++) {
        statement_pprint(to, nesting, &block->statements[i]);
    }
}

void top_level_pprint(FILE *to, const TopLevel *top_level) {
    switch (top_level->tag) {
       case TOP_LEVEL_FUNC:
        expr_pprint(to, &top_level->func.ret_type);
        fprintf(to, " %s(", top_level->name);

        for (size_t i = 0; i < top_level->func.num_params; i++) {
            if (i > 0) {
                fprintf(to, ", ");
            }

            expr_pprint(to, &top_level->func.param_types[i]);
            if (top_level->func.param_names[i] != NULL) {
                fprintf(to, " %s", top_level->func.param_names[i]);
            }
        }
        fprintf(to, ") = \n    ");
        expr_pprint(to, &top_level->func.body);
        fprintf(to, ";\n");
        break;
    }
}

void translation_unit_pprint(FILE *to, const TranslationUnit *unit) {
    for (size_t i = 0; i < unit->num_top_levels; i++) {
        if (i > 0) {
            putc('\n', to);
        }

        top_level_pprint(to, &unit->top_levels[i]);
    }
}
