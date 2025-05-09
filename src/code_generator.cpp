#include "code_generator.h"
#include <iostream> // For debugging output from generator itself

CodeGenerator::CodeGenerator() {}

std::string CodeGenerator::hscript_type_to_cpp_type(HScriptType type) {
    switch (type) {
        case HScriptType::NUMBER:  return "int";
        case HScriptType::LNUMBER: return "long long";
        case HScriptType::TEXT:    return "std::string";
        case HScriptType::LOGIC:   return "bool";
        case HScriptType::RIEL:    return "double";
        case HScriptType::VOID:    return "void";
        default:
            throw std::runtime_error("CodeGenerator Error: Cannot map unknown HScriptType to C++ type.");
    }
}

std::string CodeGenerator::generate(const ProgramNode* program) {
    output_stream.str(""); 
    output_stream.clear(); 
    iostream_included = false; // Reset for each generation
    bool string_needed_for_says = false; // For std::to_string

    output_stream << "// Generated by HumanScript Compiler\n\n";

    // 1. Process 'use' declarations from ProgramNode
    for (const auto& use_decl : program->use_declarations) {
        // Assuming all are system includes for now as we only parse use <...>
        output_stream << "#include <" << use_decl->header_name << ">\n";
        if (use_decl->header_name == "iostream") {
            iostream_included = true;
        }
        // Could check for "string" here too if it's commonly used for text type
        if (use_decl->header_name == "string") {
            // string_included_via_use = true; // A flag if needed
        }
    }
    // Add a newline if any includes were generated
    if (!program->use_declarations.empty()) {
        output_stream << "\n";
    }

    // Auto-include for 'says' if not already brought in by a 'use <iostream>;'
    // Also include <string> for std::to_string if 'says' is used with non-string types
    // And <iomanip> for std::boolalpha
    bool says_is_used = false;
    bool text_type_is_used = false;
    for (const auto& stmt : program->statements) {
        if (dynamic_cast<const SaysStatementNode*>(stmt.get())) {
            says_is_used = true;
        }
        if (auto var_decl = dynamic_cast<const VariableDeclarationNode*>(stmt.get())) {
            if (var_decl->var_type == HScriptType::TEXT || 
                (var_decl->expression && var_decl->expression->expr_type == HScriptType::TEXT) ) {
                text_type_is_used = true;
            }
        }
         if (auto says_node = dynamic_cast<const SaysStatementNode*>(stmt.get())) {
            if (says_node->expression && says_node->expression->expr_type == HScriptType::TEXT) {
                text_type_is_used = true;
            }
        }
    }

    if (text_type_is_used && program->use_declarations.end() == std::find_if(program->use_declarations.begin(), program->use_declarations.end(), [](const auto& u){ return u->header_name == "string"; })) {
         output_stream << "#include <string> // Auto-included for text type or string operations\n";
    }


    if (says_is_used) {
        if (!iostream_included) {
            output_stream << "#include <iostream> // Auto-included for 'says'\n";
            iostream_included = true; // Mark it as included
        }
        // Always include iomanip and string for says if iostream is involved, for boolalpha and to_string
        // Check if already included by a 'use' directive
        if (program->use_declarations.end() == std::find_if(program->use_declarations.begin(), program->use_declarations.end(), [](const auto& u){ return u->header_name == "iomanip"; })) {
            output_stream << "#include <iomanip>  // For std::boolalpha with 'says'\n";
        }
        if (program->use_declarations.end() == std::find_if(program->use_declarations.begin(), program->use_declarations.end(), [](const auto& u){ return u->header_name == "string"; })) {
             // Check if already included above for text_type_is_used
            bool string_already_auto_included = text_type_is_used && (program->use_declarations.end() == std::find_if(program->use_declarations.begin(), program->use_declarations.end(), [](const auto& u){ return u->header_name == "string"; }));
            if (!string_already_auto_included) {
                 output_stream << "#include <string>   // For std::to_string with 'says'\n";
            }
        }
        output_stream << "\n";
    }


    output_stream << "int main() {\n";
    if (iostream_included) { // Check if iostream was included either by 'use' or by 'says' auto-include
        output_stream << "    std::cout << std::boolalpha; // Print booleans as true/false\n";
    }

    for (const auto& stmt : program->statements) {
        output_stream << "    "; // Indentation
        visit(stmt.get()); // visit methods for VariableDeclarationNode, SaysStatementNode, etc.
    }

    output_stream << "    return 0;\n";
    output_stream << "}\n";

    return output_stream.str();
}

// --- Statement Visitors ---
void CodeGenerator::visit(const StatementNode* stmt) {
    if (auto var_decl_stmt = dynamic_cast<const VariableDeclarationNode*>(stmt)) {
        visit(var_decl_stmt);
    } else if (auto says_stmt = dynamic_cast<const SaysStatementNode*>(stmt)) {
        visit(says_stmt);
    } else if (auto if_stmt = dynamic_cast<const IfStatementNode*>(stmt)) {
        visit(if_stmt);
    } else if (auto block_stmt = dynamic_cast<const BlockStatementNode*>(stmt)) {
        visit(block_stmt);
    }
    // else if (auto assign_stmt = dynamic_cast<const AssignmentNode*>(stmt)) {
    //     visit(assign_stmt);
    // }
    else {
        throw std::runtime_error("CodeGenerator Error: Unknown statement node type for code generation.");
    }
}

void CodeGenerator::visit(const VariableDeclarationNode* stmt) {
    std::string cpp_type = hscript_type_to_cpp_type(stmt->var_type);
    output_stream << cpp_type << " " << stmt->identifier_name << " = ";
    // The expression's generated code should be compatible due to semantic analysis.
    // For numeric types, C++ handles implicit conversion (e.g., int to long long, int/ll to double).
    output_stream << generate_cpp_for_expression(stmt->expression.get(), stmt->var_type);
    output_stream << ";\n";
}

void CodeGenerator::visit(const SaysStatementNode* stmt) {
    if (!iostream_included) { // Should have been caught by pre-scan, but as a fallback
        // This is tricky, ideally pre-scan handles all includes.
        // Forcing it here might put include in the wrong place if main() isn't wrapped.
        // For simplicity, assume pre-scan is correct.
        // Or throw: throw std::runtime_error("CodeGenerator Error: <iostream> not included for 'says'.");
    }
    output_stream << "std::cout << (";
    HScriptType expr_h_type = stmt->expression->expr_type;
    std::string expr_code = generate_cpp_for_expression(stmt->expression.get());

    if (expr_h_type == HScriptType::TEXT) {
        output_stream << expr_code;
    } else if (expr_h_type == HScriptType::NUMBER || expr_h_type == HScriptType::LNUMBER || expr_h_type == HScriptType::RIEL || expr_h_type == HScriptType::LOGIC) {
        output_stream << expr_code;
    } else {
        // This path should ideally not be taken if semantic analysis restricts 'says' or if
        // binary op '+' with string already converted other types to string for concatenation.
        // However, as a fallback for direct printing of a non-string/non-numeric type:
        output_stream << "std::to_string(" << expr_code << ")";
    }
    output_stream << ") << std::endl;\n";
}

void CodeGenerator::visit(const IfStatementNode* stmt) {
    // Generate condition with parentheses for clarity
    output_stream << "if (";
    output_stream << generate_cpp_for_expression(stmt->condition.get(), HScriptType::LOGIC);
    output_stream << ") ";
    
    // For the then branch
    if (dynamic_cast<const BlockStatementNode*>(stmt->then_branch.get())) {
        // If it's already a block, just visit it (it will generate its own braces)
        visit(stmt->then_branch.get());
    } else {
        // If it's a single statement, wrap it in braces for consistency
        output_stream << "{\n        ";
        visit(stmt->then_branch.get());
        output_stream << "    }";
    }
    
    // For the else branch if it exists
    if (stmt->else_branch) {
        output_stream << " else ";
        if (dynamic_cast<const BlockStatementNode*>(stmt->else_branch.get())) {
            // If it's already a block, just visit it
            visit(stmt->else_branch.get());
        } else {
            // If it's a single statement, wrap it in braces for consistency
            output_stream << "{\n        ";
            visit(stmt->else_branch.get());
            output_stream << "    }";
        }
    }
    
    output_stream << "\n";
}

void CodeGenerator::visit(const BlockStatementNode* stmt) {
    output_stream << "{\n";
    
    // Visit each statement in the block with increased indentation
    for (const auto& s : stmt->statements) {
        output_stream << "        "; // Extra indentation for block statements
        visit(s.get());
    }
    
    output_stream << "    }";
}

// --- Expression Code Generation Helper ---
std::string CodeGenerator::generate_cpp_for_expression(const ExprNode* expr, HScriptType expected_context_type) {
    // expected_context_type can be used for explicit casts if needed, but C++ implicit conversions handle many cases.
    // This function dispatches to the specific generate_expr_code methods.
    if (auto int_lit = dynamic_cast<const IntegerLiteralNode*>(expr)) {
        return generate_expr_code(int_lit);
    } else if (auto dbl_lit = dynamic_cast<const DoubleLiteralNode*>(expr)) {
        return generate_expr_code(dbl_lit);
    } else if (auto str_lit = dynamic_cast<const StringLiteralNode*>(expr)) {
        return generate_expr_code(str_lit);
    } else if (auto bool_lit = dynamic_cast<const BooleanLiteralNode*>(expr)) {
        return generate_expr_code(bool_lit);
    } else if (auto ident = dynamic_cast<const IdentifierNode*>(expr)) {
        return generate_expr_code(ident);
    } else if (auto bin_op = dynamic_cast<const BinaryOpNode*>(expr)) {
        return generate_expr_code(bin_op);
    } else {
        throw std::runtime_error("CodeGenerator Error: Unknown expression node type for expression code generation.");
    }
}


// --- Specific Expression Node Code Generators ---
std::string CodeGenerator::generate_expr_code(const IntegerLiteralNode* expr) {
    return std::to_string(expr->value) + "LL"; // Suffix with LL for long long literals in C++
                                              // C++ will implicitly convert to int if assigned to int.
}

std::string CodeGenerator::generate_expr_code(const DoubleLiteralNode* expr) {
    std::string s = std::to_string(expr->value);
    // Ensure it has a decimal point to be treated as double if it's like "1.0" -> "1"
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
        s += ".0";
    }
    return s;
}

std::string CodeGenerator::generate_expr_code(const StringLiteralNode* expr) {
    // Need to escape characters for C++ string literal if they weren't already
    // For now, assume lexer handled basic escapes like \", \\, \n, \t correctly for storage,
    // and we just need to wrap in C++ quotes.
    std::string cpp_str = "\"";
    for (char c : expr->value) {
        switch (c) {
            case '"': cpp_str += "\\\""; break;
            case '\\': cpp_str += "\\\\"; break;
            case '\n': cpp_str += "\\n"; break;
            case '\r': cpp_str += "\\r"; break;
            case '\t': cpp_str += "\\t"; break;
            // Add other escapes if necessary
            default: cpp_str += c; break;
        }
    }
    cpp_str += "\"";
    return cpp_str;
}

std::string CodeGenerator::generate_expr_code(const BooleanLiteralNode* expr) {
    return expr->value ? "true" : "false";
}

std::string CodeGenerator::generate_expr_code(const IdentifierNode* expr) {
    return expr->name; // Assumes variable name is valid C++ identifier
}

std::string CodeGenerator::generate_expr_code(const BinaryOpNode* expr) {
    std::string left_cpp = generate_cpp_for_expression(expr->left.get());
    std::string right_cpp = generate_cpp_for_expression(expr->right.get());
    std::string op_cpp;

    HScriptType expr_result_type = expr->expr_type; // Overall type of the binary operation
    HScriptType left_h_type = expr->left->expr_type;
    HScriptType right_h_type = expr->right->expr_type;

    switch (expr->op_token.type) {
        case TokenType::PLUS:
            if (expr_result_type == HScriptType::TEXT) {
                if (left_h_type != HScriptType::TEXT) {
                    left_cpp = "std::to_string(" + left_cpp + ")";
                }
                if (right_h_type != HScriptType::TEXT) {
                    right_cpp = "std::to_string(" + right_cpp + ")";
                }
            }
            op_cpp = "+";
            break;
        case TokenType::QUESTION_EQUALS:
            op_cpp = "==";
            break;
        default:
            throw std::runtime_error("CodeGenerator Error: Unsupported binary operator token for C++ code generation: " + expr->op_token.text);
    }
    return "(" + left_cpp + " " + op_cpp + " " + right_cpp + ")";
}