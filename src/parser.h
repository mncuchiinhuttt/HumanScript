#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept> 

class Parser {
public:
    Parser(std::vector<Token> tokens);
    std::unique_ptr<ProgramNode> parse_program();

private:
    std::vector<Token> tokens_list; // Renamed to avoid conflict with a potential 'tokens' member
    size_t current_token_idx = 0;

    // Helper methods
    Token peek();
    Token advance();
    Token consume(TokenType type, const std::string& message);
    bool match(TokenType type); // Checks current token and advances if it matches

    std::unique_ptr<UseNode> parse_use_declaration();
    std::string parse_header_path(); // Helper for content inside <...>

    // Parsing methods for different grammar rules
    std::unique_ptr<StatementNode> parse_statement();
    std::unique_ptr<VariableDeclarationNode> parse_variable_declaration_statement();
    std::unique_ptr<SaysStatementNode> parse_says_statement();
    
    // Expression parsing (operator precedence will be handled here)
    std::unique_ptr<ExprNode> parse_expression();       // Entry point for expressions
    std::unique_ptr<ExprNode> parse_comparison();       // Handles '?='
    std::unique_ptr<ExprNode> parse_addition();         // Handles '+'
    std::unique_ptr<ExprNode> parse_factor();           // Handles literals, identifiers, (expr)
};