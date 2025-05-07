#pragma once
#include "ast.h"
#include <string>
#include <unordered_map> // For symbol table
#include <stdexcept>     // For runtime_error
#include <set>           // For tracking declared variables

// Symbol Table Entry
struct Symbol {
    std::string name;
    HScriptType type;
    bool is_initialized; // Could be useful if we allow declaration without immediate assignment

    Symbol(std::string n, HScriptType t, bool init = true) : name(std::move(n)), type(t), is_initialized(init) {}
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(const ProgramNode* program);

private:
    // For now, a single global scope. Could be extended to a stack of scopes.
    std::unordered_map<std::string, Symbol> symbol_table;

    // Visitor methods for AST nodes (will return the HScriptType of the expression)
    void visit(const StatementNode* stmt);
    void visit(const VariableDeclarationNode* stmt);
    void visit(const SaysStatementNode* stmt);
    // void visit(const AssignmentNode* stmt); // If we add separate assignment

    HScriptType visit_and_get_type(const ExprNode* expr); // Returns the type of the expression
    HScriptType visit_and_get_type(const IntegerLiteralNode* expr);
    HScriptType visit_and_get_type(const DoubleLiteralNode* expr);
    HScriptType visit_and_get_type(const StringLiteralNode* expr);
    HScriptType visit_and_get_type(const BooleanLiteralNode* expr);
    HScriptType visit_and_get_type(const IdentifierNode* expr);
    HScriptType visit_and_get_type(const BinaryOpNode* expr);

    // Type compatibility and promotion (simple for now)
    bool is_assignable(HScriptType target_type, HScriptType value_type);
    HScriptType get_binary_op_result_type(HScriptType left_type, HScriptType right_type, TokenType op_type);
};