#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib> 
#include <cstdio>  

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantic_analyzer.h"
#include "code_generator.h"

std::string get_compiler_command() {
    #if defined(_WIN32) || defined(_WIN64)
        if (system("g++ --version > nul 2>&1") == 0) return "g++"; /
        if (system("cl /? > nul 2>&1") == 0) return "cl"; 
        return "g++";
    #else 
        if (system("clang++ --version > /dev/null 2>&1") == 0) return "clang++";
        if (system("g++ --version > /dev/null 2>&1") == 0) return "g++";
        return "g++"; 
    #endif
}

int main(int argc, char* argv[]) {
    bool run_after_compile = false;
    std::string input_filename;
    std::string user_output_cpp_filename; 
    std::string user_output_exe_filename; 
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-run") {
            run_after_compile = true;
        } else if (arg == "-o_cpp" && i + 1 < argc) {
            user_output_cpp_filename = argv[++i];
        } else if (arg == "-o_exe" && i + 1 < argc) {
            user_output_exe_filename = argv[++i];
        } else if (input_filename.empty()) {
            input_filename = arg;
        } else {
            std::cerr << "Warning: Unrecognized or misplaced argument '" << arg << "'" << std::endl;
        }
    }

    if (input_filename.empty()) {
        std::cerr << "Usage: humanscript_compiler <input_file.humanscript> [-run] [-o_cpp output.cpp] [-o_exe output_exe]" << std::endl;
        return 1;
    }
    
    std::string base_filename = input_filename;
    size_t dot_pos = base_filename.rfind('.');
    if (dot_pos != std::string::npos) {
        base_filename = base_filename.substr(0, dot_pos);
    }

    std::string temp_cpp_filename = user_output_cpp_filename.empty() ? base_filename + "_hs_generated.cpp" : user_output_cpp_filename;
    std::string temp_exe_filename = user_output_exe_filename.empty() ? base_filename + "_hs_executable" : user_output_exe_filename;
    #if defined(_WIN32) || defined(_WIN64)
    if (user_output_exe_filename.empty() || user_output_exe_filename.rfind(".exe") == std::string::npos) {
        if (temp_exe_filename.rfind(".exe") == std::string::npos) {
            temp_exe_filename += ".exe";
        }
    }
    #endif

    std::ifstream input_file_stream(input_filename);
    if (!input_file_stream.is_open()) {
        std::cerr << "Error: Could not open input file '" << input_filename << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << input_file_stream.rdbuf();
    std::string source_code = buffer.str();
    input_file_stream.close();

    if (source_code.empty() && input_filename != "/dev/null") {
        std::cerr << "Warning: Input file '" << input_filename << "' is empty or could not be read." << std::endl;
    }

    std::cout << "Compiling HumanScript file: " << input_filename << std::endl;

    try {
        Lexer lexer(source_code);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::unique_ptr<ProgramNode> ast_root = parser.parse_program();

        SemanticAnalyzer semantic_analyzer;
        semantic_analyzer.analyze(ast_root.get());

        CodeGenerator code_generator;
        std::string cpp_code = code_generator.generate(ast_root.get());

        std::ofstream temp_cpp_file(temp_cpp_filename);
        if (!temp_cpp_file.is_open()) {
            std::cerr << "Error: Could not open temporary C++ output file '" << temp_cpp_filename << "'" << std::endl;
            return 1;
        }
        temp_cpp_file << cpp_code;
        temp_cpp_file.close();
        std::cout << "Generated C++ code written to: " << temp_cpp_filename << std::endl;

        if (run_after_compile) {
            std::cout << "\nCompiling generated C++ code..." << std::endl;
            std::string compiler = get_compiler_command();
            std::string compile_command;
            bool is_msvc = (compiler == "cl");

            if (is_msvc) {
                compile_command = compiler + " /EHsc /Fe\"" + temp_exe_filename + "\" \"" + temp_cpp_filename + "\" /std:c++17 /O2";
            } else {
                compile_command = compiler + " -std=c++17 -O2 \"" + temp_cpp_filename + "\" -o \"" + temp_exe_filename + "\"";
            }
            
            std::cout << "Executing: " << compile_command << std::endl;
            int compile_result = std::system(compile_command.c_str());

            if (compile_result != 0) {
                std::cerr << "Error: C++ compilation failed. Exit code: " << compile_result << std::endl;
                return 1; 
            }
            std::cout << "C++ compilation successful. Executable: " << temp_exe_filename << std::endl;

            std::cout << "\nRunning compiled HumanScript program..." << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            
            std::string run_command = "\"" + temp_exe_filename + "\"";
            #ifndef _WIN32
            if (run_command.rfind("./", 0) != 0 && run_command.rfind("/",0) !=0 ) { 
                 run_command = "./" + run_command;
            }
            #endif

            int run_result = std::system(run_command.c_str());
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "HumanScript program finished with exit code: " << run_result << std::endl;

            if (user_output_cpp_filename.empty()) { 
                std::remove(temp_cpp_filename.c_str()); 
            }
            if (user_output_exe_filename.empty()) { 
                std::remove(temp_exe_filename.c_str()); 
            }
        } else {
             std::cout << "\nTo run the compiled C++ code, use a C++ compiler, e.g.:" << std::endl;
             std::cout << "  g++ -std=c++17 -O2 " << temp_cpp_filename << " -o " << base_filename << "_executable" << std::endl;
             std::cout << "  ./" << base_filename << "_executable" << std::endl;
        }


    } catch (const std::exception& e) {
        std::cerr << "\nCompilation Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}