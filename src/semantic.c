/*
 * semantic.c - analise semantica (nomes, escopos e tipos).
 *
 * Organizacao do arquivo:
 *   1. diagnosticos;
 *   2. tabela de simbolos: uma pilha de escopos (global -> funcao -> blocos);
 *      cada escopo guarda seus simbolos em ordem de declaracao e um indice
 *      hash (enderecamento aberto, sondagem linear) para busca O(1) media;
 *   3. regras de tipos (D15-D23): promocao inteira < flut < duplocarpado;
 *   4. passada 1: registros, globais e assinaturas (collect_globals);
 *   5. passada 2: corpos de funcao (analyze_function e analyze_*);
 *   6. analise de fluxo de retorno (D34).
 *
 * A AST nunca e modificada aqui; os simbolos apontam para nomes da AST.
 */
#include "semantic.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SEMANTIC_INITIAL_CAPACITY 4u
#define SEMANTIC_INITIAL_INDEX_CAPACITY 16u

typedef enum {
    SEM_SYMBOL_VARIABLE,
    SEM_SYMBOL_VECTOR,
    SEM_SYMBOL_PARAMETER,
    SEM_SYMBOL_FUNCTION,
    SEM_SYMBOL_RECORD
} SemanticSymbolKind;

typedef struct SemanticSymbol SemanticSymbol;

struct SemanticSymbol {
    const char *name;
    SemanticSymbolKind kind;
    SemanticType type;
    SourceLocation location;
    size_t vector_size;
    const AstFunctionDecl *function;
    /*
     * SEM_SYMBOL_RECORD: a propria definicao. Variavel/vetor com
     * type == SEM_TYPE_REGISTRO: o registro que e seu tipo (D39).
     */
    const AstRecordDecl *record;
    /* Identificador unico de variaveis, vetores e parametros (> 0). */
    size_t id;
    /* Funcao embutida (D41, D44); function == NULL nesse caso. */
    AstBuiltin builtin;
};

struct SemanticScope {
    SemanticScope *parent;
    /* Simbolos na ordem de declaracao (dono da memoria de cada simbolo). */
    SemanticSymbol **symbols;
    size_t count;
    size_t capacity;
    /*
     * Indice hash: cada posicao guarda (posicao em symbols + 1), ou 0 se
     * vazia. index_capacity e potencia de 2 e mantida com carga <= 50%.
     */
    size_t *index;
    size_t index_capacity;
};

static const char *const MESSAGE_UNDECLARED = "identificador nao declarado";
static const char *const MESSAGE_DUPLICATE = "declaracao duplicada";
static const char *const MESSAGE_NAME_KIND = "categoria de nome invalida";
static const char *const MESSAGE_ASSIGNMENT = "atribuicao incompativel";
static const char *const MESSAGE_NOT_VECTOR = "nome nao e vetor";
static const char *const MESSAGE_VECTOR_INDEX =
    "indice de vetor deve ser inteira";
static const char *const MESSAGE_VECTOR_SIZE =
    "tamanho de vetor deve ser inteiro positivo";
static const char *const MESSAGE_UNKNOWN_FUNCTION =
    "funcao nao declarada";
static const char *const MESSAGE_NOT_FUNCTION = "nome nao e funcao";
static const char *const MESSAGE_ARGUMENT_COUNT =
    "quantidade de argumentos invalida";
static const char *const MESSAGE_ARGUMENT_TYPE =
    "tipo de argumento incompativel";
static const char *const MESSAGE_VOID_VALUE =
    "funcao vazio nao produz valor";
static const char *const MESSAGE_INVALID_RETURN = "retorno invalido";
static const char *const MESSAGE_INVALID_CONDITION =
    "condicao deve ser bool";
static const char *const MESSAGE_INVALID_OPERATOR = "operador invalido";
static const char *const MESSAGE_INVALID_RECORD =
    "declaracao de registro invalida";
static const char *const MESSAGE_INVALID_PRINCIPAL =
    "o programa deve possuir exatamente uma principal";
static const char *const MESSAGE_MISSING_RETURN =
    "funcao nao-vazio pode terminar sem respost em algum caminho";
static const char *const MESSAGE_ASSIGNMENT_DIVISION =
    "atribuicao incompativel: '/' sempre produz valor real (D18, D38); "
    "para divisao inteira use //";
static const char *const MESSAGE_RETURN_DIVISION =
    "retorno invalido: '/' sempre produz valor real (D18, D38); "
    "para divisao inteira use //";
static const char *const MESSAGE_UNKNOWN_RECORD =
    "tipo registro nao declarado";
static const char *const MESSAGE_NOT_RECORD = "nome nao e registro";
static const char *const MESSAGE_VECTOR_AS_SCALAR =
    "vetor sem indice so pode ser usado em operacao vetorial (DSL)";
static const char *const MESSAGE_VECTOR_CONTEXT =
    "expressao vetorial so pode ser atribuida a um vetor ou reduzida "
    "(somavet, maxvet, minvet, prodesc)";
static const char *const MESSAGE_RECORD_AS_SCALAR =
    "registro inteiro so pode ser atribuido a outro registro do mesmo tipo";
static const char *const MESSAGE_VECTOR_SIZE_MISMATCH =
    "vetores de tamanhos diferentes na operacao vetorial";
static const char *const MESSAGE_VECTOR_OPERATION =
    "operacao vetorial invalida: a DSL aceita +, - e * entre vetores e "
    "escalares numericos";
static const char *const MESSAGE_VECTOR_ELEMENT =
    "operacao vetorial exige vetores de elementos numericos";
static const char *const MESSAGE_VECTOR_ARGUMENT =
    "argumento deve ser vetor de elementos numericos";
static const char *const MESSAGE_NOT_A_RECORD_VALUE =
    "acesso a campo em valor que nao e registro";
static const char *const MESSAGE_UNKNOWN_FIELD = "registro nao possui este campo";
static const char *const MESSAGE_FIELD_ON_VECTOR =
    "acesso a campo exige elemento indexado: use v{i}.campo";
static const char *const MESSAGE_INDEX_BOUNDS =
    "indice constante fora dos limites do vetor";
static const char *const MESSAGE_DIVISION_ZERO = "divisao por zero constante";
static const char *const MESSAGE_LITERAL_RANGE =
    "literal fora do intervalo do tipo (inteira e i32)";
static const char *const MESSAGE_READ_SCALAR =
    "le exige variavel escalar ou elemento de vetor";
static const char *const MESSAGE_TARGET_GLOBALS_UNO =
    "variaveis globais e textos excedem a RAM do alvo uno "
    "(2048 bytes, 512 reservados para pilha e biblioteca)";
static const char *const MESSAGE_TARGET_FRAME_UNO =
    "variaveis locais desta funcao somadas as globais excedem a RAM do "
    "alvo uno (2048 bytes, 512 reservados para pilha e biblioteca)";
static const char *const MESSAGE_TARGET_GLOBALS_PICO =
    "variaveis globais e textos excedem a RAM do alvo pico "
    "(524288 bytes, 65536 reservados para SDK, USB e pilha)";
static const char *const MESSAGE_TARGET_FRAME_PICO =
    "variaveis locais desta funcao somadas as globais excedem a RAM do "
    "alvo pico (524288 bytes, 65536 reservados para SDK, USB e pilha)";

typedef enum {
    VECTOR_SIZE_VALID,
    VECTOR_SIZE_INVALID_VALUE,
    VECTOR_SIZE_INVALID_AST
} VectorSizeStatus;

static VectorSizeStatus parse_vector_size(
    const AstDeclarator *declarator,
    size_t *out_size
);
static SemanticType semantic_type_from_ast(
    SemanticAnalyzer *analyzer,
    AstTypeKind kind
);
static void destroy_scope(SemanticScope *scope);
static void destroy_diagnostics(
    SemanticDiagnostic *diagnostics,
    size_t count
);

static void mark_fatal(SemanticAnalyzer *analyzer)
{
    if (analyzer != NULL) {
        analyzer->fatal = true;
    }
}

static bool grow_diagnostics(SemanticAnalyzer *analyzer)
{
    size_t new_capacity;
    SemanticDiagnostic *new_diagnostics;

    if (analyzer == NULL ||
        analyzer->diagnostic_count > analyzer->diagnostic_capacity) {
        return false;
    }

    if (analyzer->diagnostic_capacity == 0) {
        new_capacity = SEMANTIC_INITIAL_CAPACITY;
    } else {
        if (analyzer->diagnostic_capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity = analyzer->diagnostic_capacity * 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*analyzer->diagnostics)) {
        return false;
    }

    new_diagnostics = realloc(
        analyzer->diagnostics,
        new_capacity * sizeof(*analyzer->diagnostics)
    );
    if (new_diagnostics == NULL) {
        return false;
    }

    analyzer->diagnostics = new_diagnostics;
    analyzer->diagnostic_capacity = new_capacity;
    return true;
}

static bool append_diagnostic(
    SemanticAnalyzer *analyzer,
    SemanticDiagnostic *diagnostic
)
{
    if (analyzer == NULL || diagnostic == NULL ||
        analyzer->diagnostic_count > analyzer->diagnostic_capacity) {
        return false;
    }

    if (analyzer->diagnostic_count == analyzer->diagnostic_capacity &&
        !grow_diagnostics(analyzer)) {
        return false;
    }

    analyzer->diagnostics[analyzer->diagnostic_count++] = *diagnostic;
    return true;
}

static bool copy_bytes(
    const char *source,
    size_t length,
    char **out_copy
)
{
    char *copy;

    if (out_copy == NULL || length == SIZE_MAX ||
        (length > 0 && source == NULL)) {
        return false;
    }

    copy = malloc(length + 1);
    if (copy == NULL) {
        return false;
    }

    if (length > 0) {
        memcpy(copy, source, length);
    }
    copy[length] = '\0';
    *out_copy = copy;
    return true;
}

static bool add_diagnostic(
    SemanticAnalyzer *analyzer,
    SemanticDiagnosticCode code,
    SourceLocation location,
    const char *symbol_name,
    const char *message
)
{
    SemanticDiagnostic diagnostic = {0};

    if (analyzer == NULL || analyzer->fatal) {
        return false;
    }

    diagnostic.code = code;
    diagnostic.location = location;
    diagnostic.message = message;

    if (symbol_name != NULL) {
        size_t length = strlen(symbol_name);

        diagnostic.symbol_name_length = length;
        if (!copy_bytes(
                symbol_name,
                length,
                &diagnostic.symbol_name
            )) {
            mark_fatal(analyzer);
            return false;
        }
    }

    if (!append_diagnostic(analyzer, &diagnostic)) {
        free(diagnostic.symbol_name);
        mark_fatal(analyzer);
        return false;
    }
    return true;
}

static SemanticScope *create_scope(SemanticScope *parent)
{
    SemanticScope *scope = calloc(1, sizeof(*scope));

    if (scope != NULL) {
        scope->parent = parent;
    }
    return scope;
}

static bool grow_scope_symbols(SemanticScope *scope)
{
    size_t new_capacity;
    SemanticSymbol **new_symbols;

    if (scope == NULL || scope->count > scope->capacity) {
        return false;
    }

    if (scope->capacity == 0) {
        new_capacity = SEMANTIC_INITIAL_CAPACITY;
    } else {
        if (scope->capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity = scope->capacity * 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*scope->symbols)) {
        return false;
    }

    new_symbols = realloc(
        scope->symbols,
        new_capacity * sizeof(*scope->symbols)
    );
    if (new_symbols == NULL) {
        return false;
    }

    scope->symbols = new_symbols;
    scope->capacity = new_capacity;
    return true;
}

/* FNV-1a de 64 bits truncado para size_t. */
static size_t hash_name(const char *name)
{
    unsigned long long hash = 1469598103934665603ULL;

    while (*name != '\0') {
        hash ^= (unsigned char)*name++;
        hash *= 1099511628211ULL;
    }
    return (size_t)hash;
}

/* Insere a posicao "position" de symbols no indice (sem verificar carga). */
static void index_insert(SemanticScope *scope, size_t position)
{
    size_t mask = scope->index_capacity - 1;
    size_t slot = hash_name(scope->symbols[position]->name) & mask;

    while (scope->index[slot] != 0) {
        slot = (slot + 1) & mask;
    }
    scope->index[slot] = position + 1;
}

/* Reconstroi o indice com o dobro da capacidade. */
static bool grow_scope_index(SemanticScope *scope)
{
    size_t new_capacity;
    size_t *new_index;
    size_t i;

    if (scope->index_capacity == 0) {
        new_capacity = SEMANTIC_INITIAL_INDEX_CAPACITY;
    } else {
        if (scope->index_capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity = scope->index_capacity * 2;
    }
    if (new_capacity > SIZE_MAX / sizeof(*new_index)) {
        return false;
    }
    new_index = calloc(new_capacity, sizeof(*new_index));
    if (new_index == NULL) {
        return false;
    }

    free(scope->index);
    scope->index = new_index;
    scope->index_capacity = new_capacity;
    for (i = 0; i < scope->count; i++) {
        index_insert(scope, i);
    }
    return true;
}

static bool append_symbol(
    SemanticScope *scope,
    SemanticSymbol *symbol
)
{
    if (scope == NULL || symbol == NULL || symbol->name == NULL ||
        scope->count > scope->capacity) {
        return false;
    }

    if (scope->count == scope->capacity &&
        !grow_scope_symbols(scope)) {
        return false;
    }
    /* Mantem a carga do indice em no maximo 50% (count + 1 <= cap / 2). */
    if (scope->count >= scope->index_capacity / 2 &&
        !grow_scope_index(scope)) {
        return false;
    }

    scope->symbols[scope->count] = symbol;
    index_insert(scope, scope->count);
    scope->count++;
    return true;
}

static SemanticSymbol *create_symbol(
    const char *name,
    SemanticSymbolKind kind,
    SemanticType type,
    SourceLocation location
)
{
    SemanticSymbol *symbol = calloc(1, sizeof(*symbol));

    if (symbol == NULL) {
        return NULL;
    }

    symbol->name = name;
    symbol->kind = kind;
    symbol->type = type;
    symbol->location = location;
    return symbol;
}

/* Busca apenas no escopo dado, pelo indice hash. */
static SemanticSymbol *lookup_current_scope(
    const SemanticScope *scope,
    const char *name
)
{
    size_t mask;
    size_t slot;

    if (scope == NULL || name == NULL || scope->index_capacity == 0) {
        return NULL;
    }

    mask = scope->index_capacity - 1;
    slot = hash_name(name) & mask;
    while (scope->index[slot] != 0) {
        SemanticSymbol *symbol = scope->symbols[scope->index[slot] - 1];

        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        slot = (slot + 1) & mask;
    }
    return NULL;
}

/* Busca do escopo atual ate o global; o simbolo mais interno vence. */
static SemanticSymbol *lookup_visible(
    const SemanticScope *scope,
    const char *name
)
{
    const SemanticScope *current = scope;

    while (current != NULL) {
        SemanticSymbol *symbol = lookup_current_scope(current, name);

        if (symbol != NULL) {
            return symbol;
        }
        current = current->parent;
    }
    return NULL;
}

/*
 * Declara um nome no escopo atual. Mesmo nome no mesmo escopo e
 * SEM_DUPLICATE_DECLARATION (D25); em escopo filho e shadowing valido.
 * Variaveis, funcoes e registros compartilham o mesmo namespace.
 */
static bool declare_symbol(
    SemanticAnalyzer *analyzer,
    const char *name,
    SemanticSymbolKind kind,
    SemanticType type,
    SourceLocation location,
    size_t vector_size,
    const AstFunctionDecl *function,
    const AstRecordDecl *record
)
{
    SemanticSymbol *symbol;

    if (analyzer == NULL || analyzer->current_scope == NULL ||
        name == NULL) {
        mark_fatal(analyzer);
        return false;
    }
    analyzer->last_symbol_id = 0;

    if (lookup_current_scope(analyzer->current_scope, name) != NULL) {
        add_diagnostic(
            analyzer,
            SEM_DUPLICATE_DECLARATION,
            location,
            name,
            MESSAGE_DUPLICATE
        );
        return !analyzer->fatal;
    }

    symbol = create_symbol(name, kind, type, location);
    if (symbol == NULL) {
        mark_fatal(analyzer);
        return false;
    }
    symbol->vector_size = vector_size;
    symbol->function = function;
    symbol->record = record;
    if (kind == SEM_SYMBOL_VARIABLE || kind == SEM_SYMBOL_VECTOR ||
        kind == SEM_SYMBOL_PARAMETER) {
        symbol->id = ++analyzer->next_symbol_id;
        analyzer->last_symbol_id = symbol->id;
    }

    if (!append_symbol(analyzer->current_scope, symbol)) {
        free(symbol);
        mark_fatal(analyzer);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Anotacao da AST e utilitarios de tipo                               */
/* ------------------------------------------------------------------ */

/*
 * A analise recebe a AST como const porque nao altera sua estrutura; as
 * anotacoes (tipos, ids) sao campos reservados para isso, escritos apenas
 * por estas funcoes. Os nos sao alocados pelo parser (nunca const de fato).
 */
static AstTypeKind ast_kind_of(SemanticType type)
{
    switch (type) {
    case SEM_TYPE_BOOL:
        return AST_TYPE_BOOL;
    case SEM_TYPE_INTEIRA:
        return AST_TYPE_INTEIRA;
    case SEM_TYPE_FLUT:
        return AST_TYPE_FLUT;
    case SEM_TYPE_PALAVRA:
        return AST_TYPE_PALAVRA;
    case SEM_TYPE_DUPLOCARPADO:
        return AST_TYPE_DUPLOCARPADO;
    case SEM_TYPE_REGISTRO:
        return AST_TYPE_REGISTRO;
    default:
        return AST_TYPE_VAZIO;
    }
}

static AstValueType value_type_make(
    SemanticType type,
    size_t vector_size,
    const AstRecordDecl *record
)
{
    AstValueType value = {0};

    value.kind = ast_kind_of(type);
    value.vector_size = vector_size;
    value.record = record;
    value.valid = type != SEM_TYPE_ERROR;
    return value;
}

static void annotate_expression(
    const AstExpression *expression,
    AstValueType type
)
{
    ((AstExpression *)expression)->type = type;
}

static void annotate_declarator(
    const AstDeclarator *declarator,
    size_t symbol_id,
    size_t vector_size
)
{
    ((AstDeclarator *)declarator)->symbol_id = symbol_id;
    ((AstDeclarator *)declarator)->vector_size = vector_size;
}

/*
 * Tamanho em bytes no alvo, sem alinhamento: ponteiro (palavra) de 16 bits
 * no Uno (AVR) e de 32 bits na Pico (ARM). Estimativa para a verificacao
 * de memoria (D45, D48).
 */
static size_t target_pointer_bytes(SemanticTarget target)
{
    return target == SEMANTIC_TARGET_UNO ? 2 : 4;
}

static size_t target_scalar_size(AstTypeKind kind, size_t pointer_bytes)
{
    switch (kind) {
    case AST_TYPE_BOOL:
        return 1;
    case AST_TYPE_PALAVRA:
        return pointer_bytes;
    case AST_TYPE_DUPLOCARPADO:
        return 8;
    case AST_TYPE_INTEIRA:
    case AST_TYPE_FLUT:
        return 4;
    default:
        return 0;
    }
}

static size_t saturating_add(size_t a, size_t b)
{
    return a > SIZE_MAX - b ? SIZE_MAX : a + b;
}

static size_t saturating_mul(size_t a, size_t b)
{
    return (b != 0 && a > SIZE_MAX / b) ? SIZE_MAX : a * b;
}

static size_t target_value_size(
    AstTypeKind kind,
    const AstRecordDecl *record,
    size_t vector_size,
    size_t pointer_bytes
)
{
    size_t element = 0;
    size_t i;

    if (kind == AST_TYPE_REGISTRO) {
        if (record != NULL) {
            for (i = 0; i < record->field_count; i++) {
                if (record->fields[i] != NULL) {
                    element = saturating_add(
                        element,
                        target_scalar_size(
                            record->fields[i]->type.kind,
                            pointer_bytes
                        )
                    );
                }
            }
        }
    } else {
        element = target_scalar_size(kind, pointer_bytes);
    }
    return vector_size > 0 ? saturating_mul(element, vector_size) : element;
}

/*
 * Resolve o registro de uma declaracao "registro R x;" (D39). Registros
 * sao sempre globais. Devolve NULL (com diagnostico) se R nao e registro.
 */
static const AstRecordDecl *resolve_record_type(
    SemanticAnalyzer *analyzer,
    const AstDeclaration *declaration
)
{
    SemanticSymbol *symbol;

    if (declaration->record_name == NULL) {
        mark_fatal(analyzer);
        return NULL;
    }
    symbol = lookup_current_scope(
        analyzer->global_scope,
        declaration->record_name
    );
    if (symbol == NULL) {
        add_diagnostic(
            analyzer,
            SEM_UNDECLARED_IDENTIFIER,
            declaration->type.location,
            declaration->record_name,
            MESSAGE_UNKNOWN_RECORD
        );
        return NULL;
    }
    if (symbol->kind != SEM_SYMBOL_RECORD) {
        add_diagnostic(
            analyzer,
            SEM_NAME_KIND_MISMATCH,
            declaration->type.location,
            declaration->record_name,
            MESSAGE_NOT_RECORD
        );
        return NULL;
    }
    ((AstDeclaration *)declaration)->record = symbol->record;
    return symbol->record;
}

/*
 * Declara todos os declaradores de uma declaracao (global ou local),
 * anotando symbol_id e tamanho de vetor. bytes recebe a soma dos tamanhos
 * no alvo, para a verificacao de memoria (D45).
 */
static bool declare_declarators(
    SemanticAnalyzer *analyzer,
    const AstDeclaration *declaration,
    size_t *bytes
)
{
    SemanticType type;
    const AstRecordDecl *record = NULL;
    size_t i;

    if (analyzer == NULL || declaration == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    if (declaration->type.kind == AST_TYPE_REGISTRO) {
        type = SEM_TYPE_REGISTRO;
        record = resolve_record_type(analyzer, declaration);
        if (analyzer->fatal) {
            return false;
        }
        if (record == NULL) {
            /* Erro ja relatado: declara como erro para evitar cascata. */
            type = SEM_TYPE_ERROR;
        }
    } else {
        type = semantic_type_from_ast(analyzer, declaration->type.kind);
        if (type == SEM_TYPE_ERROR || type == SEM_TYPE_VAZIO) {
            mark_fatal(analyzer);
            return false;
        }
    }
    if (declaration->declarator_count > 0 &&
        declaration->declarators == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    for (i = 0; i < declaration->declarator_count; i++) {
        const AstDeclarator *declarator = declaration->declarators[i];
        size_t vector_size = 0;

        if (declarator == NULL || declarator->name == NULL) {
            mark_fatal(analyzer);
            return false;
        }
        if (declarator->kind == AST_DECLARATOR_VECTOR) {
            VectorSizeStatus vector_status = parse_vector_size(
                declarator,
                &vector_size
            );

            if (vector_status == VECTOR_SIZE_INVALID_AST) {
                mark_fatal(analyzer);
                return false;
            }
            if (vector_status == VECTOR_SIZE_INVALID_VALUE) {
                add_diagnostic(
                    analyzer,
                    SEM_INVALID_VECTOR_SIZE,
                    declarator->location,
                    declarator->name,
                    MESSAGE_VECTOR_SIZE
                );
                vector_size = 0;
            }
        } else if (declarator->kind != AST_DECLARATOR_SIMPLE) {
            mark_fatal(analyzer);
            return false;
        }

        if (!declare_symbol(
                analyzer,
                declarator->name,
                declarator->kind == AST_DECLARATOR_VECTOR
                    ? SEM_SYMBOL_VECTOR
                    : SEM_SYMBOL_VARIABLE,
                type,
                declarator->location,
                vector_size,
                NULL,
                record
            )) {
            return false;
        }
        annotate_declarator(
            declarator,
            analyzer->last_symbol_id,
            vector_size
        );
        *bytes = saturating_add(
            *bytes,
            target_value_size(
                declaration->type.kind,
                record,
                vector_size,
                target_pointer_bytes(analyzer->target)
            )
        );
    }
    return true;
}

/* Abre um escopo filho do atual (corpo de funcao ou bloco aninhado). */
static bool scope_push(SemanticAnalyzer *analyzer)
{
    SemanticScope *scope;

    if (analyzer == NULL || analyzer->current_scope == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    scope = create_scope(analyzer->current_scope);
    if (scope == NULL) {
        mark_fatal(analyzer);
        return false;
    }
    analyzer->current_scope = scope;
    return true;
}

static bool scope_pop(SemanticAnalyzer *analyzer)
{
    SemanticScope *scope;
    SemanticScope *parent;

    if (analyzer == NULL || analyzer->current_scope == NULL ||
        analyzer->current_scope == analyzer->global_scope) {
        mark_fatal(analyzer);
        return false;
    }

    scope = analyzer->current_scope;
    parent = scope->parent;
    analyzer->current_scope = parent;
    destroy_scope(scope);
    return true;
}

static void destroy_scope(SemanticScope *scope)
{
    size_t i;

    if (scope == NULL) {
        return;
    }

    for (i = 0; i < scope->count; i++) {
        free(scope->symbols[i]);
    }
    free(scope->symbols);
    free(scope->index);
    free(scope);
}

static bool is_numeric(SemanticType type)
{
    return type == SEM_TYPE_INTEIRA ||
           type == SEM_TYPE_FLUT ||
           type == SEM_TYPE_DUPLOCARPADO;
}

static int numeric_rank(SemanticType type)
{
    switch (type) {
    case SEM_TYPE_INTEIRA:
        return 0;
    case SEM_TYPE_FLUT:
        return 1;
    case SEM_TYPE_DUPLOCARPADO:
        return 2;
    default:
        return -1;
    }
}


/* Tipo resultante de + - * ^ entre numericos: o de maior rank (D17). */
static SemanticType wider_numeric_type(
    SemanticType left,
    SemanticType right
)
{
    if (!is_numeric(left) || !is_numeric(right)) {
        return SEM_TYPE_ERROR;
    }
    return numeric_rank(left) >= numeric_rank(right) ? left : right;
}

/*
 * Matriz de compatibilidade unica para atribuicao, argumento e retorno
 * (D16): tipos iguais, ou promocao numerica de rank menor para maior.
 * vazio nunca e valor.
 */
static bool is_assignable(
    SemanticType destination,
    SemanticType source
)
{
    if (destination == SEM_TYPE_ERROR || source == SEM_TYPE_ERROR ||
        destination == SEM_TYPE_VAZIO || source == SEM_TYPE_VAZIO) {
        return false;
    }

    if (destination == source) {
        return true;
    }

    return is_numeric(destination) &&
           is_numeric(source) &&
           numeric_rank(source) <= numeric_rank(destination);
}

static SemanticType semantic_type_from_ast(
    SemanticAnalyzer *analyzer,
    AstTypeKind kind
)
{
    switch (kind) {
    case AST_TYPE_BOOL:
        return SEM_TYPE_BOOL;
    case AST_TYPE_INTEIRA:
        return SEM_TYPE_INTEIRA;
    case AST_TYPE_FLUT:
        return SEM_TYPE_FLUT;
    case AST_TYPE_PALAVRA:
        return SEM_TYPE_PALAVRA;
    case AST_TYPE_DUPLOCARPADO:
        return SEM_TYPE_DUPLOCARPADO;
    case AST_TYPE_VAZIO:
        return SEM_TYPE_VAZIO;
    default:
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
}

/* Converte o lexema do tamanho do vetor detectando overflow e zero. */
static VectorSizeStatus parse_vector_size(
    const AstDeclarator *declarator,
    size_t *out_size
)
{
    size_t value = 0;
    size_t i;

    if (declarator == NULL || out_size == NULL ||
        declarator->vector_size_lexeme == NULL ||
        declarator->vector_size_length == 0) {
        return VECTOR_SIZE_INVALID_AST;
    }

    for (i = 0; i < declarator->vector_size_length; i++) {
        unsigned char digit = (unsigned char)
            declarator->vector_size_lexeme[i];

        if (digit < (unsigned char)'0' || digit > (unsigned char)'9') {
            return VECTOR_SIZE_INVALID_AST;
        }
        digit = (unsigned char)(digit - (unsigned char)'0');
        if (value > (SIZE_MAX - digit) / 10) {
            return VECTOR_SIZE_INVALID_VALUE;
        }
        value = value * 10 + digit;
    }

    if (value == 0) {
        return VECTOR_SIZE_INVALID_VALUE;
    }
    *out_size = value;
    return VECTOR_SIZE_VALID;
}

/* Passada 1: registra o nome do registro e rejeita campos duplicados. */
static bool collect_record(
    SemanticAnalyzer *analyzer,
    const AstRecordDecl *record
)
{
    size_t i;

    if (analyzer == NULL || record == NULL || record->name == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    if (!declare_symbol(
            analyzer,
            record->name,
            SEM_SYMBOL_RECORD,
            SEM_TYPE_ERROR,
            record->location,
            0,
            NULL,
            record
        )) {
        return false;
    }

    if (record->field_count > 0 && record->fields == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    for (i = 0; i < record->field_count; i++) {
        const AstField *field = record->fields[i];
        size_t j;

        if (field == NULL || field->name == NULL) {
            mark_fatal(analyzer);
            return false;
        }
        if (semantic_type_from_ast(analyzer, field->type.kind) ==
                SEM_TYPE_VAZIO) {
            mark_fatal(analyzer);
            return false;
        }

        for (j = 0; j < i; j++) {
            const AstField *previous = record->fields[j];

            if (previous != NULL && previous->name != NULL &&
                strcmp(previous->name, field->name) == 0) {
                add_diagnostic(
                    analyzer,
                    SEM_INVALID_RECORD_DECLARATION,
                    field->location,
                    field->name,
                    MESSAGE_INVALID_RECORD
                );
                break;
            }
        }
        if (analyzer->fatal) {
            return false;
        }
    }
    return true;
}

/* Passada 1: variaveis e vetores globais (visiveis em todo o programa). */
static bool collect_global_declaration(
    SemanticAnalyzer *analyzer,
    const AstDeclaration *declaration
)
{
    return declare_declarators(
        analyzer,
        declaration,
        &analyzer->global_bytes
    );
}

static bool validate_function_header(
    SemanticAnalyzer *analyzer,
    const AstFunctionDecl *function,
    SemanticType *out_return_type
)
{
    size_t i;

    if (analyzer == NULL || function == NULL || out_return_type == NULL ||
        function->body == NULL ||
        (function->is_main && function->name != NULL) ||
        (!function->is_main && function->name == NULL)) {
        mark_fatal(analyzer);
        return false;
    }

    *out_return_type = semantic_type_from_ast(
        analyzer,
        function->return_type.kind
    );
    if (analyzer->fatal) {
        return false;
    }

    if (function->parameter_count > 0 && function->parameters == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    for (i = 0; i < function->parameter_count; i++) {
        const AstParameter *parameter = function->parameters[i];
        SemanticType parameter_type;

        if (parameter == NULL || parameter->name == NULL) {
            mark_fatal(analyzer);
            return false;
        }
        parameter_type = semantic_type_from_ast(
            analyzer,
            parameter->type.kind
        );
        if (analyzer->fatal || parameter_type == SEM_TYPE_VAZIO) {
            mark_fatal(analyzer);
            return false;
        }
    }
    return true;
}

/*
 * Passada 1 (D26): percorre os itens de nivel superior coletando
 * registros, globais e assinaturas, e conta as funcoes principal
 * (exatamente uma, D27). Isso permite chamadas para funcoes declaradas
 * mais abaixo e recursao mutua.
 */
static bool collect_globals(
    SemanticAnalyzer *analyzer,
    size_t *main_count
)
{
    size_t i;

    if (analyzer == NULL || analyzer->program == NULL ||
        main_count == NULL) {
        mark_fatal(analyzer);
        return false;
    }
    if (analyzer->program->count > 0 && analyzer->program->items == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    for (i = 0; i < analyzer->program->count; i++) {
        const AstTopLevelItem *item = analyzer->program->items[i];

        if (item == NULL) {
            mark_fatal(analyzer);
            return false;
        }
        if (item->kind == AST_TOP_RECORD &&
            !collect_record(analyzer, item->as.record)) {
            return false;
        }
    }

    for (i = 0; i < analyzer->program->count; i++) {
        const AstTopLevelItem *item = analyzer->program->items[i];

        switch (item->kind) {
        case AST_TOP_RECORD:
            break;
        case AST_TOP_DECLARATION:
            if (!collect_global_declaration(analyzer, item->as.declaration)) {
                return false;
            }
            break;
        case AST_TOP_FUNCTION:
        {
            SemanticType return_type;

            if (item->as.function == NULL) {
                mark_fatal(analyzer);
                return false;
            }
            if (!validate_function_header(
                    analyzer,
                    item->as.function,
                    &return_type
                )) {
                return false;
            }
            if (item->as.function->is_main) {
                if (*main_count == SIZE_MAX) {
                    mark_fatal(analyzer);
                    return false;
                }
                (*main_count)++;
                if (*main_count > 1) {
                    add_diagnostic(
                        analyzer,
                        SEM_INVALID_PRINCIPAL,
                        item->as.function->location,
                        NULL,
                        MESSAGE_INVALID_PRINCIPAL
                    );
                }
            } else {
                if (item->as.function->name == NULL) {
                    mark_fatal(analyzer);
                    return false;
                }
                if (!declare_symbol(
                        analyzer,
                        item->as.function->name,
                        SEM_SYMBOL_FUNCTION,
                        return_type,
                        item->as.function->location,
                        0,
                        item->as.function,
                        NULL
                    )) {
                    return false;
                }
            }
            break;
        }
        default:
            mark_fatal(analyzer);
            return false;
        }

        if (analyzer->fatal) {
            return false;
        }
    }

    if (*main_count == 0) {
        SourceLocation location = {1, 1};

        add_diagnostic(
            analyzer,
            SEM_INVALID_PRINCIPAL,
            location,
            NULL,
            MESSAGE_INVALID_PRINCIPAL
        );
    }
    return !analyzer->fatal;
}

static SemanticType analyze_expression(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
);

static SemanticType analyze_reference(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
);

static SemanticType analyze_lvalue(
    SemanticAnalyzer *analyzer,
    const AstLValue *lvalue
);

static void analyze_block_contents(
    SemanticAnalyzer *analyzer,
    const AstBlock *block
);

static void analyze_statement(
    SemanticAnalyzer *analyzer,
    const AstStatement *statement
);

static void analyze_io(
    SemanticAnalyzer *analyzer,
    const AstIo *io
);

static void analyze_assignment(
    SemanticAnalyzer *analyzer,
    const AstAssignment *assignment
);

static SemanticType analyze_call_reference(
    SemanticAnalyzer *analyzer,
    const AstReference *reference,
    bool allow_void
);
static SemanticType analyze_builtin_call(
    SemanticAnalyzer *analyzer,
    const AstReference *reference,
    AstBuiltin builtin,
    bool allow_void
);
static SemanticType analyze_scalar(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
);

/*
 * true quando o erro de compatibilidade vem de "inteira <- a / b": o caso
 * mais comum de surpresa com a D18; a mensagem passa a sugerir //.
 */
static bool is_integer_from_division(
    SemanticType destination,
    const AstExpression *value
)
{
    return destination == SEM_TYPE_INTEIRA && value != NULL &&
           value->kind == AST_EXPR_BINARY &&
           value->as.binary.op == AST_BINARY_DIVIDE;
}

static SemanticType sem_of_kind(AstTypeKind kind)
{
    switch (kind) {
    case AST_TYPE_BOOL:
        return SEM_TYPE_BOOL;
    case AST_TYPE_INTEIRA:
        return SEM_TYPE_INTEIRA;
    case AST_TYPE_FLUT:
        return SEM_TYPE_FLUT;
    case AST_TYPE_PALAVRA:
        return SEM_TYPE_PALAVRA;
    case AST_TYPE_DUPLOCARPADO:
        return SEM_TYPE_DUPLOCARPADO;
    case AST_TYPE_REGISTRO:
        return SEM_TYPE_REGISTRO;
    default:
        return SEM_TYPE_ERROR;
    }
}

/*
 * Exige valor escalar. Vetor sem indice e registro inteiro so existem em
 * contextos proprios (DSL vetorial e atribuicao de registro); em qualquer
 * outro lugar sao erro.
 */
static SemanticType require_scalar(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression,
    SemanticType type
)
{
    if (type == SEM_TYPE_VETOR) {
        bool is_name =
            expression->kind == AST_EXPR_REFERENCE &&
            expression->as.reference.kind == AST_REFERENCE_VARIABLE;

        add_diagnostic(
            analyzer,
            is_name ? SEM_NAME_KIND_MISMATCH : SEM_INVALID_VECTOR_OPERATION,
            expression->location,
            is_name ? expression->as.reference.name : NULL,
            is_name ? MESSAGE_VECTOR_AS_SCALAR : MESSAGE_VECTOR_CONTEXT
        );
        return SEM_TYPE_ERROR;
    }
    if (type == SEM_TYPE_REGISTRO) {
        add_diagnostic(
            analyzer,
            SEM_TYPE_MISMATCH,
            expression->location,
            NULL,
            MESSAGE_RECORD_AS_SCALAR
        );
        return SEM_TYPE_ERROR;
    }
    return type;
}

static SemanticType analyze_scalar(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    SemanticType type = analyze_expression(analyzer, expression);

    if (analyzer->fatal) {
        return SEM_TYPE_ERROR;
    }
    return require_scalar(analyzer, expression, type);
}

/* Divisor literalmente zero: inteiro constante 0 ou decimal 0.0 (D40). */
static bool is_constant_zero(const AstExpression *expression)
{
    long long value;

    if (expression == NULL) {
        return false;
    }
    if (ast_constant_integer(expression, &value)) {
        return value == 0;
    }
    if (expression->kind == AST_EXPR_LITERAL &&
        expression->as.literal.kind == AST_LITERAL_DECIMAL &&
        expression->as.literal.lexeme != NULL) {
        return strtod(expression->as.literal.lexeme, NULL) == 0.0;
    }
    return false;
}

static bool analyze_nested_block(
    SemanticAnalyzer *analyzer,
    const AstBlock *block
)
{
    bool popped;

    if (analyzer == NULL || block == NULL) {
        mark_fatal(analyzer);
        return false;
    }
    if (!scope_push(analyzer)) {
        return false;
    }

    analyze_block_contents(analyzer, block);
    popped = scope_pop(analyzer);
    if (!popped) {
        return false;
    }
    return !analyzer->fatal;
}

/* Declaracao local: valida tamanho de vetor e declara cada nome. */
static void analyze_declaration(
    SemanticAnalyzer *analyzer,
    const AstDeclaration *declaration
)
{
    size_t ignored = 0;

    (void)declare_declarators(analyzer, declaration, &ignored);
}

/* Condicao de cond/durante/repete deve ser bool (D09; sem truthiness). */
static void analyze_condition(
    SemanticAnalyzer *analyzer,
    const AstExpression *condition
)
{
    SemanticType type = analyze_scalar(analyzer, condition);

    if (analyzer == NULL || analyzer->fatal || type == SEM_TYPE_ERROR) {
        return;
    }
    if (type != SEM_TYPE_BOOL) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_CONDITION,
            condition->location,
            NULL,
            MESSAGE_INVALID_CONDITION
        );
    }
}

static void analyze_if(
    SemanticAnalyzer *analyzer,
    const AstIf *if_statement
)
{
    size_t i;

    if (analyzer == NULL || if_statement == NULL ||
        if_statement->condition == NULL ||
        if_statement->then_block == NULL) {
        mark_fatal(analyzer);
        return;
    }

    analyze_condition(analyzer, if_statement->condition);
    analyze_nested_block(analyzer, if_statement->then_block);

    if (analyzer->fatal) {
        return;
    }
    if (if_statement->else_if_count > 0 &&
        if_statement->else_if_branches == NULL) {
        mark_fatal(analyzer);
        return;
    }
    for (i = 0; i < if_statement->else_if_count; i++) {
        const AstIfBranch *branch = if_statement->else_if_branches[i];

        if (branch == NULL || branch->condition == NULL ||
            branch->block == NULL) {
            mark_fatal(analyzer);
            return;
        }
        analyze_condition(analyzer, branch->condition);
        analyze_nested_block(analyzer, branch->block);
        if (analyzer->fatal) {
            return;
        }
    }

    if (if_statement->else_block != NULL) {
        analyze_nested_block(analyzer, if_statement->else_block);
    }
}

static void analyze_while(
    SemanticAnalyzer *analyzer,
    const AstWhile *while_statement
)
{
    if (analyzer == NULL || while_statement == NULL ||
        while_statement->condition == NULL ||
        while_statement->body == NULL) {
        mark_fatal(analyzer);
        return;
    }

    analyze_condition(analyzer, while_statement->condition);
    analyze_nested_block(analyzer, while_statement->body);
}

static void analyze_repeat(
    SemanticAnalyzer *analyzer,
    const AstRepeat *repeat
)
{
    if (analyzer == NULL || repeat == NULL ||
        repeat->initialization == NULL || repeat->condition == NULL ||
        repeat->update == NULL || repeat->body == NULL) {
        mark_fatal(analyzer);
        return;
    }

    analyze_assignment(analyzer, repeat->initialization);
    analyze_condition(analyzer, repeat->condition);
    analyze_assignment(analyzer, repeat->update);
    analyze_nested_block(analyzer, repeat->body);
}

static void analyze_return(
    SemanticAnalyzer *analyzer,
    const AstReturn *return_statement
)
{
    SemanticType value_type;
    SemanticType return_type;

    if (analyzer == NULL || return_statement == NULL ||
        return_statement->value == NULL ||
        analyzer->current_function == NULL) {
        mark_fatal(analyzer);
        return;
    }

    value_type = analyze_scalar(analyzer, return_statement->value);
    return_type = semantic_type_from_ast(
        analyzer,
        analyzer->current_function->return_type.kind
    );
    if (analyzer->fatal) {
        return;
    }

    if (return_type == SEM_TYPE_VAZIO) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_RETURN,
            return_statement->location,
            NULL,
            MESSAGE_INVALID_RETURN
        );
    } else if (value_type != SEM_TYPE_ERROR &&
               !is_assignable(return_type, value_type)) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_RETURN,
            return_statement->location,
            NULL,
            is_integer_from_division(return_type, return_statement->value)
                ? MESSAGE_RETURN_DIVISION
                : MESSAGE_INVALID_RETURN
        );
    }
}

static void analyze_io(
    SemanticAnalyzer *analyzer,
    const AstIo *io
)
{
    SemanticType type;

    if (analyzer == NULL || io == NULL) {
        mark_fatal(analyzer);
        return;
    }

    switch (io->kind) {
    case AST_IO_READ:
        if (io->as.target == NULL) {
            mark_fatal(analyzer);
            return;
        }
        type = analyze_lvalue(analyzer, io->as.target);
        if (type == SEM_TYPE_VETOR || type == SEM_TYPE_REGISTRO) {
            add_diagnostic(
                analyzer,
                SEM_NAME_KIND_MISMATCH,
                io->as.target->location,
                io->as.target->name,
                MESSAGE_READ_SCALAR
            );
        }
        break;
    case AST_IO_WRITE:
        if (io->as.value == NULL) {
            mark_fatal(analyzer);
            return;
        }
        (void)analyze_scalar(analyzer, io->as.value);
        break;
    default:
        mark_fatal(analyzer);
        break;
    }
}

static void analyze_statement(
    SemanticAnalyzer *analyzer,
    const AstStatement *statement
)
{
    if (analyzer == NULL || statement == NULL || analyzer->fatal) {
        if (statement == NULL) {
            mark_fatal(analyzer);
        }
        return;
    }

    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        analyze_declaration(analyzer, statement->as.declaration);
        break;
    case AST_STMT_ASSIGNMENT:
        analyze_assignment(analyzer, statement->as.assignment);
        break;
    case AST_STMT_IF:
        analyze_if(analyzer, statement->as.if_statement);
        break;
    case AST_STMT_WHILE:
        analyze_while(analyzer, statement->as.while_statement);
        break;
    case AST_STMT_REPEAT:
        analyze_repeat(analyzer, statement->as.repeat_statement);
        break;
    case AST_STMT_RETURN:
        analyze_return(analyzer, statement->as.return_statement);
        break;
    case AST_STMT_BLOCK:
        analyze_nested_block(analyzer, statement->as.block);
        break;
    case AST_STMT_IO:
        analyze_io(analyzer, statement->as.io);
        break;
    case AST_STMT_CALL:
        /*
         * D33: chamada como comando. Unico contexto em que uma funcao vazio
         * pode ser chamada; para as demais, o valor e descartado.
         */
        if (statement->as.call == NULL ||
            statement->as.call->kind != AST_EXPR_REFERENCE ||
            statement->as.call->as.reference.kind != AST_REFERENCE_CALL) {
            mark_fatal(analyzer);
            break;
        }
        (void)analyze_call_reference(
            analyzer,
            &statement->as.call->as.reference,
            true
        );
        break;
    default:
        mark_fatal(analyzer);
        break;
    }
}

static void analyze_block_contents(
    SemanticAnalyzer *analyzer,
    const AstBlock *block
)
{
    size_t i;

    if (analyzer == NULL || block == NULL) {
        mark_fatal(analyzer);
        return;
    }
    if (block->statement_count > 0 && block->statements == NULL) {
        mark_fatal(analyzer);
        return;
    }

    for (i = 0; i < block->statement_count; i++) {
        if (analyzer->fatal) {
            return;
        }
        analyze_statement(analyzer, block->statements[i]);
    }
}

/*
 * Alvo de atribuicao ou de le(): variavel/parametro escalar, ou
 * elemento de vetor com indice inteira (D28). Devolve o tipo do alvo.
 */
/*
 * Acesso a nome com indice e campo opcionais, comum a lvalues e
 * referencias (D28, D39, D40, D41):
 *   x        variavel/parametro: escalar ou registro inteiro
 *   v        vetor inteiro (operando da DSL vetorial)
 *   v{i}     elemento; i deve ser inteira e, se constante, dentro dos limites
 *   p.c      campo de registro; v{i}.c campo de elemento de vetor de registros
 * Preenche id, indice de campo e tipo anotado.
 */
static SemanticType analyze_access(
    SemanticAnalyzer *analyzer,
    const char *name,
    SourceLocation location,
    bool indexed,
    const AstExpression *index,
    const char *field_name,
    size_t *out_id,
    size_t *out_field,
    AstValueType *out_type
)
{
    SemanticSymbol *symbol;
    SemanticType type;
    const AstRecordDecl *record;
    SemanticType index_type = SEM_TYPE_ERROR;
    size_t i;

    *out_type = value_type_make(SEM_TYPE_ERROR, 0, NULL);
    if (indexed) {
        if (index == NULL) {
            mark_fatal(analyzer);
            return SEM_TYPE_ERROR;
        }
        index_type = analyze_scalar(analyzer, index);
        if (analyzer->fatal) {
            return SEM_TYPE_ERROR;
        }
    }

    symbol = lookup_visible(analyzer->current_scope, name);
    if (symbol == NULL) {
        add_diagnostic(
            analyzer,
            SEM_UNDECLARED_IDENTIFIER,
            location,
            name,
            MESSAGE_UNDECLARED
        );
        return SEM_TYPE_ERROR;
    }

    if (!indexed) {
        if (symbol->kind == SEM_SYMBOL_VECTOR) {
            if (field_name != NULL) {
                add_diagnostic(
                    analyzer,
                    SEM_INVALID_FIELD,
                    location,
                    name,
                    MESSAGE_FIELD_ON_VECTOR
                );
                return SEM_TYPE_ERROR;
            }
            *out_id = symbol->id;
            *out_type = value_type_make(
                symbol->type,
                symbol->vector_size,
                symbol->record
            );
            /* Tamanho invalido ja foi relatado na declaracao. */
            out_type->valid = symbol->type != SEM_TYPE_ERROR;
            return out_type->valid ? SEM_TYPE_VETOR : SEM_TYPE_ERROR;
        }
        if (symbol->kind != SEM_SYMBOL_VARIABLE &&
            symbol->kind != SEM_SYMBOL_PARAMETER) {
            add_diagnostic(
                analyzer,
                SEM_NAME_KIND_MISMATCH,
                location,
                name,
                MESSAGE_NAME_KIND
            );
            return SEM_TYPE_ERROR;
        }
    } else {
        if (symbol->kind != SEM_SYMBOL_VECTOR) {
            add_diagnostic(
                analyzer,
                SEM_NOT_A_VECTOR,
                location,
                name,
                MESSAGE_NOT_VECTOR
            );
            return SEM_TYPE_ERROR;
        }
        if (index_type == SEM_TYPE_ERROR) {
            return SEM_TYPE_ERROR;
        }
        if (index_type != SEM_TYPE_INTEIRA) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_VECTOR_INDEX,
                index->location,
                NULL,
                MESSAGE_VECTOR_INDEX
            );
            return SEM_TYPE_ERROR;
        }
        {
            long long constant;

            if (symbol->vector_size > 0 &&
                ast_constant_integer(index, &constant) &&
                (constant < 0 ||
                 (unsigned long long)constant >= symbol->vector_size)) {
                add_diagnostic(
                    analyzer,
                    SEM_INDEX_OUT_OF_BOUNDS,
                    index->location,
                    name,
                    MESSAGE_INDEX_BOUNDS
                );
                return SEM_TYPE_ERROR;
            }
        }
    }

    *out_id = symbol->id;
    type = symbol->type;
    record = symbol->record;
    if (type == SEM_TYPE_ERROR) {
        return SEM_TYPE_ERROR;
    }

    if (field_name != NULL) {
        if (type != SEM_TYPE_REGISTRO || record == NULL) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_FIELD,
                location,
                name,
                MESSAGE_NOT_A_RECORD_VALUE
            );
            return SEM_TYPE_ERROR;
        }
        for (i = 0; i < record->field_count; i++) {
            if (record->fields[i] != NULL &&
                strcmp(record->fields[i]->name, field_name) == 0) {
                break;
            }
        }
        if (i == record->field_count) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_FIELD,
                location,
                field_name,
                MESSAGE_UNKNOWN_FIELD
            );
            return SEM_TYPE_ERROR;
        }
        *out_field = i;
        type = semantic_type_from_ast(analyzer, record->fields[i]->type.kind);
        record = NULL;
    }

    *out_type = value_type_make(type, 0, record);
    return type;
}

/*
 * Alvo de atribuicao ou de le(): variavel/parametro, elemento de vetor,
 * campo de registro, registro inteiro ou vetor inteiro (DSL). Devolve o tipo
 * do alvo e anota o lvalue.
 */
static SemanticType analyze_lvalue(
    SemanticAnalyzer *analyzer,
    const AstLValue *lvalue
)
{
    AstLValue *mutable_lvalue = (AstLValue *)lvalue;

    if (analyzer == NULL || lvalue == NULL || lvalue->name == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    if (lvalue->kind != AST_LVALUE_VARIABLE &&
        lvalue->kind != AST_LVALUE_VECTOR) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    return analyze_access(
        analyzer,
        lvalue->name,
        lvalue->location,
        lvalue->kind == AST_LVALUE_VECTOR,
        lvalue->index,
        lvalue->field_name,
        &mutable_lvalue->symbol_id,
        &mutable_lvalue->field_index,
        &mutable_lvalue->type
    );
}

/*
 * Atribuicao. Formas aceitas:
 *   escalar  <- escalar compativel (D16)
 *   registro <- registro do mesmo tipo (copia, D39)
 *   vetor    <- expressao vetorial de mesmo tamanho (D41)
 *   vetor    <- escalar numerico (preenche todos os elementos, D41)
 */
static void analyze_assignment(
    SemanticAnalyzer *analyzer,
    const AstAssignment *assignment
)
{
    SemanticType target_type;
    SemanticType value_type;
    const AstLValue *target;
    const AstExpression *value;

    if (analyzer == NULL || assignment == NULL ||
        assignment->target == NULL || assignment->value == NULL) {
        mark_fatal(analyzer);
        return;
    }
    target = assignment->target;
    value = assignment->value;

    target_type = analyze_lvalue(analyzer, target);
    value_type = analyze_expression(analyzer, value);

    if (analyzer->fatal || target_type == SEM_TYPE_ERROR ||
        value_type == SEM_TYPE_ERROR) {
        return;
    }

    if (target_type == SEM_TYPE_VETOR) {
        SemanticType destination = sem_of_kind(target->type.kind);

        if (!is_numeric(destination)) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_VECTOR_OPERATION,
                assignment->location,
                target->name,
                MESSAGE_VECTOR_ELEMENT
            );
            return;
        }
        if (value_type == SEM_TYPE_VETOR) {
            SemanticType source = sem_of_kind(value->type.kind);

            if (value->type.vector_size != target->type.vector_size) {
                add_diagnostic(
                    analyzer,
                    SEM_VECTOR_SIZE_MISMATCH,
                    assignment->location,
                    target->name,
                    MESSAGE_VECTOR_SIZE_MISMATCH
                );
            } else if (!is_numeric(source)) {
                add_diagnostic(
                    analyzer,
                    SEM_INVALID_VECTOR_OPERATION,
                    value->location,
                    NULL,
                    MESSAGE_VECTOR_ELEMENT
                );
            } else if (!is_assignable(destination, source)) {
                add_diagnostic(
                    analyzer,
                    SEM_INVALID_ASSIGNMENT,
                    assignment->location,
                    NULL,
                    MESSAGE_ASSIGNMENT
                );
            }
            return;
        }
        if (value_type == SEM_TYPE_REGISTRO ||
            !is_assignable(destination, value_type)) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_ASSIGNMENT,
                assignment->location,
                NULL,
                MESSAGE_ASSIGNMENT
            );
        }
        return;
    }

    if (target_type == SEM_TYPE_REGISTRO) {
        if (value_type != SEM_TYPE_REGISTRO ||
            value->type.record != target->type.record) {
            add_diagnostic(
                analyzer,
                SEM_INVALID_ASSIGNMENT,
                assignment->location,
                NULL,
                MESSAGE_ASSIGNMENT
            );
        }
        return;
    }

    value_type = require_scalar(analyzer, value, value_type);
    if (value_type == SEM_TYPE_ERROR) {
        return;
    }
    if (!is_assignable(target_type, value_type)) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_ASSIGNMENT,
            assignment->location,
            NULL,
            is_integer_from_division(target_type, value)
                ? MESSAGE_ASSIGNMENT_DIVISION
                : MESSAGE_ASSIGNMENT
        );
    }
}

/*
 * Verifica uma chamada ja resolvida para uma funcao: quantidade e tipos dos
 * argumentos (com promocao, D16). allow_void diz se o contexto aceita uma
 * funcao vazio (somente o comando de chamada, D33); em expressoes, chamar
 * uma funcao vazio e SEM_VOID_VALUE.
 */
static SemanticType analyze_call(
    SemanticAnalyzer *analyzer,
    const AstReference *reference,
    const SemanticSymbol *symbol,
    bool allow_void
)
{
    const AstFunctionDecl *function;
    size_t argument_count;
    size_t parameter_count;
    size_t i;

    if (analyzer == NULL || reference == NULL || symbol == NULL ||
        symbol->function == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    function = symbol->function;
    argument_count = reference->as.call.argument_count;
    parameter_count = function->parameter_count;

    if (argument_count > 0 && reference->as.call.arguments == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    if (parameter_count > 0 && function->parameters == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    if (argument_count != parameter_count) {
        add_diagnostic(
            analyzer,
            SEM_ARGUMENT_COUNT_MISMATCH,
            reference->location,
            reference->name,
            MESSAGE_ARGUMENT_COUNT
        );
    }

    for (i = 0; i < argument_count; i++) {
        SemanticType argument_type = analyze_scalar(
            analyzer,
            reference->as.call.arguments[i]
        );

        if (analyzer->fatal) {
            return SEM_TYPE_ERROR;
        }
        if (i < parameter_count) {
            const AstParameter *parameter = function->parameters[i];
            SemanticType expected_type;

            if (parameter == NULL) {
                mark_fatal(analyzer);
                return SEM_TYPE_ERROR;
            }
            expected_type = semantic_type_from_ast(
                analyzer,
                parameter->type.kind
            );
            if (analyzer->fatal) {
                return SEM_TYPE_ERROR;
            }
            if (argument_type != SEM_TYPE_ERROR &&
                !is_assignable(expected_type, argument_type)) {
                add_diagnostic(
                    analyzer,
                    SEM_ARGUMENT_TYPE_MISMATCH,
                    reference->as.call.arguments[i]->location,
                    NULL,
                    MESSAGE_ARGUMENT_TYPE
                );
            }
        }
    }

    if (function->return_type.kind == AST_TYPE_VAZIO) {
        if (allow_void) {
            return SEM_TYPE_VAZIO;
        }
        add_diagnostic(
            analyzer,
            SEM_VOID_VALUE,
            reference->location,
            reference->name,
            MESSAGE_VOID_VALUE
        );
        return SEM_TYPE_ERROR;
    }
    return semantic_type_from_ast(analyzer, function->return_type.kind);
}

/* Referencia em expressao: variavel, elemento, campo ou chamada. */
static SemanticType analyze_reference(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    const AstReference *reference;
    AstReference *mutable_reference;
    SemanticType type;
    AstValueType value_type;

    if (analyzer == NULL || expression == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    reference = &expression->as.reference;
    mutable_reference = (AstReference *)reference;
    if (reference->name == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    if (reference->kind == AST_REFERENCE_CALL) {
        return analyze_call_reference(analyzer, reference, false);
    }
    if (reference->kind != AST_REFERENCE_VARIABLE &&
        reference->kind != AST_REFERENCE_VECTOR) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    type = analyze_access(
        analyzer,
        reference->name,
        reference->location,
        reference->kind == AST_REFERENCE_VECTOR,
        reference->as.vector.index,
        reference->field_name,
        &mutable_reference->symbol_id,
        &mutable_reference->field_index,
        &value_type
    );
    annotate_expression(expression, value_type);
    return type;
}

/*
 * Resolve o nome de uma chamada e a verifica. Se o nome nao existe ou nao e
 * funcao, o erro e relatado e os argumentos ainda sao analisados (para que
 * erros dentro deles tambem aparecam na mesma execucao).
 */
static SemanticType analyze_call_reference(
    SemanticAnalyzer *analyzer,
    const AstReference *reference,
    bool allow_void
)
{
    SemanticSymbol *symbol;
    size_t i;

    if (analyzer == NULL || reference == NULL || reference->name == NULL ||
        reference->kind != AST_REFERENCE_CALL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    symbol = lookup_visible(analyzer->current_scope, reference->name);
    if (symbol != NULL && symbol->kind == SEM_SYMBOL_FUNCTION) {
        if (symbol->builtin != AST_BUILTIN_NONE) {
            return analyze_builtin_call(
                analyzer,
                reference,
                symbol->builtin,
                allow_void
            );
        }
        ((AstReference *)reference)->callee = symbol->function;
        return analyze_call(analyzer, reference, symbol, allow_void);
    }

    add_diagnostic(
        analyzer,
        symbol == NULL ? SEM_UNKNOWN_FUNCTION : SEM_NOT_A_FUNCTION,
        reference->location,
        reference->name,
        symbol == NULL ? MESSAGE_UNKNOWN_FUNCTION : MESSAGE_NOT_FUNCTION
    );
    if (reference->as.call.argument_count > 0 &&
        reference->as.call.arguments == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    for (i = 0; i < reference->as.call.argument_count; i++) {
        analyze_expression(analyzer, reference->as.call.arguments[i]);
        if (analyzer->fatal) {
            return SEM_TYPE_ERROR;
        }
    }
    return SEM_TYPE_ERROR;
}

/*
 * Funcoes embutidas (registradas na tabela de simbolos em semantic_analyze):
 *   somavet(v), maxvet(v), minvet(v)  -> tipo do elemento     (D41)
 *   prodesc(a, b)                     -> tipo mais largo      (D41)
 *   pinomodo(pino, modo), pinoescreve(pino, valor), espera(ms) -> vazio (D44)
 *   pinole(pino)                      -> inteira              (D44)
 * Os argumentos das reducoes podem ser quaisquer expressoes vetoriais.
 */
static size_t builtin_arity(AstBuiltin builtin)
{
    switch (builtin) {
    case AST_BUILTIN_PRODESC:
    case AST_BUILTIN_PINOMODO:
    case AST_BUILTIN_PINOESCREVE:
        return 2;
    default:
        return 1;
    }
}

static bool builtin_is_reduction(AstBuiltin builtin)
{
    return builtin == AST_BUILTIN_SOMAVET || builtin == AST_BUILTIN_MAXVET ||
           builtin == AST_BUILTIN_MINVET || builtin == AST_BUILTIN_PRODESC;
}

static SemanticType analyze_builtin_call(
    SemanticAnalyzer *analyzer,
    const AstReference *reference,
    AstBuiltin builtin,
    bool allow_void
)
{
    size_t count = reference->as.call.argument_count;
    size_t arity = builtin_arity(builtin);
    SemanticType result = SEM_TYPE_ERROR;
    bool ok = true;
    size_t i;

    ((AstReference *)reference)->builtin = builtin;
    if (count > 0 && reference->as.call.arguments == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    if (count != arity) {
        add_diagnostic(
            analyzer,
            SEM_ARGUMENT_COUNT_MISMATCH,
            reference->location,
            reference->name,
            MESSAGE_ARGUMENT_COUNT
        );
        ok = false;
    }

    for (i = 0; i < count; i++) {
        const AstExpression *argument = reference->as.call.arguments[i];
        SemanticType type;

        if (builtin_is_reduction(builtin)) {
            type = analyze_expression(analyzer, argument);
            if (analyzer->fatal) {
                return SEM_TYPE_ERROR;
            }
            if (type == SEM_TYPE_ERROR) {
                ok = false;
                continue;
            }
            if (type != SEM_TYPE_VETOR ||
                !is_numeric(sem_of_kind(argument->type.kind))) {
                add_diagnostic(
                    analyzer,
                    SEM_ARGUMENT_TYPE_MISMATCH,
                    argument->location,
                    reference->name,
                    MESSAGE_VECTOR_ARGUMENT
                );
                ok = false;
                continue;
            }
            if (i == 0) {
                result = sem_of_kind(argument->type.kind);
            } else {
                const AstExpression *first = reference->as.call.arguments[0];

                if (result != SEM_TYPE_ERROR &&
                    first->type.vector_size != argument->type.vector_size) {
                    add_diagnostic(
                        analyzer,
                        SEM_VECTOR_SIZE_MISMATCH,
                        argument->location,
                        reference->name,
                        MESSAGE_VECTOR_SIZE_MISMATCH
                    );
                    ok = false;
                }
                result = wider_numeric_type(
                    result,
                    sem_of_kind(argument->type.kind)
                );
            }
        } else {
            type = analyze_scalar(analyzer, argument);
            if (analyzer->fatal) {
                return SEM_TYPE_ERROR;
            }
            if (type != SEM_TYPE_ERROR &&
                !is_assignable(SEM_TYPE_INTEIRA, type)) {
                add_diagnostic(
                    analyzer,
                    SEM_ARGUMENT_TYPE_MISMATCH,
                    argument->location,
                    NULL,
                    MESSAGE_ARGUMENT_TYPE
                );
                ok = false;
            }
        }
    }

    if (builtin_is_reduction(builtin)) {
        return ok ? result : SEM_TYPE_ERROR;
    }
    if (builtin == AST_BUILTIN_PINOLE) {
        return SEM_TYPE_INTEIRA;
    }
    if (!allow_void) {
        add_diagnostic(
            analyzer,
            SEM_VOID_VALUE,
            reference->location,
            reference->name,
            MESSAGE_VOID_VALUE
        );
        return SEM_TYPE_ERROR;
    }
    return SEM_TYPE_VAZIO;
}

/*
 * Operacao binaria com ao menos um operando vetor (DSL, D41): + - * entre
 * vetores de mesmo tamanho ou entre vetor e escalar, elementos numericos,
 * com a promocao usual (D17). O resultado e um vetor anotado.
 */
static SemanticType analyze_vector_binary(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression,
    SemanticType left,
    SemanticType right
)
{
    const AstExpression *left_node = expression->as.binary.left;
    const AstExpression *right_node = expression->as.binary.right;
    SemanticType left_element;
    SemanticType right_element;
    size_t size;

    if (left == SEM_TYPE_REGISTRO || right == SEM_TYPE_REGISTRO) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_VECTOR_OPERATION,
            expression->location,
            NULL,
            MESSAGE_VECTOR_OPERATION
        );
        return SEM_TYPE_ERROR;
    }
    left_element = left == SEM_TYPE_VETOR ? sem_of_kind(left_node->type.kind)
                                          : left;
    right_element = right == SEM_TYPE_VETOR
                        ? sem_of_kind(right_node->type.kind)
                        : right;
    if (left == SEM_TYPE_VETOR && right == SEM_TYPE_VETOR &&
        left_node->type.vector_size != right_node->type.vector_size) {
        add_diagnostic(
            analyzer,
            SEM_VECTOR_SIZE_MISMATCH,
            expression->location,
            NULL,
            MESSAGE_VECTOR_SIZE_MISMATCH
        );
        return SEM_TYPE_ERROR;
    }
    size = left == SEM_TYPE_VETOR ? left_node->type.vector_size
                                  : right_node->type.vector_size;
    if (!is_numeric(left_element) || !is_numeric(right_element)) {
        add_diagnostic(
            analyzer,
            SEM_INVALID_VECTOR_OPERATION,
            expression->location,
            NULL,
            MESSAGE_VECTOR_ELEMENT
        );
        return SEM_TYPE_ERROR;
    }
    annotate_expression(
        expression,
        value_type_make(
            wider_numeric_type(left_element, right_element),
            size,
            NULL
        )
    );
    return SEM_TYPE_VETOR;
}

/*
 * Regras dos operadores binarios (D17-D23). Se algum operando ja tem
 * erro, devolve SEM_TYPE_ERROR sem novo diagnostico (evita cascata).
 */
static SemanticType analyze_binary(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    SemanticType left;
    SemanticType right;
    AstBinaryOp op;

    if (expression == NULL || analyzer == NULL ||
        expression->as.binary.left == NULL ||
        expression->as.binary.right == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    left = analyze_expression(analyzer, expression->as.binary.left);
    right = analyze_expression(analyzer, expression->as.binary.right);
    if (analyzer->fatal || left == SEM_TYPE_ERROR ||
        right == SEM_TYPE_ERROR) {
        return SEM_TYPE_ERROR;
    }
    op = expression->as.binary.op;

    if (left == SEM_TYPE_VETOR || right == SEM_TYPE_VETOR) {
        if (op == AST_BINARY_ADD || op == AST_BINARY_SUBTRACT ||
            op == AST_BINARY_MULTIPLY) {
            return analyze_vector_binary(analyzer, expression, left, right);
        }
        add_diagnostic(
            analyzer,
            SEM_INVALID_VECTOR_OPERATION,
            expression->location,
            NULL,
            MESSAGE_VECTOR_OPERATION
        );
        return SEM_TYPE_ERROR;
    }
    left = require_scalar(analyzer, expression->as.binary.left, left);
    right = require_scalar(analyzer, expression->as.binary.right, right);
    if (analyzer->fatal || left == SEM_TYPE_ERROR ||
        right == SEM_TYPE_ERROR) {
        return SEM_TYPE_ERROR;
    }

    switch (op) {
    case AST_BINARY_ADD:
    case AST_BINARY_SUBTRACT:
    case AST_BINARY_MULTIPLY:
    case AST_BINARY_POWER:
        if (is_numeric(left) && is_numeric(right)) {
            return wider_numeric_type(left, right);
        }
        break;
    case AST_BINARY_DIVIDE:
        /*
         * D18 revista pela D38: '/' e sempre divisao real. O resultado e
         * flut, ou duplocarpado se algum operando for duplocarpado (sem
         * perda de precisao). inteira / inteira e flut.
         */
        if (is_numeric(left) && is_numeric(right)) {
            if (is_constant_zero(expression->as.binary.right)) {
                add_diagnostic(
                    analyzer,
                    SEM_DIVISION_BY_ZERO,
                    expression->as.binary.right->location,
                    NULL,
                    MESSAGE_DIVISION_ZERO
                );
                return SEM_TYPE_ERROR;
            }
            return wider_numeric_type(
                SEM_TYPE_FLUT,
                wider_numeric_type(left, right)
            );
        }
        break;
    case AST_BINARY_DIV_INT:
        if (left == SEM_TYPE_INTEIRA && right == SEM_TYPE_INTEIRA) {
            if (is_constant_zero(expression->as.binary.right)) {
                add_diagnostic(
                    analyzer,
                    SEM_DIVISION_BY_ZERO,
                    expression->as.binary.right->location,
                    NULL,
                    MESSAGE_DIVISION_ZERO
                );
                return SEM_TYPE_ERROR;
            }
            return SEM_TYPE_INTEIRA;
        }
        break;
    case AST_BINARY_LESS:
    case AST_BINARY_GREATER:
        if (is_numeric(left) && is_numeric(right)) {
            return SEM_TYPE_BOOL;
        }
        break;
    case AST_BINARY_EQUAL:
    case AST_BINARY_DIFFERENT:
        if (left == right) {
            return SEM_TYPE_BOOL;
        }
        break;
    case AST_BINARY_AND:
    case AST_BINARY_XOR:
    case AST_BINARY_OR:
        if (left == SEM_TYPE_BOOL && right == SEM_TYPE_BOOL) {
            return SEM_TYPE_BOOL;
        }
        break;
    default:
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    add_diagnostic(
        analyzer,
        SEM_INVALID_OPERATOR,
        expression->location,
        NULL,
        MESSAGE_INVALID_OPERATOR
    );
    return SEM_TYPE_ERROR;
}

static SemanticType analyze_unary(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    SemanticType operand;
    const AstExpression *operand_node;

    if (expression == NULL || analyzer == NULL ||
        expression->as.unary.operand == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    operand_node = expression->as.unary.operand;

    operand = analyze_expression(analyzer, operand_node);
    if (analyzer->fatal || operand == SEM_TYPE_ERROR) {
        return SEM_TYPE_ERROR;
    }

    /* + e - unarios sobre vetor: elemento a elemento (D41). */
    if (operand == SEM_TYPE_VETOR &&
        expression->as.unary.op != AST_UNARY_NOT &&
        is_numeric(sem_of_kind(operand_node->type.kind))) {
        annotate_expression(expression, operand_node->type);
        return SEM_TYPE_VETOR;
    }
    operand = require_scalar(analyzer, operand_node, operand);
    if (operand == SEM_TYPE_ERROR) {
        return SEM_TYPE_ERROR;
    }

    switch (expression->as.unary.op) {
    case AST_UNARY_PLUS:
    case AST_UNARY_MINUS:
        if (is_numeric(operand)) {
            return operand;
        }
        break;
    case AST_UNARY_NOT:
        if (operand == SEM_TYPE_BOOL) {
            return SEM_TYPE_BOOL;
        }
        break;
    default:
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }

    add_diagnostic(
        analyzer,
        SEM_INVALID_OPERATOR,
        expression->location,
        NULL,
        MESSAGE_INVALID_OPERATOR
    );
    return SEM_TYPE_ERROR;
}

/* Tipo de uma expressao; literal decimal e flut (D15). */
/* Tipo de uma expressao, sem anotacao (ver analyze_expression). */
static SemanticType analyze_expression_kind(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.lexeme == NULL ||
            expression->as.literal.lexeme_length == 0) {
            mark_fatal(analyzer);
            return SEM_TYPE_ERROR;
        }
        switch (expression->as.literal.kind) {
        case AST_LITERAL_INTEGER: {
            long long value;

            if (!ast_constant_integer(expression, &value)) {
                add_diagnostic(
                    analyzer,
                    SEM_LITERAL_OUT_OF_RANGE,
                    expression->location,
                    NULL,
                    MESSAGE_LITERAL_RANGE
                );
            }
            return SEM_TYPE_INTEIRA;
        }
        case AST_LITERAL_DECIMAL: {
            double value = strtod(expression->as.literal.lexeme, NULL);

            if (value > 3.4028234663852886e38) {
                add_diagnostic(
                    analyzer,
                    SEM_LITERAL_OUT_OF_RANGE,
                    expression->location,
                    NULL,
                    MESSAGE_LITERAL_RANGE
                );
            }
            return SEM_TYPE_FLUT;
        }
        case AST_LITERAL_TEXT:
            /* Textos ficam na RAM do alvo: aspas saem, o '\0' entra (D45). */
            analyzer->global_bytes = saturating_add(
                analyzer->global_bytes,
                expression->as.literal.lexeme_length >= 2
                    ? expression->as.literal.lexeme_length - 1
                    : 1
            );
            return SEM_TYPE_PALAVRA;
        case AST_LITERAL_BOOLEAN:
            return SEM_TYPE_BOOL;
        default:
            mark_fatal(analyzer);
            return SEM_TYPE_ERROR;
        }
    case AST_EXPR_REFERENCE:
        return analyze_reference(analyzer, expression);
    case AST_EXPR_UNARY:
        return analyze_unary(analyzer, expression);
    case AST_EXPR_BINARY:
        return analyze_binary(analyzer, expression);
    case AST_EXPR_ROOT: {
        SemanticType argument;

        if (expression->as.root.argument == NULL) {
            mark_fatal(analyzer);
            return SEM_TYPE_ERROR;
        }
        argument = analyze_scalar(analyzer, expression->as.root.argument);
        if (argument == SEM_TYPE_ERROR || analyzer->fatal) {
            return SEM_TYPE_ERROR;
        }
        /* D20 revista pela D38: flut, ou duplocarpado para duplocarpado. */
        if (is_numeric(argument)) {
            return wider_numeric_type(SEM_TYPE_FLUT, argument);
        }
        add_diagnostic(
            analyzer,
            SEM_INVALID_OPERATOR,
            expression->location,
            NULL,
            MESSAGE_INVALID_OPERATOR
        );
        return SEM_TYPE_ERROR;
    }
    default:
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
}

/*
 * Tipo de uma expressao; literal decimal e flut (D15). Anota o tipo na AST:
 * resultados escalares sao anotados aqui; vetores e registros inteiros sao
 * anotados por quem os produz (com tamanho e registro).
 */
static SemanticType analyze_expression(
    SemanticAnalyzer *analyzer,
    const AstExpression *expression
)
{
    SemanticType type;

    if (analyzer == NULL || expression == NULL) {
        mark_fatal(analyzer);
        return SEM_TYPE_ERROR;
    }
    type = analyze_expression_kind(analyzer, expression);
    if (type != SEM_TYPE_VETOR && type != SEM_TYPE_REGISTRO) {
        annotate_expression(expression, value_type_make(type, 0, NULL));
    }
    return type;
}

/*
 * Analise de fluxo de retorno (D34).
 *
 * Um comando "sempre termina com respost" quando nenhuma execucao dele pode
 * seguir para o comando seguinte:
 *   - respost;
 *   - bloco cujo algum comando sempre retorna;
 *   - cond com casocontrario final em que todos os ramos sempre retornam;
 *   - durante/repete com condicao literal VER: como a linguagem nao tem
 *     "interromper", o laco so termina por respost.
 * Qualquer outro laco e conservadoramente considerado capaz de terminar.
 */
static bool block_always_returns(const AstBlock *block);

static bool is_literal_true(const AstExpression *expression)
{
    return expression != NULL &&
           expression->kind == AST_EXPR_LITERAL &&
           expression->as.literal.kind == AST_LITERAL_BOOLEAN &&
           expression->as.literal.lexeme != NULL &&
           strcmp(expression->as.literal.lexeme, "VER") == 0;
}

static bool statement_always_returns(const AstStatement *statement)
{
    size_t i;

    if (statement == NULL) {
        return false;
    }
    switch (statement->kind) {
    case AST_STMT_RETURN:
        return true;
    case AST_STMT_BLOCK:
        return block_always_returns(statement->as.block);
    case AST_STMT_IF: {
        const AstIf *if_statement = statement->as.if_statement;

        if (if_statement == NULL || if_statement->else_block == NULL ||
            !block_always_returns(if_statement->then_block) ||
            !block_always_returns(if_statement->else_block)) {
            return false;
        }
        for (i = 0; i < if_statement->else_if_count; i++) {
            if (if_statement->else_if_branches[i] == NULL ||
                !block_always_returns(
                    if_statement->else_if_branches[i]->block
                )) {
                return false;
            }
        }
        return true;
    }
    case AST_STMT_WHILE:
        return statement->as.while_statement != NULL &&
               is_literal_true(statement->as.while_statement->condition);
    case AST_STMT_REPEAT:
        return statement->as.repeat_statement != NULL &&
               is_literal_true(statement->as.repeat_statement->condition);
    default:
        return false;
    }
}

static bool block_always_returns(const AstBlock *block)
{
    size_t i;

    if (block == NULL) {
        return false;
    }
    for (i = 0; i < block->statement_count; i++) {
        if (statement_always_returns(block->statements[i])) {
            return true;
        }
    }
    return false;
}

/*
 * Passada 2 para uma funcao: abre o escopo de funcao, declara parametros,
 * analisa o corpo e, para funcoes nao-vazio que nao sao a principal, exige
 * respost em todos os caminhos. A principal pode terminar sem respost: o
 * valor de retorno e o zero do tipo (como "return 0" implicito do C99).
 */
static bool analyze_function(
    SemanticAnalyzer *analyzer,
    const AstFunctionDecl *function
)
{
    size_t i;

    if (analyzer == NULL || function == NULL || function->body == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    analyzer->current_function = function;
    if (!scope_push(analyzer)) {
        analyzer->current_function = NULL;
        return false;
    }

    if (function->parameter_count > 0 && function->parameters == NULL) {
        mark_fatal(analyzer);
    }
    for (i = 0; i < function->parameter_count && !analyzer->fatal; i++) {
        const AstParameter *parameter = function->parameters[i];
        SemanticType type;

        if (parameter == NULL || parameter->name == NULL) {
            mark_fatal(analyzer);
            break;
        }
        type = semantic_type_from_ast(analyzer, parameter->type.kind);
        if (type == SEM_TYPE_VAZIO || type == SEM_TYPE_ERROR) {
            mark_fatal(analyzer);
            break;
        }
        if (!declare_symbol(
                analyzer,
                parameter->name,
                SEM_SYMBOL_PARAMETER,
                type,
                parameter->location,
                0,
                NULL,
                NULL
            )) {
            break;
        }
        ((AstParameter *)parameter)->symbol_id = analyzer->last_symbol_id;
    }

    if (!analyzer->fatal) {
        analyze_block_contents(analyzer, function->body);
    }
    if (!analyzer->fatal && !function->is_main &&
        function->return_type.kind != AST_TYPE_VAZIO &&
        !block_always_returns(function->body)) {
        add_diagnostic(
            analyzer,
            SEM_MISSING_RETURN,
            function->location,
            function->name,
            MESSAGE_MISSING_RETURN
        );
    }
    if (analyzer->current_scope != analyzer->global_scope) {
        if (!scope_pop(analyzer)) {
            analyzer->fatal = true;
        }
    } else {
        mark_fatal(analyzer);
    }
    analyzer->current_function = NULL;
    return !analyzer->fatal;
}

static bool analyze_bodies(SemanticAnalyzer *analyzer)
{
    size_t i;

    if (analyzer == NULL || analyzer->program == NULL) {
        mark_fatal(analyzer);
        return false;
    }

    for (i = 0; i < analyzer->program->count; i++) {
        const AstTopLevelItem *item = analyzer->program->items[i];

        if (item == NULL) {
            mark_fatal(analyzer);
            return false;
        }
        if (item->kind == AST_TOP_FUNCTION &&
            !analyze_function(analyzer, item->as.function)) {
            return false;
        }
        if (analyzer->fatal) {
            return false;
        }
    }
    return true;
}

static void transfer_diagnostics(
    SemanticAnalyzer *analyzer,
    SemanticResult *result
)
{
    if (analyzer == NULL || result == NULL) {
        return;
    }

    result->diagnostics = analyzer->diagnostics;
    result->diagnostic_count = analyzer->diagnostic_count;
    analyzer->diagnostics = NULL;
    analyzer->diagnostic_count = 0;
    analyzer->diagnostic_capacity = 0;
}

static void destroy_diagnostics(
    SemanticDiagnostic *diagnostics,
    size_t count
)
{
    size_t i;

    if (diagnostics == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(diagnostics[i].symbol_name);
    }
    free(diagnostics);
}

bool semantic_init(
    SemanticAnalyzer *analyzer,
    const AstProgram *program
)
{
    SemanticScope *global_scope;

    if (analyzer == NULL || program == NULL || analyzer->initialized ||
        analyzer->program != NULL || analyzer->global_scope != NULL ||
        analyzer->current_scope != NULL || analyzer->diagnostics != NULL) {
        return false;
    }

    global_scope = create_scope(NULL);
    if (global_scope == NULL) {
        return false;
    }

    analyzer->program = program;
    analyzer->global_scope = global_scope;
    analyzer->current_scope = global_scope;
    analyzer->initialized = true;
    return true;
}

/*
 * Registra as funcoes embutidas no escopo global (D41, D44). Como
 * compartilham o namespace, o programa nao pode declarar outra funcao,
 * variavel ou registro com esses nomes.
 */
static void declare_builtins(SemanticAnalyzer *analyzer)
{
    static const struct {
        const char *name;
        AstBuiltin builtin;
        SemanticType type;
    } builtins[] = {
        {"somavet", AST_BUILTIN_SOMAVET, SEM_TYPE_ERROR},
        {"maxvet", AST_BUILTIN_MAXVET, SEM_TYPE_ERROR},
        {"minvet", AST_BUILTIN_MINVET, SEM_TYPE_ERROR},
        {"prodesc", AST_BUILTIN_PRODESC, SEM_TYPE_ERROR},
        {"pinomodo", AST_BUILTIN_PINOMODO, SEM_TYPE_VAZIO},
        {"pinoescreve", AST_BUILTIN_PINOESCREVE, SEM_TYPE_VAZIO},
        {"pinole", AST_BUILTIN_PINOLE, SEM_TYPE_INTEIRA},
        {"espera", AST_BUILTIN_ESPERA, SEM_TYPE_VAZIO}
    };
    SourceLocation nowhere = {0, 0};
    size_t i;

    for (i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        SemanticSymbol *symbol;

        if (!declare_symbol(
                analyzer,
                builtins[i].name,
                SEM_SYMBOL_FUNCTION,
                builtins[i].type,
                nowhere,
                0,
                NULL,
                NULL
            )) {
            return;
        }
        symbol = lookup_current_scope(analyzer->global_scope, builtins[i].name);
        if (symbol == NULL) {
            mark_fatal(analyzer);
            return;
        }
        symbol->builtin = builtins[i].builtin;
    }
}

/* Soma dos tamanhos (no alvo) de todas as declaracoes de um bloco. */
static size_t block_frame_bytes(
    const AstBlock *block,
    size_t pointer_bytes
);

static size_t statement_frame_bytes(
    const AstStatement *statement,
    size_t pointer_bytes
)
{
    size_t total = 0;
    size_t i;

    if (statement == NULL) {
        return 0;
    }
    switch (statement->kind) {
    case AST_STMT_DECLARATION: {
        const AstDeclaration *declaration = statement->as.declaration;

        for (i = 0; i < declaration->declarator_count; i++) {
            total = saturating_add(
                total,
                target_value_size(
                    declaration->type.kind,
                    declaration->record,
                    declaration->declarators[i]->vector_size,
                    pointer_bytes
                )
            );
        }
        return total;
    }
    case AST_STMT_BLOCK:
        return block_frame_bytes(statement->as.block, pointer_bytes);
    case AST_STMT_IF:
        total = block_frame_bytes(
            statement->as.if_statement->then_block,
            pointer_bytes
        );
        for (i = 0; i < statement->as.if_statement->else_if_count; i++) {
            total = saturating_add(
                total,
                block_frame_bytes(
                    statement->as.if_statement->else_if_branches[i]->block,
                    pointer_bytes
                )
            );
        }
        return saturating_add(
            total,
            block_frame_bytes(
                statement->as.if_statement->else_block,
                pointer_bytes
            )
        );
    case AST_STMT_WHILE:
        return block_frame_bytes(
            statement->as.while_statement->body,
            pointer_bytes
        );
    case AST_STMT_REPEAT:
        return block_frame_bytes(
            statement->as.repeat_statement->body,
            pointer_bytes
        );
    default:
        return 0;
    }
}

static size_t block_frame_bytes(
    const AstBlock *block,
    size_t pointer_bytes
)
{
    size_t total = 0;
    size_t i;

    if (block == NULL) {
        return 0;
    }
    for (i = 0; i < block->statement_count; i++) {
        total = saturating_add(
            total,
            statement_frame_bytes(block->statements[i], pointer_bytes)
        );
    }
    return total;
}

/*
 * Verificacao de memoria por alvo (D45). O gerador de codigo coloca todas
 * as variaveis locais de uma funcao no bloco de entrada (alloca), entao o
 * quadro de uma funcao e a soma de TODAS as suas declaracoes, inclusive as
 * de blocos internos, mais os parametros. Globais e textos ficam na RAM
 * durante todo o programa. No Uno: globais + quadro <= 2048 - 512; na
 * Pico 2 W: globais + quadro <= 524288 - 65536 (D48).
 */
static void check_target_memory(SemanticAnalyzer *analyzer)
{
    const SemanticTargetInfo info = semantic_target_info(analyzer->target);
    const size_t limit = info.ram_bytes - info.reserved_bytes;
    const size_t pointer_bytes = target_pointer_bytes(analyzer->target);
    const bool checked = info.ram_bytes > 0;
    const bool pico = analyzer->target == SEMANTIC_TARGET_PICO;
    size_t i;

    for (i = 0; i < analyzer->program->count; i++) {
        const AstTopLevelItem *item = analyzer->program->items[i];
        const AstFunctionDecl *function;
        size_t frame;
        size_t p;

        if (item->kind != AST_TOP_FUNCTION) {
            continue;
        }
        function = item->as.function;
        frame = block_frame_bytes(function->body, pointer_bytes);
        for (p = 0; p < function->parameter_count; p++) {
            frame = saturating_add(
                frame,
                target_scalar_size(
                    function->parameters[p]->type.kind,
                    pointer_bytes
                )
            );
        }
        if (frame > analyzer->max_frame_bytes) {
            analyzer->max_frame_bytes = frame;
        }
        if (checked &&
            analyzer->global_bytes <= limit &&
            saturating_add(analyzer->global_bytes, frame) > limit) {
            add_diagnostic(
                analyzer,
                SEM_TARGET_MEMORY,
                function->location,
                function->is_main ? "principal" : function->name,
                pico ? MESSAGE_TARGET_FRAME_PICO : MESSAGE_TARGET_FRAME_UNO
            );
        }
    }
    if (checked && analyzer->global_bytes > limit) {
        SourceLocation start = {1, 1};

        add_diagnostic(
            analyzer,
            SEM_TARGET_MEMORY,
            start,
            NULL,
            pico ? MESSAGE_TARGET_GLOBALS_PICO : MESSAGE_TARGET_GLOBALS_UNO
        );
    }
}

SemanticTargetInfo semantic_target_info(SemanticTarget target)
{
    SemanticTargetInfo info = {"pc", 0, 0};

    if (target == SEMANTIC_TARGET_UNO) {
        info.name = "uno";
        info.ram_bytes = SEMANTIC_UNO_RAM_BYTES;
        info.reserved_bytes = SEMANTIC_UNO_RESERVED_BYTES;
    } else if (target == SEMANTIC_TARGET_PICO) {
        info.name = "pico";
        info.ram_bytes = SEMANTIC_PICO_RAM_BYTES;
        info.reserved_bytes = SEMANTIC_PICO_RESERVED_BYTES;
    }
    return info;
}

void semantic_set_target(SemanticAnalyzer *analyzer, SemanticTarget target)
{
    if (analyzer != NULL) {
        analyzer->target = target;
    }
}

SemanticResult semantic_analyze(SemanticAnalyzer *analyzer)
{
    SemanticResult result = {0};
    size_t main_count = 0;

    if (analyzer == NULL || !analyzer->initialized || analyzer->analyzed) {
        result.status = SEMANTIC_INTERNAL_FAILURE;
        return result;
    }
    analyzer->analyzed = true;

    if (analyzer->global_scope == NULL ||
        analyzer->current_scope != analyzer->global_scope) {
        mark_fatal(analyzer);
    }
    if (!analyzer->fatal) {
        declare_builtins(analyzer);
    }
    if (!analyzer->fatal) {
        collect_globals(analyzer, &main_count);
    }
    if (!analyzer->fatal) {
        analyze_bodies(analyzer);
    }
    if (!analyzer->fatal && analyzer->diagnostic_count == 0) {
        check_target_memory(analyzer);
    }

    while (analyzer->current_scope != NULL &&
           analyzer->current_scope != analyzer->global_scope) {
        if (!scope_pop(analyzer)) {
            mark_fatal(analyzer);
            break;
        }
    }
    analyzer->current_function = NULL;
    result.symbol_count = analyzer->next_symbol_id;
    result.global_bytes = analyzer->global_bytes;
    result.max_frame_bytes = analyzer->max_frame_bytes;

    if (analyzer->fatal) {
        result.status = SEMANTIC_INTERNAL_FAILURE;
        transfer_diagnostics(analyzer, &result);
    } else if (analyzer->diagnostic_count != 0) {
        result.status = SEMANTIC_ERRORS;
        transfer_diagnostics(analyzer, &result);
    } else {
        result.status = SEMANTIC_OK;
    }
    return result;
}

void semantic_destroy(SemanticAnalyzer *analyzer)
{
    if (analyzer == NULL) {
        return;
    }

    while (analyzer->current_scope != NULL &&
           analyzer->current_scope != analyzer->global_scope) {
        SemanticScope *scope = analyzer->current_scope;
        analyzer->current_scope = scope->parent;
        destroy_scope(scope);
    }
    destroy_scope(analyzer->global_scope);
    destroy_diagnostics(analyzer->diagnostics, analyzer->diagnostic_count);
    *analyzer = (SemanticAnalyzer){0};
}

void semantic_result_destroy(SemanticResult *result)
{
    if (result == NULL) {
        return;
    }

    destroy_diagnostics(result->diagnostics, result->diagnostic_count);
    *result = (SemanticResult){0};
}
