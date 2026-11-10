/*
 * ast.c - construcao incremental e destruicao da AST.
 *
 * As funcoes ast_*_append crescem vetores dinamicos (dobrando a capacidade,
 * com verificacao de overflow de size_t). As funcoes ast_*_destroy aceitam
 * NULL e liberam recursivamente toda a subarvore; a profundidade dessa
 * recursao e limitada porque o parser rejeita arvores mais altas que
 * PARSER_MAX_EXPRESSION_HEIGHT / PARSER_MAX_NESTING.
 */
#include "ast.h"

#include <stdint.h>
#include <stdlib.h>

#define AST_INITIAL_CAPACITY 4u

/* Dobra a capacidade de um vetor generico; false em overflow ou sem memoria. */
static bool grow_array(void **data, size_t *capacity, size_t element_size)
{
    size_t new_capacity;
    void *new_data;

    if (data == NULL || capacity == NULL || element_size == 0) {
        return false;
    }

    if (*capacity == 0) {
        new_capacity = AST_INITIAL_CAPACITY;
    } else {
        if (*capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity = *capacity * 2;
    }

    if (new_capacity > SIZE_MAX / element_size) {
        return false;
    }

    new_data = realloc(*data, new_capacity * element_size);
    if (new_data == NULL) {
        return false;
    }

    *data = new_data;
    *capacity = new_capacity;
    return true;
}

bool ast_program_append(AstProgram *program, AstTopLevelItem *item)
{
    if (program == NULL || item == NULL || program->count > program->capacity) {
        return false;
    }

    if (program->count == program->capacity &&
        !grow_array(
            (void **)&program->items,
            &program->capacity,
            sizeof(*program->items)
        )) {
        return false;
    }

    program->items[program->count++] = item;
    return true;
}

bool ast_record_append_field(AstRecordDecl *record, AstField *field)
{
    if (record == NULL || field == NULL || record->field_count > record->field_capacity) {
        return false;
    }

    if (record->field_count == record->field_capacity &&
        !grow_array(
            (void **)&record->fields,
            &record->field_capacity,
            sizeof(*record->fields)
        )) {
        return false;
    }

    record->fields[record->field_count++] = field;
    return true;
}

bool ast_declaration_append_declarator(
    AstDeclaration *declaration,
    AstDeclarator *declarator
)
{
    if (declaration == NULL || declarator == NULL ||
        declaration->declarator_count > declaration->declarator_capacity) {
        return false;
    }

    if (declaration->declarator_count == declaration->declarator_capacity &&
        !grow_array(
            (void **)&declaration->declarators,
            &declaration->declarator_capacity,
            sizeof(*declaration->declarators)
        )) {
        return false;
    }

    declaration->declarators[declaration->declarator_count++] = declarator;
    return true;
}

bool ast_function_append_parameter(
    AstFunctionDecl *function,
    AstParameter *parameter
)
{
    if (function == NULL || parameter == NULL ||
        function->parameter_count > function->parameter_capacity) {
        return false;
    }

    if (function->parameter_count == function->parameter_capacity &&
        !grow_array(
            (void **)&function->parameters,
            &function->parameter_capacity,
            sizeof(*function->parameters)
        )) {
        return false;
    }

    function->parameters[function->parameter_count++] = parameter;
    return true;
}

bool ast_block_append_statement(AstBlock *block, AstStatement *statement)
{
    if (block == NULL || statement == NULL ||
        block->statement_count > block->statement_capacity) {
        return false;
    }

    if (block->statement_count == block->statement_capacity &&
        !grow_array(
            (void **)&block->statements,
            &block->statement_capacity,
            sizeof(*block->statements)
        )) {
        return false;
    }

    block->statements[block->statement_count++] = statement;
    return true;
}

bool ast_if_append_else_if(AstIf *if_statement, AstIfBranch *branch)
{
    if (if_statement == NULL || branch == NULL ||
        if_statement->else_if_count > if_statement->else_if_capacity) {
        return false;
    }

    if (if_statement->else_if_count == if_statement->else_if_capacity &&
        !grow_array(
            (void **)&if_statement->else_if_branches,
            &if_statement->else_if_capacity,
            sizeof(*if_statement->else_if_branches)
        )) {
        return false;
    }

    if_statement->else_if_branches[if_statement->else_if_count++] = branch;
    return true;
}

bool ast_reference_append_argument(
    AstReference *reference,
    AstExpression *argument
)
{
    if (reference == NULL || argument == NULL ||
        reference->as.call.argument_count >
            reference->as.call.argument_capacity) {
        return false;
    }

    if (reference->as.call.argument_count ==
            reference->as.call.argument_capacity &&
        !grow_array(
            (void **)&reference->as.call.arguments,
            &reference->as.call.argument_capacity,
            sizeof(*reference->as.call.arguments)
        )) {
        return false;
    }

    reference->as.call.arguments[
        reference->as.call.argument_count++
    ] = argument;
    return true;
}

void ast_program_destroy(AstProgram *program)
{
    size_t i;

    if (program == NULL) {
        return;
    }

    for (i = 0; i < program->count; i++) {
        ast_top_level_item_destroy(program->items[i]);
    }
    free(program->items);
    free(program);
}

void ast_top_level_item_destroy(AstTopLevelItem *item)
{
    if (item == NULL) {
        return;
    }

    switch (item->kind) {
    case AST_TOP_RECORD:
        ast_record_decl_destroy(item->as.record);
        break;
    case AST_TOP_FUNCTION:
        ast_function_decl_destroy(item->as.function);
        break;
    case AST_TOP_DECLARATION:
        ast_declaration_destroy(item->as.declaration);
        break;
    }

    free(item);
}

void ast_record_decl_destroy(AstRecordDecl *record)
{
    size_t i;

    if (record == NULL) {
        return;
    }

    free(record->name);
    for (i = 0; i < record->field_count; i++) {
        ast_field_destroy(record->fields[i]);
    }
    free(record->fields);
    free(record);
}

void ast_field_destroy(AstField *field)
{
    if (field == NULL) {
        return;
    }

    free(field->name);
    free(field);
}

void ast_declarator_destroy(AstDeclarator *declarator)
{
    if (declarator == NULL) {
        return;
    }

    free(declarator->name);
    free(declarator->vector_size_lexeme);
    free(declarator);
}

void ast_declaration_destroy(AstDeclaration *declaration)
{
    size_t i;

    if (declaration == NULL) {
        return;
    }

    for (i = 0; i < declaration->declarator_count; i++) {
        ast_declarator_destroy(declaration->declarators[i]);
    }
    free(declaration->declarators);
    free(declaration->record_name);
    free(declaration);
}

void ast_parameter_destroy(AstParameter *parameter)
{
    if (parameter == NULL) {
        return;
    }

    free(parameter->name);
    free(parameter);
}

void ast_function_decl_destroy(AstFunctionDecl *function)
{
    size_t i;

    if (function == NULL) {
        return;
    }

    free(function->name);
    for (i = 0; i < function->parameter_count; i++) {
        ast_parameter_destroy(function->parameters[i]);
    }
    free(function->parameters);
    ast_block_destroy(function->body);
    free(function);
}

void ast_block_destroy(AstBlock *block)
{
    size_t i;

    if (block == NULL) {
        return;
    }

    for (i = 0; i < block->statement_count; i++) {
        ast_statement_destroy(block->statements[i]);
    }
    free(block->statements);
    free(block);
}

void ast_statement_destroy(AstStatement *statement)
{
    if (statement == NULL) {
        return;
    }

    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        ast_declaration_destroy(statement->as.declaration);
        break;
    case AST_STMT_ASSIGNMENT:
        ast_assignment_destroy(statement->as.assignment);
        break;
    case AST_STMT_IF:
        ast_if_destroy(statement->as.if_statement);
        break;
    case AST_STMT_WHILE:
        ast_while_destroy(statement->as.while_statement);
        break;
    case AST_STMT_REPEAT:
        ast_repeat_destroy(statement->as.repeat_statement);
        break;
    case AST_STMT_RETURN:
        ast_return_destroy(statement->as.return_statement);
        break;
    case AST_STMT_BLOCK:
        ast_block_destroy(statement->as.block);
        break;
    case AST_STMT_IO:
        ast_io_destroy(statement->as.io);
        break;
    case AST_STMT_CALL:
        ast_expression_destroy(statement->as.call);
        break;
    }

    free(statement);
}

void ast_io_destroy(AstIo *io)
{
    if (io == NULL) {
        return;
    }

    switch (io->kind) {
    case AST_IO_READ:
        ast_lvalue_destroy(io->as.target);
        break;
    case AST_IO_WRITE:
        ast_expression_destroy(io->as.value);
        break;
    }

    free(io);
}

void ast_lvalue_destroy(AstLValue *lvalue)
{
    if (lvalue == NULL) {
        return;
    }

    free(lvalue->name);
    free(lvalue->field_name);
    ast_expression_destroy(lvalue->index);
    free(lvalue);
}

void ast_assignment_destroy(AstAssignment *assignment)
{
    if (assignment == NULL) {
        return;
    }

    ast_lvalue_destroy(assignment->target);
    ast_expression_destroy(assignment->value);
    free(assignment);
}

void ast_if_branch_destroy(AstIfBranch *branch)
{
    if (branch == NULL) {
        return;
    }

    ast_expression_destroy(branch->condition);
    ast_block_destroy(branch->block);
    free(branch);
}

void ast_if_destroy(AstIf *if_statement)
{
    size_t i;

    if (if_statement == NULL) {
        return;
    }

    ast_expression_destroy(if_statement->condition);
    ast_block_destroy(if_statement->then_block);
    for (i = 0; i < if_statement->else_if_count; i++) {
        ast_if_branch_destroy(if_statement->else_if_branches[i]);
    }
    free(if_statement->else_if_branches);
    ast_block_destroy(if_statement->else_block);
    free(if_statement);
}

void ast_while_destroy(AstWhile *while_statement)
{
    if (while_statement == NULL) {
        return;
    }

    ast_expression_destroy(while_statement->condition);
    ast_block_destroy(while_statement->body);
    free(while_statement);
}

void ast_repeat_destroy(AstRepeat *repeat)
{
    if (repeat == NULL) {
        return;
    }

    ast_assignment_destroy(repeat->initialization);
    ast_expression_destroy(repeat->condition);
    ast_assignment_destroy(repeat->update);
    ast_block_destroy(repeat->body);
    free(repeat);
}

void ast_return_destroy(AstReturn *return_statement)
{
    if (return_statement == NULL) {
        return;
    }

    ast_expression_destroy(return_statement->value);
    free(return_statement);
}

static void destroy_reference_contents(AstReference *reference)
{
    size_t i;

    if (reference == NULL) {
        return;
    }

    free(reference->name);
    free(reference->field_name);
    switch (reference->kind) {
    case AST_REFERENCE_VARIABLE:
        break;
    case AST_REFERENCE_VECTOR:
        ast_expression_destroy(reference->as.vector.index);
        break;
    case AST_REFERENCE_CALL:
        for (i = 0; i < reference->as.call.argument_count; i++) {
            ast_expression_destroy(reference->as.call.arguments[i]);
        }
        free(reference->as.call.arguments);
        break;
    }
}

void ast_reference_destroy(AstReference *reference)
{
    if (reference == NULL) {
        return;
    }

    destroy_reference_contents(reference);
    free(reference);
}

void ast_expression_destroy(AstExpression *expression)
{
    if (expression == NULL) {
        return;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        free(expression->as.literal.lexeme);
        break;
    case AST_EXPR_REFERENCE:
        destroy_reference_contents(&expression->as.reference);
        break;
    case AST_EXPR_UNARY:
        ast_expression_destroy(expression->as.unary.operand);
        break;
    case AST_EXPR_BINARY:
        ast_expression_destroy(expression->as.binary.left);
        ast_expression_destroy(expression->as.binary.right);
        break;
    case AST_EXPR_ROOT:
        ast_expression_destroy(expression->as.root.argument);
        break;
    }

    free(expression);
}

static bool fits_i32(long long value)
{
    return value >= -2147483647LL - 1 && value <= 2147483647LL;
}

bool ast_constant_integer(const AstExpression *expression, long long *out)
{
    long long left;
    long long right;
    long long value = 0;
    size_t i;

    if (expression == NULL || out == NULL) {
        return false;
    }
    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind != AST_LITERAL_INTEGER ||
            expression->as.literal.lexeme == NULL) {
            return false;
        }
        for (i = 0; i < expression->as.literal.lexeme_length; i++) {
            char digit = expression->as.literal.lexeme[i];

            if (digit < '0' || digit > '9') {
                return false;
            }
            value = value * 10 + (digit - '0');
            if (value > 2147483647LL) {
                return false;
            }
        }
        *out = value;
        return true;
    case AST_EXPR_UNARY:
        if (expression->as.unary.op == AST_UNARY_NOT ||
            !ast_constant_integer(expression->as.unary.operand, &value)) {
            return false;
        }
        *out = expression->as.unary.op == AST_UNARY_MINUS ? -value : value;
        return fits_i32(*out);
    case AST_EXPR_BINARY:
        if (!ast_constant_integer(expression->as.binary.left, &left) ||
            !ast_constant_integer(expression->as.binary.right, &right)) {
            return false;
        }
        switch (expression->as.binary.op) {
        case AST_BINARY_ADD:
            value = left + right;
            break;
        case AST_BINARY_SUBTRACT:
            value = left - right;
            break;
        case AST_BINARY_MULTIPLY:
            value = left * right;
            break;
        case AST_BINARY_DIV_INT:
            if (right == 0) {
                return false;
            }
            value = left / right;
            break;
        default:
            return false;
        }
        if (!fits_i32(value)) {
            return false;
        }
        *out = value;
        return true;
    default:
        return false;
    }
}
