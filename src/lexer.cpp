#include "lexer.h"
#include <cctype>
#include <iostream> // For errors
#include <unordered_map>

// Token constructors
Token::Token(TokenType t, std::string txt) : type(t), text(std::move(txt)), value(std::monostate{}) {}
Token::Token(TokenType t, std::string txt, std::variant<std::monostate, int, long long, double, std::string, bool> val)
    : type(t), text(std::move(txt)), value(std::move(val)) {}


Lexer::Lexer(std::string source) : source_code(std::move(source)) {}

char Lexer::peek() {
    if (current_pos >= source_code.length()) return '\0';
    return source_code[current_pos];
}

char Lexer::peek_next() {
    if (current_pos + 1 >= source_code.length()) return '\0';
    return source_code[current_pos + 1];
}

char Lexer::advance() {
    if (current_pos < source_code.length()) {
        char current_char = source_code[current_pos];
        current_pos++;
        if (current_char == '\n') line_number++;
        return current_char;
    }
    return '\0';
}

void Lexer::skip_whitespace_and_comments() {
    while (current_pos < source_code.length()) {
        char current_char = peek();
        if (std::isspace(current_char)) {
            advance();
        } else if (current_char == '/' && peek_next() == '/') { // Single line comment
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::make_identifier_or_keyword(const std::string& ident_text) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"number", TokenType::KEYWORD_NUMBER}, {"lnumber", TokenType::KEYWORD_LNUMBER},
        {"text", TokenType::KEYWORD_TEXT},     {"logic", TokenType::KEYWORD_LOGIC},
        {"riel", TokenType::KEYWORD_RIEL},     {"says", TokenType::KEYWORD_SAYS},
        {"true", TokenType::KEYWORD_TRUE},     {"false", TokenType::KEYWORD_FALSE},
        {"use", TokenType::KEYWORD_USE},       {"if", TokenType::KEYWORD_IF},
        {"else", TokenType::KEYWORD_ELSE}
    };

    auto it = keywords.find(ident_text);
    if (it != keywords.end()) {
        if (ident_text == "true") return Token(it->second, ident_text, true);
        if (ident_text == "false") return Token(it->second, ident_text, false);
        return Token(it->second, ident_text);
    }
    return Token(TokenType::IDENTIFIER, ident_text, ident_text); // Store name in value for identifiers
}

Token Lexer::make_number() {
    std::string num_str;
    bool is_double = false;
    while (std::isdigit(peek()) || (peek() == '.' && !is_double && std::isdigit(peek_next()))) {
        if (peek() == '.') {
            is_double = true;
        }
        num_str += advance();
    }

    if (is_double) {
        try {
            return Token(TokenType::DOUBLE_LITERAL, num_str, std::stod(num_str));
        } catch (const std::out_of_range&) {
            std::cerr << "Lexer Warning: Double literal '" << num_str << "' out of range." << std::endl;
            return Token(TokenType::DOUBLE_LITERAL, num_str, 0.0); // Default or error
        }
    } else {
        try {
            // For simplicity, lexing as int. Parser/Semantic can decide if it fits 'number' or 'lnumber' context
            // or we could attempt to parse as long long here if 'L' suffix was supported.
            // For now, all whole numbers are INTEGER_LITERAL.
            return Token(TokenType::INTEGER_LITERAL, num_str, std::stoi(num_str));
        } catch (const std::out_of_range&) {
             try {
                return Token(TokenType::INTEGER_LITERAL, num_str, std::stoll(num_str)); // Try as long long
            } catch (const std::out_of_range&) {
                std::cerr << "Lexer Warning: Integer literal '" << num_str << "' out of range for long long." << std::endl;
                return Token(TokenType::INTEGER_LITERAL, num_str, 0LL); // Default or error
            }
        }
    }
}

Token Lexer::make_string_literal() {
    std::string str_val;
    advance(); // Consume the opening quote
    while (peek() != '"' && peek() != '\0') {
        // Handle escape sequences if needed (e.g., \n, \t, \")
        if (peek() == '\\') {
            advance(); // consume backslash
            switch(peek()) {
                case 'n': str_val += '\n'; break;
                case 't': str_val += '\t'; break;
                case '"': str_val += '"'; break;
                case '\\': str_val += '\\'; break;
                default: str_val += peek(); // store the char after backslash literally
            }
        } else {
            str_val += peek();
        }
        advance();
    }
    if (peek() == '"') {
        advance(); // Consume the closing quote
    } else {
        std::cerr << "Lexer Error: Unterminated string literal." << std::endl;
        // Return an error token or handle differently
    }
    return Token(TokenType::STRING_LITERAL, "\"" + str_val + "\"", str_val);
}

Token Lexer::get_next_token() {
    skip_whitespace_and_comments();
    char current_char = peek();

    if (current_char == '\0') return Token(TokenType::END_OF_FILE, "");

    if (std::isalpha(current_char) || current_char == '_') {
        std::string ident_text;
        while (std::isalnum(peek()) || peek() == '_') {
            ident_text += advance();
        }
        return make_identifier_or_keyword(ident_text);
    }

    if (std::isdigit(current_char) || (current_char == '.' && std::isdigit(peek_next()))) {
        return make_number();
    }
    
    if (current_char == '"') {
        return make_string_literal();
    }

    switch (current_char) {
        case ':':
            if (peek_next() == '=') {
                advance(); advance(); return Token(TokenType::COLON_EQUALS, ":=");
            }
            break; // Or handle single ':' as error or other token
        case '?':
            if (peek_next() == '=') {
                advance(); advance(); return Token(TokenType::QUESTION_EQUALS, "?=");
            }
            break;
        case '+': advance(); return Token(TokenType::PLUS, "+");
        case ';': advance(); return Token(TokenType::SEMICOLON, ";");
        case '(': advance(); return Token(TokenType::LPAREN, "(");
        case ')': advance(); return Token(TokenType::RPAREN, ")");
        case '{': advance(); return Token(TokenType::LBRACE, "{");
        case '}': advance(); return Token(TokenType::RBRACE, "}");
        case '<': advance(); return Token(TokenType::LT, "<");
        case '>': advance(); return Token(TokenType::GT, ">");
        case '.': advance(); return Token(TokenType::DOT, ".");
        case '/': advance(); return Token(TokenType::SLASH, "/");
    }

    // If no match
    std::cerr << "Lexer Error: Unknown character '" << current_char << "' on line " << line_number << std::endl;
    advance();
    return Token(TokenType::UNKNOWN, std::string(1, current_char));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token = get_next_token();
    while (token.type != TokenType::END_OF_FILE && token.type != TokenType::UNKNOWN) {
        tokens.push_back(token);
        token = get_next_token();
    }
    if (token.type == TokenType::UNKNOWN) tokens.push_back(token); // include last unknown token for error reporting
    tokens.push_back(Token(TokenType::END_OF_FILE, "")); // Add EOF
    return tokens;
}