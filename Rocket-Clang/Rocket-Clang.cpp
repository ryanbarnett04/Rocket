#include "Rocket-Clang.h"

using namespace clang;
using namespace clang::tooling;
int violations = 0;


class TreeVisitor : public RecursiveASTVisitor<TreeVisitor> {
public:

    explicit TreeVisitor(ASTContext* Context) : Context(Context) {}

    bool VisitStmt(Stmt* s) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(s->getBeginLoc())) {
            GotoChecker(s, SM);
        }

        return true;
    }

    bool VisitCallExpr(CallExpr* expr) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(expr->getBeginLoc())) {
            FunctionCallDetector(expr, SM);
        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl* fd) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(fd->getBeginLoc())) {
            FunctionLengthChecker(fd, SM);
        }

        return true;
    }

    bool VisitStaticAssertDecl(StaticAssertDecl *D) {

        SourceManager &SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(D->getBeginLoc())) {
            llvm::outs() << "static_assert in "
                << SM.getFilename(D->getBeginLoc()) << "\n";
        }

        return true;
    }

    bool TraverseFunctionDecl(FunctionDecl* fd) {

        SourceManager& SM = Context->getSourceManager();

        if (!fd || !fd->hasBody()) {
            return true;
        }

        if (SM.isInSystemHeader(fd->getBeginLoc())) {
            return true;
        }

        FunctionStack.push_back(fd);
        RecursiveASTVisitor::TraverseFunctionDecl(fd);
        FunctionStack.pop_back();

        return true;
    }

private:

    ASTContext* Context;
    std::vector<FunctionDecl*> FunctionStack;

    void GotoChecker(Stmt* s, SourceManager& SM) {
        
        if (!s) {
            return;
        }

        if (isa<GotoStmt>(s)) {
            llvm::outs() << "-> Rule 1 Violation: Goto statement used in " << SM.getFilename(s->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(s->getBeginLoc()) << "\n";
            ++violations;
        }
    }


    void FunctionCallDetector(CallExpr* expr, SourceManager& SM) {
        
        FunctionDecl* CalledFunction = expr->getDirectCallee();

        if (!CalledFunction) {
            return;
        }

        std::string CalledFunctionName = CalledFunction->getNameAsString();

        if (CalledFunctionName == "setjmp") {
            llvm::outs() << "-> Rule 1 Violation: 'setjmp' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
        if (CalledFunctionName == "longjmp") {
            llvm::outs() << "-> Rule 1 Violation: 'longjmp' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }

        if (!FunctionStack.empty()) {

            FunctionDecl* CurrentFunction = FunctionStack.back();

            if (CalledFunction == CurrentFunction) {
                llvm::outs()
                    << "-> Rule 1 Violation: Recursion found in function '"
                    << CurrentFunction->getNameAsString()
                    << "' in " << SM.getFilename(expr->getBeginLoc())
                    << " at line "
                    << SM.getSpellingLineNumber(expr->getBeginLoc())
                    << "\n";

                ++violations;
            }
        }

        if (CalledFunctionName == "malloc") {
            llvm::outs() << "-> Rule 3 Violation: 'malloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
        if (CalledFunctionName == "calloc") {
            llvm::outs() << "-> Rule 3 Violation: 'calloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
        if (CalledFunctionName == "realloc") {
            llvm::outs() << "-> Rule 3 Violation: 'realloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }

        if (CalledFunctionName == "assert") {
            llvm::outs() << "-> Assert Spotted!" << "\n";
        }
        if (CalledFunctionName == "static_assert") {
            llvm::outs() << "-> Static Assert Spotted!" << "\n";
        }
    }


    void RecursionDetector(CallExpr* expr, SourceManager& SM) {

        if (FunctionStack.empty()) {
            return;
        }

        FunctionDecl* CurrentFunction = FunctionStack.back();
        FunctionDecl* CalledFunction = expr->getDirectCallee();

        if (!CalledFunction) {
            return;
        }

        if (CalledFunction == CurrentFunction) {
            llvm::outs()
                << "-> Rule 1 Violation: Recursion found in function '"
                << CurrentFunction->getNameAsString()
                << "' in " << SM.getFilename(expr->getBeginLoc())
                << " at line "
                << SM.getSpellingLineNumber(expr->getBeginLoc())
                << "\n";

            ++violations;
        }
    }


    void DynamicMemoryAfterInit(CallExpr* expr, SourceManager& SM) {
        
        FunctionDecl* CalledFunction = expr->getDirectCallee();

        if (!CalledFunction) {
            return;
        }

        std::string FunctionName = CalledFunction->getNameAsString();

        if (FunctionName == "malloc") {
            llvm::outs() << "-> Rule 3 Violation: 'malloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
        if (FunctionName == "calloc") {
            llvm::outs() << "-> Rule 3 Violation: 'calloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
        if (FunctionName == "realloc") {
            llvm::outs() << "-> Rule 3 Violation: 'realloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++violations;
        }
    }

    
    void FunctionLengthChecker(FunctionDecl* fd, SourceManager& SM) {
        
        if (!fd->hasBody()) {
            return;
        }

        Stmt* body = fd->getBody();
        unsigned int start = SM.getSpellingLineNumber(body->getBeginLoc());
        unsigned int end = SM.getSpellingLineNumber(body->getEndLoc());
        
        if ((end - start + 1) > 60) {
            llvm::outs() << "-> Rule 4 Violation: Function '" << fd->getNameAsString() << "' in " << SM.getFilename(fd->getBeginLoc()) << " is longer than 60 lines of code \n";
            ++violations;
        }
    }
};


/*
Everything below here is boilerplate
*/


class NewASTConsumer : public ASTConsumer {
public:
    explicit NewASTConsumer(ASTContext* Context)
        : Visitor(Context) {}

    void HandleTranslationUnit(ASTContext& Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    TreeVisitor Visitor;
};


class NewFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer>
        CreateASTConsumer(CompilerInstance& CI, StringRef) override {
        return std::make_unique<NewASTConsumer>(&CI.getASTContext());
    }
};


int main(int argc, const char** argv) {

    if (argc < 2) {
        llvm::errs() << "Usage: RocketClang.exe <file>\n";
        return 1;
    }

    std::string filepath = argv[1];

    llvm::outs() << "Trying to open: " << filepath << "\n";

    std::ifstream t(filepath);
    if (!t) {
        llvm::errs() << "Failed to open file: " << filepath << "\n";
        return 1;
    }

    std::string code((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());

    std::vector<std::string> args = { "-std=c++17", "-w"};

    runToolOnCodeWithArgs(
        std::make_unique<NewFrontendAction>(),
        code,
        args,
        filepath
    );

    llvm::outs() << "Total violations detected: " << violations << "\n";

    return 0;
}