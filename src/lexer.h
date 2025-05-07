#pragma once
#include <string>
#include <vector>
#include <variant>

// Forward declare HScriptType for Token value
enum class HScriptType;

enum class TokenType {
    KEYWORD_NUMBER,    // "number"
    KEYWORD_LNUMBER,   // "lnumber"
    KEYWORD_TEXT,      // "text"
    KEYWORD_LOGIC,     // "logic"
    KEYWORD_RIEL,      // "riel"
    KEYWORD_SAYS,      // "says"
    KEYWORD_TRUE,      // "true"
    KEYWORD_FALSE,     // "false"
    KEYWORD_USE,       // "use"

    LT,               // "<"
    GT,               // ">"
    // DOT, SLASH for paths - already added if you kept them from previous "use" attempt
    DOT,              // "."
    SLASH,            // "/" (for paths within include, e.g. <sys/socket.h>)

    // Identifiers
    IDENTIFIER,

    // Literals
    INTEGER_LITERAL,
    DOUBLE_LITERAL,
    STRING_LITERAL,

    // Operators
    COLON_EQUALS,    // ":="
    QUESTION_EQUALS, // "?="
    PLUS,            // "+"
    // Add other operators like MINUS, MULTIPLY, DIVIDE as needed

    // Punctuation
    SEMICOLON,       // ";"
    LPAREN,          // "("
    RPAREN,          // ")"
    // No <, > for "use" yet, assuming it's removed for this fresh start

    // Meta
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string text; // The raw text of the token
    // Using std::any or a more complex variant if many literal types
    // For now, let's use a variant for common literal values.
    std::variant<std::monostate, int, long long, double, std::string, bool> value;

    Token(TokenType t, std::string txt = "");
    // Constructor for literals with values
    Token(TokenType t, std::string txt, std::variant<std::monostate, int, long long, double, std::string, bool> val);
};

class Lexer {
public:
    Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_code;
    size_t current_pos = 0;
    size_t line_number = 1; // For error reporting (optional for now)

    char peek();
    char peek_next();
    char advance();
    void skip_whitespace_and_comments();
    Token get_next_token();
    Token make_identifier_or_keyword(const std::string& ident_text);
    Token make_number();
    Token make_string_literal();
};