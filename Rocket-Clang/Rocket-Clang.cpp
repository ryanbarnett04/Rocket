#include "Rocket-Clang.h"

using namespace clang;
using namespace clang::tooling;

struct ProgramInfo {
    std::vector<FunctionDecl*> FunctionStack;
    int FunctionCount = 0;
    int AssertionCount = 0;
    int Violations = 0;
    int DereferenceCount = 0;
    bool MultipleDereference = false;
};

class TreeVisitor : public RecursiveASTVisitor<TreeVisitor> {
public:

    explicit TreeVisitor(ASTContext* Context, ProgramInfo& PI) : Context(Context), PI(PI) {}

    bool VisitGotoStmt(GotoStmt* gs) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(gs->getBeginLoc())) {
            llvm::outs() << "-> Rule 1 Violation: Goto statement used in " << SM.getFilename(gs->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(gs->getBeginLoc()) << "\n";
            ++PI.Violations;
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

            if (fd->hasBody()) {
                ++PI.FunctionCount;
                FunctionLengthChecker(fd, SM);
            }
        }

        return true;
    }

    bool VisitStaticAssertDecl(StaticAssertDecl *sad) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(sad->getBeginLoc())) {
            ++PI.AssertionCount;
        }

        return true;
    }

    bool VisitForStmt(ForStmt* fs) {
        
        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(fs->getBeginLoc())) {
            //BoundedLoopChecker(fs, SM);
        }

        return true;
    }

    bool VisitWhileStmt(WhileStmt* ws) {
        
        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(ws->getBeginLoc())) {
            //BoundedLoopChecker(ws, SM);
        }

        return true;
    }

    bool VisitUnaryOperator(UnaryOperator* uo) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(uo->getBeginLoc())) {
            
            if (uo->getOpcode() == UO_Deref) {
                MDC(uo, SM);
            }
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

        PI.FunctionStack.push_back(fd);
        RecursiveASTVisitor::TraverseFunctionDecl(fd);
        PI.FunctionStack.pop_back();

        return true;
    }

private:

    ASTContext* Context;
    ProgramInfo& PI;

    void FunctionCallDetector(CallExpr* expr, SourceManager& SM) {
        
        FunctionDecl* CalledFunction = expr->getDirectCallee();

        if (!CalledFunction) {
            return;
        }

        std::string CalledFunctionName = CalledFunction->getNameAsString();

        if (CalledFunctionName == "setjmp") {
            llvm::outs() << "-> Rule 1 Violation: 'setjmp' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
        if (CalledFunctionName == "longjmp") {
            llvm::outs() << "-> Rule 1 Violation: 'longjmp' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }

        if (!PI.FunctionStack.empty()) {

            FunctionDecl* CurrentFunction = PI.FunctionStack.back();

            if (CalledFunction == CurrentFunction) {
                llvm::outs()
                    << "-> Rule 1 Violation: Recursion found in function '"
                    << CurrentFunction->getNameAsString()
                    << "' in " << SM.getFilename(expr->getBeginLoc())
                    << " at line "
                    << SM.getSpellingLineNumber(expr->getBeginLoc())
                    << "\n";

                ++PI.Violations;
            }
        }

        if (CalledFunctionName == "malloc") {
            llvm::outs() << "-> Rule 3 Violation: 'malloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
        if (CalledFunctionName == "calloc") {
            llvm::outs() << "-> Rule 3 Violation: 'calloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
        if (CalledFunctionName == "realloc") {
            llvm::outs() << "-> Rule 3 Violation: 'realloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
    }


    void RecursionDetector(CallExpr* expr, SourceManager& SM) {

        if (PI.FunctionStack.empty()) {
            return;
        }

        FunctionDecl* CurrentFunction = PI.FunctionStack.back();
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

            ++PI.Violations;
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
            ++PI.Violations;
        }
        if (FunctionName == "calloc") {
            llvm::outs() << "-> Rule 3 Violation: 'calloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
        if (FunctionName == "realloc") {
            llvm::outs() << "-> Rule 3 Violation: 'realloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
            ++PI.Violations;
        }
    }

    
    void FunctionLengthChecker(FunctionDecl* fd, SourceManager& SM) {
        
        Stmt* body = fd->getBody();
        auto start = SM.getSpellingLineNumber(body->getBeginLoc());
        auto end = SM.getSpellingLineNumber(body->getEndLoc());
        
        if ((end - start + 1) > 60) {
            llvm::outs() << "-> Rule 4 Violation: Function '" << fd->getNameAsString() << "' in " << SM.getFilename(fd->getBeginLoc()) << " is longer than 60 lines of code \n";
            ++PI.Violations;
        }
    }


    template<typename T>
    void BoundedLoopChecker(T* stmt, SourceManager& SM) {

        Expr* condition = stmt->getCond()->IgnoreParenImpCasts();

        if (isa<ForStmt>(stmt)) {
            
            BinaryOperator* bo = dyn_cast<BinaryOperator>(condition);

            if (!bo || !bo->isComparisonOp()) {
                llvm::outs() << "-> Rule 2 Violation: For loop in "
                    << SM.getFilename(stmt->getBeginLoc())
                    << " at line "
                    << SM.getSpellingLineNumber(stmt->getBeginLoc())
                    << " is unbound";
                ++PI.Violations;
                return;
            }

            Expr* left = bo->getLHS()->IgnoreParenImpCasts();
            Expr* right = bo->getRHS()->IgnoreParenImpCasts();
        }


        if (isa<WhileStmt>(stmt)) {
            //
        }
    }

    void MultipleDereferenceChecker(UnaryOperator* uo, SourceManager& SM) {

        Expr* next = uo->getSubExpr()->IgnoreParenImpCasts();

        if (!isa<UnaryOperator>(next)) {
            return;
        }

        UnaryOperator* sub = static_cast<UnaryOperator*>(next);

        if (sub->getOpcode() == UO_Deref) {
            llvm::outs() << "-> Rule 9 Violation: Multiple dereference in "
                << SM.getFilename(uo->getBeginLoc())
                << " at line "
                << SM.getSpellingLineNumber(uo->getBeginLoc())
                << "\n";
            ++PI.Violations;
        }

        return;
    }

    void MDC(UnaryOperator* uo, SourceManager& SM) {

        Expr* next = uo->getSubExpr()->IgnoreParenImpCasts();

        if (!isa<UnaryOperator>(next)) {
            return;
        }

        UnaryOperator* sub = static_cast<UnaryOperator*>(next);

        if (sub->getOpcode() != UO_Deref) {
            return;
        }

        bool outmost = true;
        for (const auto& parent : Context->getParents(*uo)) {

            if (const auto* p = parent.get<UnaryOperator>()) {
                
                if (p->getOpcode() == UO_Deref) {
                    outmost = false;
                    break;
                }
            }
        }

        if (outmost == true) {
            llvm::outs() << "-> Rule 9 Violation: Multiple dereference in "
                << SM.getFilename(uo->getBeginLoc())
                << " at line "
                << SM.getSpellingLineNumber(uo->getBeginLoc())
                << "\n";
            ++PI.Violations;
        }

        return;
    }
};


/*
Everything below here is boilerplate
*/


class NewASTConsumer : public ASTConsumer {
public:
    explicit NewASTConsumer(ASTContext* Context, ProgramInfo& PI)
        : Visitor(Context, PI) {}

    void HandleTranslationUnit(ASTContext& Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    TreeVisitor Visitor;
};


class NewFrontendAction : public ASTFrontendAction {
public:
    explicit NewFrontendAction(ProgramInfo& PI)
        : PI(PI) {}
    std::unique_ptr<ASTConsumer>
        CreateASTConsumer(CompilerInstance& CI, StringRef) override {
        return std::make_unique<NewASTConsumer>(&CI.getASTContext(), PI);
    }
private:
    ProgramInfo& PI;
};


int main(int argc, const char** argv) {

    if (argc < 2) {
        llvm::errs() << "Usage: RocketClang.exe <file>\n";
        return 1;
    }

    std::string filepath = argv[1];

    llvm::outs() << "Trying to open: " << filepath << "\n";

    std::ifstream filestream(filepath);
    if (!filestream) {
        llvm::errs() << "Failed to open file: " << filepath << "\n";
        return 1;
    }

    std::string code((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
    std::vector<std::string> args = { "-std=c++17", "-w"};
    ProgramInfo PI;

    runToolOnCodeWithArgs(
        std::make_unique<NewFrontendAction>(PI),
        code,
        args,
        filepath
    );

    if (PI.AssertionCount / PI.FunctionCount <= 2) {
        llvm::outs() << "-> Rule 5 Violation: Assertion Density less than 2 assertions per function. Functions - " << PI.FunctionCount << ", Assertions - " << PI.AssertionCount << "\n";
        ++PI.Violations;
    }

    llvm::outs() << "Total violations detected: " << PI.Violations << "\n";
    llvm::outs() << "Functions: " << PI.FunctionCount << "\n";
    llvm::outs() << "Assertions: " << PI.AssertionCount << "\n";
    
    return 0;
}