#include <memory>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <iomanip>

#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/MacroInfo.h>

#include <llvm/Support/raw_ostream.h>

class TreeVisitor;
class NewASTConsumer;
class NewFrontendAction;