#include <memory>
#include <iostream>
#include <fstream>
#include <iterator>

#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Tooling/CompilationDatabase.h>

#include <llvm/Support/raw_ostream.h>

class TreeVisitor;
class NewASTConsumer;
class NewFrontendAction;