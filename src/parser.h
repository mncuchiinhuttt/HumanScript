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
    std::vector<Token> tokens_list; 
    size_t current_token_idx = 0;
    
    Token peek();
    Token advance();
    Token consume(TokenType type, const std::string& message);
    bool match(TokenType type); 

    std::unique_ptr<UseNode> parse_use_declaration();
    std::string parse_header_path(); 

    std::unique_ptr<StatementNode> parse_statement();
    std::unique_ptr<VariableDeclarationNode> parse_variable_declaration_statement();
    std::unique_ptr<SaysStatementNode> parse_says_statement();
    std::unique_ptr<IfStatementNode> parse_if_statement();
    std::unique_ptr<BlockStatementNode> parse_block_statement();
    
    std::unique_ptr<ExprNode> parse_expression();       
    std::unique_ptr<ExprNode> parse_comparison();       
    std::unique_ptr<ExprNode> parse_addition();         
    std::unique_ptr<ExprNode> parse_factor();          
};