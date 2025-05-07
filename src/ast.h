#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "lexer.h" // For Token

// Enum to represent HumanScript types in the AST and Semantic Analyzer
enum class HScriptType {
    NUMBER, LNUMBER, TEXT, LOGIC, RIEL,
    VOID,   // For statements or functions that don't return a value
    UNKNOWN // For errors or before type deduction
};

// Helper to convert HScriptType to string for debugging/errors
inline std::string hscript_type_to_string(HScriptType type) {
    switch (type) {
        case HScriptType::NUMBER: return "number";
        case HScriptType::LNUMBER: return "lnumber";
        case HScriptType::TEXT: return "text";
        case HScriptType::LOGIC: return "logic";
        case HScriptType::RIEL: return "riel";
        case HScriptType::VOID: return "void";
        default: return "unknown_type";
    }
}


// --- Expression Nodes ---
struct ExprNode {
    HScriptType expr_type = HScriptType::UNKNOWN; // To be filled by Semantic Analyzer
    virtual ~ExprNode() = default;
    virtual std::string to_string() const = 0;
};

struct IntegerLiteralNode : ExprNode {
    long long value; // Use long long to hold both int and potential long long from lexer
    explicit IntegerLiteralNode(long long val) : value(val) { expr_type = HScriptType::LNUMBER; } // Tentative, semantic analysis will refine
    std::string to_string() const override { return std::to_string(value); }
};

struct DoubleLiteralNode : ExprNode {
    double value;
    explicit DoubleLiteralNode(double val) : value(val) { expr_type = HScriptType::RIEL; }
    std::string to_string() const override { return std::to_string(value); }
};

struct StringLiteralNode : ExprNode {
    std::string value;
    explicit StringLiteralNode(std::string val) : value(std::move(val)) { expr_type = HScriptType::TEXT; }
    std::string to_string() const override { return "\"" + value + "\""; }
};

struct BooleanLiteralNode : ExprNode {
    bool value;
    explicit BooleanLiteralNode(bool val) : value(val) { expr_type = HScriptType::LOGIC; }
    std::string to_string() const override { return value ? "true" : "false"; }
};

struct IdentifierNode : ExprNode { // Renamed from IdentifierExprNode for clarity
    std::string name;
    explicit IdentifierNode(std::string n) : name(std::move(n)) {}
    std::string to_string() const override { return name; }
};

struct BinaryOpNode : ExprNode { // Renamed from BinaryOpExprNode
    std::unique_ptr<ExprNode> left;
    Token op_token; // Contains operator type (e.g., PLUS, QUESTION_EQUALS)
    std::unique_ptr<ExprNode> right;

    BinaryOpNode(std::unique_ptr<ExprNode> l, Token op, std::unique_ptr<ExprNode> r)
        : left(std::move(l)), op_token(op), right(std::move(r)) {}
    std::string to_string() const override {
        return "(" + left->to_string() + " " + op_token.text + " " + right->to_string() + ")";
    }
};

// --- Statement Nodes ---
struct StatementNode {
    virtual ~StatementNode() = default;
    virtual std::string to_string() const = 0;
};

// Block statement for grouped statements within braces
struct BlockStatementNode : StatementNode {
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    BlockStatementNode() {}  // Empty block
    explicit BlockStatementNode(std::vector<std::unique_ptr<StatementNode>> stmts)
        : statements(std::move(stmts)) {}
        
    std::string to_string() const override {
        std::string result = "{\n";
        for (const auto& stmt : statements) {
            result += "  " + stmt->to_string() + "\n";
        }
        result += "}";
        return result;
    }
};

// If-else statement node
struct IfStatementNode : StatementNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StatementNode> then_branch;
    std::unique_ptr<StatementNode> else_branch;  // Optional, can be nullptr
    
    IfStatementNode(std::unique_ptr<ExprNode> cond, 
                    std::unique_ptr<StatementNode> then_stmt,
                    std::unique_ptr<StatementNode> else_stmt = nullptr)
        : condition(std::move(cond)), 
          then_branch(std::move(then_stmt)), 
          else_branch(std::move(else_stmt)) {}
          
    std::string to_string() const override {
        std::string result = "if (" + condition->to_string() + ") " + 
                             then_branch->to_string();
        if (else_branch) {
            result += " else " + else_branch->to_string();
        }
        return result;
    }
};

struct VariableDeclarationNode : StatementNode {
    HScriptType var_type;
    std::string identifier_name;
    std::unique_ptr<ExprNode> expression; // Initializer

    VariableDeclarationNode(HScriptType type, std::string name, std::unique_ptr<ExprNode> expr)
        : var_type(type), identifier_name(std::move(name)), expression(std::move(expr)) {}

    std::string to_string() const override {
        return hscript_type_to_string(var_type) + " " + identifier_name + " := " +
               (expression ? expression->to_string() : " <no_initializer> ") + ";";
    }
};

struct AssignmentNode : StatementNode { // For identifier := expression; when var is already declared
    std::string identifier_name;        // For simplicity, HumanScript v0.1 might not have separate assignment
    std::unique_ptr<ExprNode> expression; // and always use declaration for assignment.
                                        // If we allow `x := 5;` after `number x := 0;`, we need this.
                                        // Let's assume for now declarations also serve as assignments.
                                        // For a more complete language, this would be distinct.
    AssignmentNode(std::string name, std::unique_ptr<ExprNode> expr)
        : identifier_name(std::move(name)), expression(std::move(expr)) {}
     std::string to_string() const override {
        return identifier_name + " := " + expression->to_string() + ";";
    }
};


struct SaysStatementNode : StatementNode {
    std::unique_ptr<ExprNode> expression;
    explicit SaysStatementNode(std::unique_ptr<ExprNode> expr) : expression(std::move(expr)) {}
    std::string to_string() const override {
        return "says " + expression->to_string() + ";";
    }
};

struct UseNode { // Not inheriting StatementNode
    std::string header_name;
    bool is_system_include;

    UseNode(std::string name, bool system = true)
        : header_name(std::move(name)), is_system_include(system) {}

    std::string to_string() const {
        return "use <" + header_name + ">;";
    }
};

// --- Program Node ---
struct ProgramNode {
    std::vector<std::unique_ptr<StatementNode>> statements;
    std::vector<std::unique_ptr<UseNode>> use_declarations;
};