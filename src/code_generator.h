#pragma once
#include "ast.h"
#include <string>
#include <sstream> // For building the output string
#include <stdexcept> // For runtime_error

class CodeGenerator {
public:
    CodeGenerator();
    std::string generate(const ProgramNode* program);

private:
    std::stringstream output_stream;
    bool iostream_included = false; // Track if <iostream> has been included

    // Helper to get C++ type string from HScriptType
    std::string hscript_type_to_cpp_type(HScriptType type);

    // Helper to generate C++ code for an expression, ensuring it's suitable for context
    std::string generate_cpp_for_expression(const ExprNode* expr, HScriptType expected_context_type = HScriptType::UNKNOWN);

    // Statement code generation
    void visit(const StatementNode* stmt);
    void visit(const VariableDeclarationNode* stmt);
    void visit(const SaysStatementNode* stmt);
    void visit(const IfStatementNode* stmt);
    void visit(const BlockStatementNode* stmt);
    // void visit(const AssignmentNode* stmt); // If added later

    // Expression code generation (internal, called by generate_cpp_for_expression)
    std::string generate_expr_code(const IntegerLiteralNode* expr);
    std::string generate_expr_code(const DoubleLiteralNode* expr);
    std::string generate_expr_code(const StringLiteralNode* expr);
    std::string generate_expr_code(const BooleanLiteralNode* expr);
    std::string generate_expr_code(const IdentifierNode* expr);
    std::string generate_expr_code(const BinaryOpNode* expr);
};