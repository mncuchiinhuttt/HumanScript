#include "parser.h"
#include <iostream>

Parser::Parser(std::vector<Token> tokens) : tokens_list(std::move(tokens)) {}

Token Parser::peek() {
    if (current_token_idx >= tokens_list.size()) {
        return Token(TokenType::END_OF_FILE, ""); 
    }
    return tokens_list[current_token_idx];
}

Token Parser::advance() {
    if (current_token_idx < tokens_list.size()) {
        return tokens_list[current_token_idx++];
    }
    return tokens_list.back(); 
}

Token Parser::consume(TokenType type, const std::string& message) {
    Token current_token = peek();
    if (current_token.type == type) {
        return advance();
    }
    
    std::string error_msg = "Parser Error: " + message + ". Got token type " +
                            std::to_string(static_cast<int>(current_token.type)) +
                            " ('" + current_token.text + "') instead of expected type " +
                            std::to_string(static_cast<int>(type)) + ".";
    throw std::runtime_error(error_msg);
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

std::unique_ptr<ProgramNode> Parser::parse_program() {
    auto program_node = std::make_unique<ProgramNode>();

    while (peek().type == TokenType::KEYWORD_USE) {
        program_node->use_declarations.push_back(parse_use_declaration()); 
    }

    while (peek().type != TokenType::END_OF_FILE && peek().type != TokenType::UNKNOWN) {
        try {
            if (peek().type == TokenType::KEYWORD_NUMBER || peek().type == TokenType::KEYWORD_LNUMBER ||
                peek().type == TokenType::KEYWORD_TEXT || peek().type == TokenType::KEYWORD_LOGIC ||
                peek().type == TokenType::KEYWORD_RIEL || peek().type == TokenType::KEYWORD_SAYS) {
                 program_node->statements.push_back(parse_statement());
            }
            
            else {
                if (peek().type != TokenType::END_OF_FILE) { 
                    throw std::runtime_error("Parser Error: Unexpected token '" +   peek().text + "' found at top level after 'use' declarations.");
                }
                break;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            throw; 
        }
    }
    if (peek().type == TokenType::UNKNOWN && !tokens_list.empty() && tokens_list.front().type != TokenType::END_OF_FILE) {
         throw std::runtime_error("Parser Error: Encountered UNKNOWN token from lexer. Stopping.");
    }
    return program_node;
}

std::unique_ptr<StatementNode> Parser::parse_statement() {
    TokenType current_type = peek().type;

    if (current_type == TokenType::KEYWORD_NUMBER || current_type == TokenType::KEYWORD_LNUMBER ||
        current_type == TokenType::KEYWORD_TEXT || current_type == TokenType::KEYWORD_LOGIC ||
        current_type == TokenType::KEYWORD_RIEL) {
        return parse_variable_declaration_statement();
    } else if (current_type == TokenType::KEYWORD_SAYS) {
        return parse_says_statement();
    }
    
    else {
        throw std::runtime_error("Parser Error: Unexpected token '" + peek().text + "' at start of a statement.");
    }
}

std::unique_ptr<VariableDeclarationNode> Parser::parse_variable_declaration_statement() {
    Token type_token = advance(); 
    HScriptType var_hscript_type;

    switch (type_token.type) {
        case TokenType::KEYWORD_NUMBER:  var_hscript_type = HScriptType::NUMBER;  break;
        case TokenType::KEYWORD_LNUMBER: var_hscript_type = HScriptType::LNUMBER; break;
        case TokenType::KEYWORD_TEXT:    var_hscript_type = HScriptType::TEXT;    break;
        case TokenType::KEYWORD_LOGIC:   var_hscript_type = HScriptType::LOGIC;   break;
        case TokenType::KEYWORD_RIEL:    var_hscript_type = HScriptType::RIEL;    break;
        default: 
            throw std::runtime_error("Parser Internal Error: Invalid type keyword in var declaration.");
    }

    Token identifier_token = consume(TokenType::IDENTIFIER, "Expected identifier name after type keyword");
    consume(TokenType::COLON_EQUALS, "Expected ':=' after identifier in variable declaration");
    
    std::unique_ptr<ExprNode> expr = parse_expression();
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration expression");

    return std::make_unique<VariableDeclarationNode>(var_hscript_type, std::get<std::string>(identifier_token.value), std::move(expr));
}

std::unique_ptr<SaysStatementNode> Parser::parse_says_statement() {
    consume(TokenType::KEYWORD_SAYS, "Expected 'says' keyword");
    std::unique_ptr<ExprNode> expr = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after 'says' statement expression");
    return std::make_unique<SaysStatementNode>(std::move(expr));
}

std::unique_ptr<ExprNode> Parser::parse_expression() {
    return parse_comparison();
}

std::unique_ptr<ExprNode> Parser::parse_comparison() {
    std::unique_ptr<ExprNode> left = parse_addition();

    while (peek().type == TokenType::QUESTION_EQUALS) {
        Token operator_token = advance(); 
        std::unique_ptr<ExprNode> right = parse_addition();
        left = std::make_unique<BinaryOpNode>(std::move(left), operator_token, std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_addition() {
    std::unique_ptr<ExprNode> left = parse_factor();

    while (peek().type == TokenType::PLUS) { 
        Token operator_token = advance(); 
        std::unique_ptr<ExprNode> right = parse_factor();
        left = std::make_unique<BinaryOpNode>(std::move(left), operator_token, std::move(right));
    }
    return left;
}

std::string Parser::parse_header_path() {
    std::string path_str;
    while (peek().type != TokenType::GT && peek().type != TokenType::END_OF_FILE) {
        Token current_part = advance();
        if (current_part.type == TokenType::IDENTIFIER ||
            current_part.type == TokenType::DOT ||
            current_part.type == TokenType::SLASH ||
            current_part.type == TokenType::INTEGER_LITERAL || 
            current_part.text == "h" ) {
            path_str += current_part.text;
        } else {
            throw std::runtime_error("Parser Error: Invalid token '" + current_part.text + "' inside use <...> path.");
        }
    }
    if (path_str.empty()) {
        throw std::runtime_error("Parser Error: Empty path in use <...> statement.");
    }
    return path_str;
}

std::unique_ptr<UseNode> Parser::parse_use_declaration() {
    consume(TokenType::KEYWORD_USE, "Expected 'use' keyword.");
    consume(TokenType::LT, "Expected '<' after 'use' keyword.");
    std::string header_name = parse_header_path();
    consume(TokenType::GT, "Expected '>' after include path in 'use' statement.");
    consume(TokenType::SEMICOLON, "Expected ';' after 'use' statement.");
    
    return std::make_unique<UseNode>(header_name, true /* is_system_include */);
}


std::unique_ptr<ExprNode> Parser::parse_factor() {
    Token current_token = peek();

    if (current_token.type == TokenType::INTEGER_LITERAL) {
        advance();
        
        if (std::holds_alternative<long long>(current_token.value)) {
            return std::make_unique<IntegerLiteralNode>(std::get<long long>(current_token.value));
        } else if (std::holds_alternative<int>(current_token.value)) {
             return std::make_unique<IntegerLiteralNode>(static_cast<long long>(std::get<int>(current_token.value)));
        }
        
        throw std::runtime_error("Parser Error: Expected int or long long value in INTEGER_LITERAL token.");

    } else if (current_token.type == TokenType::DOUBLE_LITERAL) {
        advance();
        return std::make_unique<DoubleLiteralNode>(std::get<double>(current_token.value));
    } else if (current_token.type == TokenType::STRING_LITERAL) {
        advance();
        return std::make_unique<StringLiteralNode>(std::get<std::string>(current_token.value));
    } else if (current_token.type == TokenType::KEYWORD_TRUE) {
        advance();
        return std::make_unique<BooleanLiteralNode>(true);
    } else if (current_token.type == TokenType::KEYWORD_FALSE) {
        advance();
        return std::make_unique<BooleanLiteralNode>(false);
    } else if (current_token.type == TokenType::IDENTIFIER) {
        advance();
        return std::make_unique<IdentifierNode>(std::get<std::string>(current_token.value));
    } else if (current_token.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, "Expected '(' for grouped expression");
        std::unique_ptr<ExprNode> expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after grouped expression");
        return expr;
    } else {
        throw std::runtime_error("Parser Error: Unexpected token '" + current_token.text +
                                 "' when expecting a factor (literal, identifier, or parentheses).");
    }
}