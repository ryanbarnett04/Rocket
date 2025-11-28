#pragma once

#include <windows.h>
#include <commdlg.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <unordered_map>

typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef int64_t I64;
typedef uint64_t U64;

enum class LineClass : U8 {
    _comment,
    _pre_macro,
    _code,
    _empty_line
};

enum class TokenClass : U8 {
    _if,
    _for,
    _while,
    _goto,
    _setjmp,
    _longjmp,
    _include,
    _include_file,
    _pragma_once,
    _function_name,
    _variable_name,
    _int,
    _float,
    _double,
    _char,
    _string,
    _int_lit,
    _float_lit,
    _double_lit,
    _string_lit,
    _open_curly,
    _close_curly,
    _semi,
    _bitshift_left,
    _bitshift_right,
    _equals,
    _plus,
    _subtract,
    _multiply,
    _divide,
    _using,
    _namespace,
    _comment_start,
    _unrecognised,
    _uninit
};

std::unordered_map<std::string, TokenClass> valid_tokens = {
    {"if", TokenClass::_if},
    {"for", TokenClass::_for},
    {"while", TokenClass::_while},
    {"goto", TokenClass::_goto},
    {"setjmp", TokenClass::_setjmp},
    {"longjmp", TokenClass::_longjmp},
    {"#include", TokenClass::_include},
    {"#pragma once", TokenClass::_pragma_once},
    {"int", TokenClass::_int},
    {"float", TokenClass::_float},
    {"double", TokenClass::_double},
    {"char", TokenClass::_char},
    {"string", TokenClass::_string},
    {"std::string", TokenClass::_string},
    {"char*", TokenClass::_string},
    {"{", TokenClass::_open_curly},
    {"}", TokenClass::_close_curly},
    {";", TokenClass::_semi},
    {"=", TokenClass::_equals},
    {"+", TokenClass::_plus},
    {"-", TokenClass::_subtract},
    {"*", TokenClass::_multiply},
    {"/", TokenClass::_divide},
    {"<<", TokenClass::_bitshift_left},
    {">>", TokenClass::_bitshift_right},
    {"using", TokenClass::_using},
    {"namespace", TokenClass::_namespace},
};

std::unordered_map < char, TokenClass> valid_symbols = {
    {'{', TokenClass::_open_curly},
    {'}', TokenClass::_close_curly},
    {';', TokenClass::_semi},
    {'=', TokenClass::_equals},
    {'+', TokenClass::_plus},
    {'-', TokenClass::_subtract},
    {'*', TokenClass::_multiply},
    {'/', TokenClass::_divide},
};

struct Token {
    TokenClass type;
    std::string value;
};

struct Line {
    std::string value;
    LineClass type;
    std::vector<Token> tokens;
};

std::string initial_file(void);
std::vector<Line> create_lines(const std::string &content);
void create_tokens(std::vector<Line> &lines);
void create_tokens2(std::vector<Line>& lines);
Token assign_token(const std::string &buffer, const std::vector<Token> &tokens);

std::string print_line_class(LineClass& lc) {

    switch (lc) {
    case LineClass::_code:
        return "_code";
        break;
    case LineClass::_pre_macro:
        return "_pre_macro";
        break;
    case LineClass::_comment:
        return "_comment";
        break;
    }
}

std::string print_token_class(TokenClass &tc) {

    switch (tc) {
    case TokenClass::_if:
        return "_if";
        break;
    case TokenClass::_for:
        return "_for";
        break;
    case TokenClass::_while :
        return "_while";
        break;
    case TokenClass::_goto:
        return "_goto";
        break;
    case TokenClass::_setjmp:
        return "_setjmp";
        break;
    case TokenClass::_longjmp:
        return "_longjmp";
        break;
    case TokenClass::_include:
        return "_include";
        break;
    case TokenClass::_include_file:
        return "_include_file";
        break;
    case TokenClass::_pragma_once:
        return "_pragma_once";
        break;
    case TokenClass::_int:
        return "_int";
        break;
    case TokenClass::_float:
        return "_float";
        break;
    case TokenClass::_double :
        return "_double";
        break;
    case TokenClass::_char:
        return "_char";
        break;
    case TokenClass::_string:
        return "_string";
        break;
    case TokenClass::_open_curly :
        return "_open_curly";
        break;
    case TokenClass::_close_curly :
        return "_close_curly";
        break;
    case TokenClass::_semi:
        return "_semi";
        break;
    case TokenClass::_equals:
        return "_equals";
        break;
    case TokenClass::_int_lit:
        return "_int_lit";
        break;
    case TokenClass::_float_lit:
        return "_float_lit";
        break;
    case TokenClass::_double_lit:
        return "_double_lit";
        break;
    case TokenClass::_string_lit:
        return "_string_lit";
        break;
    case TokenClass::_function_name:
        return "_function_name";
        break;
    case TokenClass::_variable_name:
        return "_variable_name";
        break;
    case TokenClass::_unrecognised:
        return "_unrecognised";
        break;
    case TokenClass::_plus:
        return "_plus";
        break;
    case TokenClass::_subtract:
        return "_subtract";
        break;
    case TokenClass::_multiply:
        return "_multiply";
        break;
    case TokenClass::_divide:
        return "_divide";
        break;
    case TokenClass::_bitshift_left:
        return "_bitshift_left";
        break;
    case TokenClass::_bitshift_right:
        return "_bitshift_right";
        break;
    case TokenClass::_using:
        return "_using";
        break;
    case TokenClass::_namespace:
        return "_namespace";
        break;
    }
}
