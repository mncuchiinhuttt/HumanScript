#include "semantic_analyzer.h"
#include <iostream> // For error/info messages

SemanticAnalyzer::SemanticAnalyzer() {}

void SemanticAnalyzer::analyze(const ProgramNode* program) {
    symbol_table.clear(); 

    for (const auto& use_decl : program->use_declarations) {
        std::cout << "Semantic Info: Processing 'use <" << use_decl->header_name << ">;' declaration." << std::endl;
    }

    for (const auto& stmt : program->statements) {
        visit(stmt.get()); 
    }
}

// --- Statement Visitors ---
void SemanticAnalyzer::visit(const StatementNode* stmt) {
    if (auto var_decl_stmt = dynamic_cast<const VariableDeclarationNode*>(stmt)) {
        visit(var_decl_stmt);
    } else if (auto says_stmt = dynamic_cast<const SaysStatementNode*>(stmt)) {
        visit(says_stmt);
    }
    // else if (auto assign_stmt = dynamic_cast<const AssignmentNode*>(stmt)) {
    //     visit(assign_stmt);
    // }
    else {
        throw std::runtime_error("Semantic Analyzer: Unknown statement type encountered.");
    }
}

void SemanticAnalyzer::visit(const VariableDeclarationNode* stmt) {
    std::string var_name = stmt->identifier_name;

    if (symbol_table.count(var_name)) {
        throw std::runtime_error("Semantic Error: Variable '" + var_name + "' already declared in this scope.");
    }

    // Analyze the initializer expression and get its type
    HScriptType initializer_expr_type = visit_and_get_type(stmt->expression.get());

    // Check type compatibility for assignment
    if (!is_assignable(stmt->var_type, initializer_expr_type)) {
        throw std::runtime_error("Semantic Error: Type mismatch in variable declaration of '" + var_name +
                                 "'. Cannot assign type " + hscript_type_to_string(initializer_expr_type) +
                                 " to variable of type " + hscript_type_to_string(stmt->var_type) + ".");
    }
    
    // Add to symbol table
    symbol_table.emplace(var_name, Symbol(var_name, stmt->var_type));
    std::cout << "Semantic Info: Declared variable '" << var_name << "' of type " << hscript_type_to_string(stmt->var_type) << std::endl;
}

void SemanticAnalyzer::visit(const SaysStatementNode* stmt) {
    // Analyze the expression to be printed. Its type doesn't restrict 'says' much,
    // but we still need to visit it to ensure its internal validity and set expr_types.
    HScriptType expr_type = visit_and_get_type(stmt->expression.get());
    if (expr_type == HScriptType::VOID || expr_type == HScriptType::UNKNOWN) {
        throw std::runtime_error("Semantic Error: 'says' statement cannot print an expression of type void or unknown.");
    }
    // `says` can print any valid, non-void type.
    std::cout << "Semantic Info: 'says' statement with expression of type " << hscript_type_to_string(expr_type) << std::endl;
}

// --- Expression Visitors (evaluating and returning type) ---

// This is the mutable version because it sets expr_type on the AST node
HScriptType SemanticAnalyzer::visit_and_get_type(const ExprNode* expr_const) {
    // We need to modify expr_type, so we cast away const. This is generally
    // not ideal, but common in AST visitors that annotate the tree.
    // Ensure this method is only called with ExprNodes we own and can modify.
    ExprNode* expr = const_cast<ExprNode*>(expr_const);

    if (auto int_lit = dynamic_cast<const IntegerLiteralNode*>(expr)) {
        expr->expr_type = visit_and_get_type(int_lit); // Call specific overload
    } else if (auto dbl_lit = dynamic_cast<const DoubleLiteralNode*>(expr)) {
        expr->expr_type = visit_and_get_type(dbl_lit);
    } else if (auto str_lit = dynamic_cast<const StringLiteralNode*>(expr)) {
        expr->expr_type = visit_and_get_type(str_lit);
    } else if (auto bool_lit = dynamic_cast<const BooleanLiteralNode*>(expr)) {
        expr->expr_type = visit_and_get_type(bool_lit);
    } else if (auto ident = dynamic_cast<const IdentifierNode*>(expr)) {
        expr->expr_type = visit_and_get_type(ident);
    } else if (auto bin_op = dynamic_cast<const BinaryOpNode*>(expr)) {
        expr->expr_type = visit_and_get_type(bin_op);
    } else {
        throw std::runtime_error("Semantic Analyzer: Unknown expression node type.");
    }
    return expr->expr_type;
}


HScriptType SemanticAnalyzer::visit_and_get_type(const IntegerLiteralNode* expr) {
    // Lexer produces INTEGER_LITERAL which stores long long.
    // For HumanScript, a plain integer literal without context could be 'number' or 'lnumber'.
    // For simplicity, let's say an integer literal by itself is compatible with 'number' and 'lnumber'.
    // The actual AST node type `IntegerLiteralNode` already has `expr_type = HScriptType::LNUMBER` by default.
    // We could refine this if strict 'number' vs 'lnumber' distinction for literals is needed.
    // For now, LNUMBER is fine as it can be assigned to both.
    return HScriptType::LNUMBER; // It's a supertype that can be implicitly converted
}

HScriptType SemanticAnalyzer::visit_and_get_type(const DoubleLiteralNode* expr) {
    return HScriptType::RIEL;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const StringLiteralNode* expr) {
    return HScriptType::TEXT;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const BooleanLiteralNode* expr) {
    return HScriptType::LOGIC;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const IdentifierNode* expr) {
    std::string var_name = expr->name;
    if (symbol_table.find(var_name) == symbol_table.end()) {
        throw std::runtime_error("Semantic Error: Variable '" + var_name + "' used before declaration.");
    }
    // Potentially check if initialized if we support declaration without init
    return symbol_table.at(var_name).type;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const BinaryOpNode* expr_const) {
     BinaryOpNode* expr = const_cast<BinaryOpNode*>(expr_const); // To set expr_type

    HScriptType left_type = visit_and_get_type(expr->left.get());
    HScriptType right_type = visit_and_get_type(expr->right.get());
    TokenType op_type = expr->op_token.type;

    expr->expr_type = get_binary_op_result_type(left_type, right_type, op_type);
    if (expr->expr_type == HScriptType::UNKNOWN) {
        throw std::runtime_error("Semantic Error: Invalid operands for binary operator '" + expr->op_token.text +
                                 "'. Left type: " + hscript_type_to_string(left_type) +
                                 ", Right type: " + hscript_type_to_string(right_type) + ".");
    }
    return expr->expr_type;
}


// --- Type System Helpers ---

bool SemanticAnalyzer::is_assignable(HScriptType target_type, HScriptType value_type) {
    if (target_type == value_type) return true;

    // Implicit promotions/conversions allowed:
    if (target_type == HScriptType::LNUMBER && value_type == HScriptType::NUMBER) return true;
    if (target_type == HScriptType::RIEL && (value_type == HScriptType::NUMBER || value_type == HScriptType::LNUMBER)) return true;
    // No other implicit conversions for assignment in this simple version.
    // e.g., number to text would require an explicit conversion function.

    return false;
}

HScriptType SemanticAnalyzer::get_binary_op_result_type(HScriptType left_type, HScriptType right_type, TokenType op_token_type) {
    // Arithmetic operators (+, extend for -, *, /)
    if (op_token_type == TokenType::PLUS) {
        // Numeric addition: result type is the "larger" of the two
        if ((left_type == HScriptType::NUMBER || left_type == HScriptType::LNUMBER || left_type == HScriptType::RIEL) &&
            (right_type == HScriptType::NUMBER || right_type == HScriptType::LNUMBER || right_type == HScriptType::RIEL)) {
            if (left_type == HScriptType::RIEL || right_type == HScriptType::RIEL) return HScriptType::RIEL;
            if (left_type == HScriptType::LNUMBER || right_type == HScriptType::LNUMBER) return HScriptType::LNUMBER;
            return HScriptType::NUMBER;
        }
        // String concatenation
        if (left_type == HScriptType::TEXT && right_type == HScriptType::TEXT) {
            return HScriptType::TEXT;
        }
        // Allow text + (number/lnumber/riel/logic) -> text (implicit conversion to string)
        if (left_type == HScriptType::TEXT && (right_type != HScriptType::VOID && right_type != HScriptType::UNKNOWN)) {
            return HScriptType::TEXT;
        }
        if (right_type == HScriptType::TEXT && (left_type != HScriptType::VOID && left_type != HScriptType::UNKNOWN)) {
            return HScriptType::TEXT;
        }
    }
    // Comparison operator (?=, extend for <, >, <=, >=, !=)
    else if (op_token_type == TokenType::QUESTION_EQUALS) {
        // Allow comparison between any two same basic types or compatible numeric types
        if (left_type == right_type && left_type != HScriptType::VOID && left_type != HScriptType::UNKNOWN) return HScriptType::LOGIC;
        if ((left_type == HScriptType::NUMBER || left_type == HScriptType::LNUMBER || left_type == HScriptType::RIEL) &&
            (right_type == HScriptType::NUMBER || right_type == HScriptType::LNUMBER || right_type == HScriptType::RIEL)) {
            return HScriptType::LOGIC; // Numeric types are comparable
        }
    }

    return HScriptType::UNKNOWN; // Operator not defined for these types
}