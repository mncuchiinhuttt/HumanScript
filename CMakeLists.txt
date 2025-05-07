cmake_minimum_required(VERSION 3.10)
project(HumanScriptCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(humanscript_compiler
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/semantic_analyzer.cpp
    src/code_generator.cpp
)

target_include_directories(humanscript_compiler PUBLIC src)