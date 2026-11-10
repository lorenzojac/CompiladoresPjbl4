#ifndef MACARRONICA_AST_H
#define MACARRONICA_AST_H

/*
 * Arvore sintatica abstrata (AST) da Macaronica.
 *
 * O parser constroi a AST e passa a ser dono de todos os nos; cada no tem um
 * destrutor ast_*_destroy que libera recursivamente seus filhos. A altura da
 * arvore e limitada pelo parser (ver PARSER_MAX_NESTING e
 * PARSER_MAX_EXPRESSION_HEIGHT em parser.h), o que torna seguras as
 * travessias recursivas feitas pela analise semantica, pelos destrutores e
 * pelo gerador de codigo.
 */

#include <stdbool.h>
#include <stddef.h>

typedef struct AstTopLevelItem AstTopLevelItem;
typedef struct AstProgram AstProgram;
typedef struct AstRecordDecl AstRecordDecl;
typedef struct AstField AstField;
typedef struct AstDeclarator AstDeclarator;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstParameter AstParameter;
typedef struct AstFunctionDecl AstFunctionDecl;
typedef struct AstStatement AstStatement;
typedef struct AstBlock AstBlock;
typedef struct AstLValue AstLValue;
typedef struct AstIo AstIo;
typedef struct AstAssignment AstAssignment;
typedef struct AstIfBranch AstIfBranch;
typedef struct AstIf AstIf;
typedef struct AstWhile AstWhile;
typedef struct AstRepeat AstRepeat;
typedef struct AstReturn AstReturn;
typedef struct AstReference AstReference;
typedef struct AstExpression AstExpression;

typedef struct {
    size_t line;
    size_t column;
} SourceLocation;

typedef enum {
    AST_TYPE_BOOL,
    AST_TYPE_INTEIRA,
    AST_TYPE_FLUT,
    AST_TYPE_PALAVRA,
    AST_TYPE_DUPLOCARPADO,
    AST_TYPE_VAZIO,
    /* Variavel de tipo registro (D39); so aparece em AstDeclaration. */
    AST_TYPE_REGISTRO
} AstTypeKind;

typedef struct {
    AstTypeKind kind;
    SourceLocation location;
} AstType;

/*
 * Tipo de um valor, anotado pela analise semantica e consumido pelo gerador
 * de codigo. Para vetores, kind e o tipo do elemento e vector_size > 0.
 * Para registros, kind == AST_TYPE_REGISTRO e record aponta a declaracao.
 */
typedef struct {
    AstTypeKind kind;
    size_t vector_size;
    const AstRecordDecl *record;
    bool valid;
} AstValueType;

/*
 * Funcoes embutidas registradas na tabela de simbolos (PjBL 4):
 * reducoes da DSL vetorial (D41) e E/S fisica da placa (D44).
 */
typedef enum {
    AST_BUILTIN_NONE,
    AST_BUILTIN_SOMAVET,
    AST_BUILTIN_MAXVET,
    AST_BUILTIN_MINVET,
    AST_BUILTIN_PRODESC,
    AST_BUILTIN_PINOMODO,
    AST_BUILTIN_PINOESCREVE,
    AST_BUILTIN_PINOLE,
    AST_BUILTIN_ESPERA
} AstBuiltin;

typedef enum {
    AST_TOP_RECORD,
    AST_TOP_FUNCTION,
    AST_TOP_DECLARATION
} AstTopLevelItemKind;

struct AstProgram {
    AstTopLevelItem **items;
    size_t count;
    size_t capacity;
};

struct AstTopLevelItem {
    AstTopLevelItemKind kind;
    SourceLocation location;
    union {
        AstRecordDecl *record;
        AstFunctionDecl *function;
        AstDeclaration *declaration;
    } as;
};

struct AstRecordDecl {
    SourceLocation location;
    char *name;
    AstField **fields;
    size_t field_count;
    size_t field_capacity;
};

struct AstField {
    SourceLocation location;
    AstType type;
    char *name;
};

typedef enum {
    AST_DECLARATOR_SIMPLE,
    AST_DECLARATOR_VECTOR
} AstDeclaratorKind;

struct AstDeclarator {
    AstDeclaratorKind kind;
    SourceLocation location;
    char *name;
    char *vector_size_lexeme;
    size_t vector_size_length;
    /* Anotacoes da semantica: identificador unico (> 0) e tamanho. */
    size_t symbol_id;
    size_t vector_size;
};

struct AstDeclaration {
    SourceLocation location;
    AstType type;
    /* type.kind == AST_TYPE_REGISTRO: nome do registro e sua declaracao. */
    char *record_name;
    const AstRecordDecl *record;
    AstDeclarator **declarators;
    size_t declarator_count;
    size_t declarator_capacity;
};

struct AstParameter {
    SourceLocation location;
    AstType type;
    char *name;
    size_t symbol_id;
};

struct AstFunctionDecl {
    SourceLocation location;
    bool is_main;
    char *name;
    AstType return_type;
    AstParameter **parameters;
    size_t parameter_count;
    size_t parameter_capacity;
    AstBlock *body;
};

typedef enum {
    AST_STMT_DECLARATION,
    AST_STMT_ASSIGNMENT,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_REPEAT,
    AST_STMT_RETURN,
    AST_STMT_BLOCK,
    AST_STMT_IO,
    /* Chamada isolada, "f(x);" (D33). O valor de retorno e descartado. */
    AST_STMT_CALL
} AstStatementKind;

typedef enum {
    AST_IO_READ,
    AST_IO_WRITE
} AstIoKind;

struct AstBlock {
    SourceLocation location;
    AstStatement **statements;
    size_t statement_count;
    size_t statement_capacity;
};

struct AstStatement {
    AstStatementKind kind;
    SourceLocation location;
    union {
        AstDeclaration *declaration;
        AstAssignment *assignment;
        AstIf *if_statement;
        AstWhile *while_statement;
        AstRepeat *repeat_statement;
        AstReturn *return_statement;
        AstBlock *block;
        AstIo *io;
        /* AST_STMT_CALL: expressao AST_EXPR_REFERENCE de tipo CALL. */
        AstExpression *call;
    } as;
};

typedef enum {
    AST_LVALUE_VARIABLE,
    AST_LVALUE_VECTOR
} AstLValueKind;

struct AstLValue {
    AstLValueKind kind;
    SourceLocation location;
    char *name;
    AstExpression *index;
    /* Acesso a campo opcional: "p.x" ou "v{i}.x" (D39). */
    char *field_name;
    /* Anotacoes da semantica. */
    size_t symbol_id;
    size_t field_index;
    AstValueType type;
};

struct AstAssignment {
    SourceLocation location;
    AstLValue *target;
    AstExpression *value;
};

struct AstIo {
    AstIoKind kind;
    SourceLocation location;
    union {
        AstLValue *target;
        AstExpression *value;
    } as;
};

struct AstIfBranch {
    SourceLocation location;
    AstExpression *condition;
    AstBlock *block;
};

struct AstIf {
    SourceLocation location;
    AstExpression *condition;
    AstBlock *then_block;
    AstIfBranch **else_if_branches;
    size_t else_if_count;
    size_t else_if_capacity;
    AstBlock *else_block;
};

struct AstWhile {
    SourceLocation location;
    AstExpression *condition;
    AstBlock *body;
};

struct AstRepeat {
    SourceLocation location;
    AstAssignment *initialization;
    AstExpression *condition;
    AstAssignment *update;
    AstBlock *body;
};

struct AstReturn {
    SourceLocation location;
    AstExpression *value;
};

typedef enum {
    AST_LITERAL_INTEGER,
    AST_LITERAL_DECIMAL,
    AST_LITERAL_TEXT,
    AST_LITERAL_BOOLEAN
} AstLiteralKind;

typedef struct {
    AstLiteralKind kind;
    char *lexeme;
    size_t lexeme_length;
} AstLiteral;

typedef enum {
    AST_REFERENCE_VARIABLE,
    AST_REFERENCE_VECTOR,
    AST_REFERENCE_CALL
} AstReferenceKind;

struct AstReference {
    AstReferenceKind kind;
    SourceLocation location;
    char *name;
    union {
        struct {
            AstExpression *index;
        } vector;
        struct {
            AstExpression **arguments;
            size_t argument_count;
            size_t argument_capacity;
        } call;
    } as;
    /* Acesso a campo opcional em VARIABLE/VECTOR: "p.x", "v{i}.x" (D39). */
    char *field_name;
    /* Anotacoes da semantica. */
    size_t symbol_id;                /* VARIABLE/VECTOR */
    size_t field_index;              /* com field_name */
    const AstFunctionDecl *callee;   /* CALL de funcao do programa */
    AstBuiltin builtin;              /* CALL de funcao embutida */
};

typedef enum {
    AST_EXPR_LITERAL,
    AST_EXPR_REFERENCE,
    AST_EXPR_UNARY,
    AST_EXPR_BINARY,
    AST_EXPR_ROOT
} AstExpressionKind;

typedef enum {
    AST_UNARY_PLUS,
    AST_UNARY_MINUS,
    AST_UNARY_NOT
} AstUnaryOp;

typedef enum {
    AST_BINARY_ADD,
    AST_BINARY_SUBTRACT,
    AST_BINARY_MULTIPLY,
    AST_BINARY_DIVIDE,
    AST_BINARY_DIV_INT,
    AST_BINARY_POWER,
    AST_BINARY_LESS,
    AST_BINARY_GREATER,
    AST_BINARY_EQUAL,
    AST_BINARY_DIFFERENT,
    AST_BINARY_AND,
    AST_BINARY_XOR,
    AST_BINARY_OR
} AstBinaryOp;

struct AstExpression {
    AstExpressionKind kind;
    SourceLocation location;
    /*
     * Altura da subarvore (folha = 1), calculada pelo parser. Serve para
     * rejeitar expressoes patologicamente profundas antes que qualquer
     * travessia recursiva estoure a pilha (D35).
     */
    size_t height;
    /* Tipo do valor, anotado pela analise semantica. */
    AstValueType type;
    union {
        AstLiteral literal;
        AstReference reference;
        struct {
            AstUnaryOp op;
            AstExpression *operand;
        } unary;
        struct {
            AstBinaryOp op;
            AstExpression *left;
            AstExpression *right;
        } binary;
        struct {
            AstExpression *argument;
        } root;
    } as;
};

bool ast_program_append(AstProgram *program, AstTopLevelItem *item);
bool ast_record_append_field(AstRecordDecl *record, AstField *field);
bool ast_declaration_append_declarator(
    AstDeclaration *declaration,
    AstDeclarator *declarator
);
bool ast_function_append_parameter(
    AstFunctionDecl *function,
    AstParameter *parameter
);
bool ast_block_append_statement(AstBlock *block, AstStatement *statement);
bool ast_if_append_else_if(AstIf *if_statement, AstIfBranch *branch);
bool ast_reference_append_argument(
    AstReference *reference,
    AstExpression *argument
);

/*
 * Avalia em tempo de compilacao uma expressao inteira formada apenas por
 * literais inteiros, parenteses, + e - unarios e + - * // binarios (D40).
 * Devolve false se a expressao nao e constante, se algum resultado
 * intermediario sai do intervalo de inteira (i32) ou se ha divisao por zero.
 */
bool ast_constant_integer(const AstExpression *expression, long long *out);

void ast_program_destroy(AstProgram *program);
void ast_top_level_item_destroy(AstTopLevelItem *item);
void ast_record_decl_destroy(AstRecordDecl *record);
void ast_field_destroy(AstField *field);
void ast_declarator_destroy(AstDeclarator *declarator);
void ast_declaration_destroy(AstDeclaration *declaration);
void ast_parameter_destroy(AstParameter *parameter);
void ast_function_decl_destroy(AstFunctionDecl *function);
void ast_block_destroy(AstBlock *block);
void ast_statement_destroy(AstStatement *statement);
void ast_io_destroy(AstIo *io);
void ast_lvalue_destroy(AstLValue *lvalue);
void ast_assignment_destroy(AstAssignment *assignment);
void ast_if_branch_destroy(AstIfBranch *branch);
void ast_if_destroy(AstIf *if_statement);
void ast_while_destroy(AstWhile *while_statement);
void ast_repeat_destroy(AstRepeat *repeat);
void ast_return_destroy(AstReturn *return_statement);
void ast_reference_destroy(AstReference *reference);
void ast_expression_destroy(AstExpression *expression);

#endif
