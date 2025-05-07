#pragma once
#include "ast.h"
#include <string>
#include <unordered_map> 
#include <stdexcept>     
#include <set>           

struct Symbol {
    std::string name;
    HScriptType type;
    bool is_initialized; 

    Symbol(std::string n, HScriptType t, bool init = true) : name(std::move(n)), type(t), is_initialized(init) {}
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(const ProgramNode* program);

private:
    std::unordered_map<std::string, Symbol> symbol_table;
    
    void visit(const StatementNode* stmt);
    void visit(const VariableDeclarationNode* stmt);
    void visit(const SaysStatementNode* stmt);

    HScriptType visit_and_get_type(const ExprNode* expr); 
    HScriptType visit_and_get_type(const IntegerLiteralNode* expr);
    HScriptType visit_and_get_type(const DoubleLiteralNode* expr);
    HScriptType visit_and_get_type(const StringLiteralNode* expr);
    HScriptType visit_and_get_type(const BooleanLiteralNode* expr);
    HScriptType visit_and_get_type(const IdentifierNode* expr);
    HScriptType visit_and_get_type(const BinaryOpNode* expr);

    bool is_assignable(HScriptType target_type, HScriptType value_type);
    HScriptType get_binary_op_result_type(HScriptType left_type, HScriptType right_type, TokenType op_type);
};