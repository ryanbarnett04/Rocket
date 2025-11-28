#undef UNICODE
#include "main.h"

int main() {

    std::string filepath(initial_file());
    std::cout << "File - " << filepath << std::endl << '\n';
    std::fstream input(filepath, std::ios::in);

    if (!input.is_open()) {
        std::cerr << "Error: Failed to open file for reading!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::stringstream content_stream;
    content_stream << input.rdbuf();
    std::string content = content_stream.str();
    std::vector<Line> lines = create_lines(content);

    for (U16 i = 0; i < lines.size(); i++) {
        std::cout << "Line " << (i + 1) << ": " << "'" << lines.at(i).value << "'" << " : " << print_line_class(lines.at(i).type) << std::endl;
    }

    std::cout << std::endl;
    create_tokens(lines);

    for (U16 i = 0; i < lines.size(); i++) {
        std::cout << "Line " << (i + 1) << " Tokens:" << std::endl;
        std::vector<Token> &toks = lines.at(i).tokens;

        for (int j = 0; j < toks.size(); j++) {
            std::cout << "Token " << (j + 1) << ": " << "[" << toks.at(j).value << "]" << " : " << print_token_class(toks.at(j).type) << std::endl;
        }

        std::cout << std::endl;
    }
}

std::string initial_file(void) {

    OPENFILENAME ofn;
    char szFile[MAX_PATH] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "C++ Source Files\0*.cpp*\0C++ Header Files\0*.hpp*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) != TRUE) {
        std::cerr << "Error: No file selected or dialog closed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return std::string(szFile);
}

std::vector<Line> create_lines(const std::string &content) {

    std::vector<Line> lines;
    std::string buffer;
    bool multi_comment = false;

    for (U32 i = 0; i < content.length(); i++) {

        if (isspace(content.at(i)) && buffer.empty()) {
            continue;
        }

        U8 c = content.at(i);

        if (c != '{' && c != '}' && c != ';' && c != '\n') {
            buffer.push_back(content.at(i));
            continue;
        }

        if (content.at(i) != '\n') {
            buffer.push_back(content.at(i));
        }

        if (buffer.empty()) {
            lines.push_back({.value = ""});
        }
        else {
            LineClass lc;

            if ( buffer.size() > 1 && buffer.at(0) == '/' && buffer.at(1) == '*') {
                multi_comment = true;
            }

            if (multi_comment == true || (buffer.size() > 1) && (buffer.at(0) == '/' && buffer.at(1) == '/')) {
                lc = LineClass::_comment;
            }
            else if (buffer.at(0) == '#') {
                lc = LineClass::_pre_macro;
            }
            else {
                lc = LineClass::_code;
            }

            lines.push_back({.value = buffer, .type = lc});
        }

        if (buffer.at(buffer.length() - 1) == '/' && buffer.at(buffer.length() - 2) == '*') {
            multi_comment = false;
        }

        buffer.clear();
    }

    return lines;
}

void create_tokens(std::vector<Line> &lines) {

    std::string buffer;
    std::vector<Token> tokens;
    bool string_lit = false;
    TokenClass recent_token_type;

    for (Line &line : lines) {
        const std::string &ls = line.value;
        for (char c : ls) {
            if (isspace(c)) {
                if (string_lit == true) {
                    buffer.push_back(c);
                    continue;
                }
                if (buffer.length() != 0 && !string_lit) {
                    tokens.push_back(assign_token(buffer, tokens));
                    buffer.clear();
                }
                else {
                    continue;
                }
            }
            else {
                if (c == '"') {
                    if (string_lit == false) {
                        string_lit = true;
                    }
                    else {
                        string_lit = false;
                    }
                }
                if (c == ';') {
                    if (buffer.length() != 0) {
                        tokens.push_back(assign_token(buffer, tokens));
                        buffer.clear();
                    }
                    tokens.push_back(assign_token(";", tokens));
                    continue;
                }
                buffer.push_back(c);
            }
        }

        if (buffer.length() != 0) {
            tokens.push_back(assign_token(buffer, tokens));
            buffer.clear();
        }

        line.tokens = tokens;
        tokens.clear();
    }
}

void create_tokens2(std::vector<Line>& lines) {

    std::string buffer;
    std::vector<Token> tokens;
    bool string_lit = false;
    TokenClass recent_token_type;

    for (Line& line : lines) {
        const std::string& ls = line.value;

        for (char c : ls) {

            if (valid_symbols.contains(c)) {

                if (buffer.length() != 0) {
                    tokens.push_back(assign_token(buffer, tokens));
                    buffer.clear();
                }

                buffer.push_back(c);
                tokens.push_back(assign_token(buffer, tokens));
                buffer.clear();
            }

            else if (c == '"') {
                if (!string_lit) {
                    string_lit = true;
                }
                else {
                    string_lit = false;
                }
            }
        }
    }
}

Token assign_token(const std::string &buffer, const std::vector<Token> &tokens) {

    if (valid_tokens.contains(buffer)) {
        
        return {.type = valid_tokens.at(buffer), .value = buffer};

    }
    else {
        if (buffer.at(0) == '<' && buffer.at(buffer.length() - 1) == '>') {
            return {.type = TokenClass::_include_file, .value = buffer};
        }

        if (buffer.at(0) == '"' && buffer.at(buffer.length() -1) == '"') {
            if (tokens.at(0).type == TokenClass::_include) {
                return {.type = TokenClass::_include_file, .value = buffer};
            }
            else {
                return {.type = TokenClass::_string_lit, .value = buffer};
            }
            return {.type = TokenClass::_string_lit, .value = buffer};
        }

        if (isalpha(buffer.at(0))) {
            return {.type = TokenClass::_variable_name, .value = buffer};
        }

        if (isdigit(buffer.at(0))) {
            if (buffer.find('.') != std::string::npos) {
                if (tokens.at(0).type == TokenClass::_float) {
                    return {.type = TokenClass::_float_lit, .value = buffer};
                }
                else if (tokens.at(0).type == TokenClass::_double) {
                    return {.type = TokenClass::_double_lit, .value = buffer};
                }
            }
            else {
                return {.type = TokenClass::_int_lit, .value = buffer};
            }
        }

        return {.type = TokenClass::_unrecognised, .value = buffer};
    }
}