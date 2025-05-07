#include "semantic_analyzer.h"
#include <iostream> 

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

void SemanticAnalyzer::visit(const StatementNode* stmt) {
    if (auto var_decl_stmt = dynamic_cast<const VariableDeclarationNode*>(stmt)) {
        visit(var_decl_stmt);
    } else if (auto says_stmt = dynamic_cast<const SaysStatementNode*>(stmt)) {
        visit(says_stmt);
    } else if (auto if_stmt = dynamic_cast<const IfStatementNode*>(stmt)) {
        visit(if_stmt);
    } else if (auto block_stmt = dynamic_cast<const BlockStatementNode*>(stmt)) {
        visit(block_stmt);
    } else {
        throw std::runtime_error("Semantic Analyzer: Unknown statement type encountered.");
    }
}

void SemanticAnalyzer::visit(const VariableDeclarationNode* stmt) {
    std::string var_name = stmt->identifier_name;

    if (symbol_table.count(var_name)) {
        throw std::runtime_error("Semantic Error: Variable '" + var_name + "' already declared in this scope.");
    }
    
    HScriptType initializer_expr_type = visit_and_get_type(stmt->expression.get());

    if (!is_assignable(stmt->var_type, initializer_expr_type)) {
        throw std::runtime_error("Semantic Error: Type mismatch in variable declaration of '" + var_name +
                                 "'. Cannot assign type " + hscript_type_to_string(initializer_expr_type) +
                                 " to variable of type " + hscript_type_to_string(stmt->var_type) + ".");
    }
    
    symbol_table.emplace(var_name, Symbol(var_name, stmt->var_type));
    std::cout << "Semantic Info: Declared variable '" << var_name << "' of type " << hscript_type_to_string(stmt->var_type) << std::endl;
}

void SemanticAnalyzer::visit(const SaysStatementNode* stmt) {
    HScriptType expr_type = visit_and_get_type(stmt->expression.get());
    if (expr_type == HScriptType::VOID || expr_type == HScriptType::UNKNOWN) {
        throw std::runtime_error("Semantic Error: 'says' statement cannot print an expression of type void or unknown.");
    }
    
    std::cout << "Semantic Info: 'says' statement with expression of type " << hscript_type_to_string(expr_type) << std::endl;
}

void SemanticAnalyzer::visit(const IfStatementNode* stmt) {
    // Check condition is a logical expression
    HScriptType condition_type = visit_and_get_type(stmt->condition.get());
    
    if (condition_type != HScriptType::LOGIC) {
        throw std::runtime_error("Semantic Error: If statement condition must be of type 'logic', got " + 
                                 hscript_type_to_string(condition_type) + " instead.");
    }
    
    // Check the then branch
    visit(stmt->then_branch.get());
    
    // Check the else branch if it exists
    if (stmt->else_branch) {
        visit(stmt->else_branch.get());
    }
    
    std::cout << "Semantic Info: Processed if statement" << std::endl;
}

void SemanticAnalyzer::visit(const BlockStatementNode* stmt) {
    // For a simple language version, we're not implementing block-level scope
    // All variables are in the global scope
    
    // Visit all statements in the block
    for (const auto& s : stmt->statements) {
        visit(s.get());
    }
    
    std::cout << "Semantic Info: Processed block statement" << std::endl;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const ExprNode* expr_const) {
    ExprNode* expr = const_cast<ExprNode*>(expr_const);

    if (auto int_lit = dynamic_cast<const IntegerLiteralNode*>(expr)) {
        expr->expr_type = visit_and_get_type(int_lit); 
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
    return HScriptType::LNUMBER; 
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
    
    return symbol_table.at(var_name).type;
}

HScriptType SemanticAnalyzer::visit_and_get_type(const BinaryOpNode* expr_const) {
     BinaryOpNode* expr = const_cast<BinaryOpNode*>(expr_const); 

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

bool SemanticAnalyzer::is_assignable(HScriptType target_type, HScriptType value_type) {
    if (target_type == value_type) return true;
    
    if (target_type == HScriptType::LNUMBER && value_type == HScriptType::NUMBER) return true;
    if (target_type == HScriptType::RIEL && (value_type == HScriptType::NUMBER || value_type == HScriptType::LNUMBER)) return true;
    
    return false;
}

HScriptType SemanticAnalyzer::get_binary_op_result_type(HScriptType left_type, HScriptType right_type, TokenType op_token_type) {
    if (op_token_type == TokenType::PLUS) {
        
        if ((left_type == HScriptType::NUMBER || left_type == HScriptType::LNUMBER || left_type == HScriptType::RIEL) &&
            (right_type == HScriptType::NUMBER || right_type == HScriptType::LNUMBER || right_type == HScriptType::RIEL)) {
            if (left_type == HScriptType::RIEL || right_type == HScriptType::RIEL) return HScriptType::RIEL;
            if (left_type == HScriptType::LNUMBER || right_type == HScriptType::LNUMBER) return HScriptType::LNUMBER;
            return HScriptType::NUMBER;
        }
        
        if (left_type == HScriptType::TEXT && right_type == HScriptType::TEXT) {
            return HScriptType::TEXT;
        }
        
        if (left_type == HScriptType::TEXT && (right_type != HScriptType::VOID && right_type != HScriptType::UNKNOWN)) {
            return HScriptType::TEXT;
        }
        if (right_type == HScriptType::TEXT && (left_type != HScriptType::VOID && left_type != HScriptType::UNKNOWN)) {
            return HScriptType::TEXT;
        }
    }
    
    else if (op_token_type == TokenType::QUESTION_EQUALS) {
        
        if (left_type == right_type && left_type != HScriptType::VOID && left_type != HScriptType::UNKNOWN) return HScriptType::LOGIC;
        if ((left_type == HScriptType::NUMBER || left_type == HScriptType::LNUMBER || left_type == HScriptType::RIEL) &&
            (right_type == HScriptType::NUMBER || right_type == HScriptType::LNUMBER || right_type == HScriptType::RIEL)) {
            return HScriptType::LOGIC; 
        }
    }

    return HScriptType::UNKNOWN; 
}