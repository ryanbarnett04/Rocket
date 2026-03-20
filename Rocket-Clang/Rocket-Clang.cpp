#include "Rocket-Clang.h"

using namespace clang;
using namespace clang::tooling;

struct VariableInfo {
    std::string VariableName;
    const unsigned int SourceLine;
    const unsigned int DeclarationDepth;
    int ShallowestReferenceDepth;
};

struct ProgramInfo {
    std::vector<FunctionDecl*> FunctionStack;
    int FunctionCount = 0;
    int AssertionCount = 0;
    int Violations = 0;
    int DereferenceCount = 0;
    bool MultipleDereference = false;
    std::vector<std::tuple<llvm::StringRef, unsigned int, unsigned int>> OutmostDerefs;
    std::unordered_map<VarDecl*, VariableInfo> VariableDepthMap{};
    unsigned int CurrentDepth = 0;
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
            FunctionReturnCheck(expr, SM);
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
                MultipleDereferenceChecker(uo, SM);
            }
        }

        return true;
    }

    bool VisitVarDecl(VarDecl* vd) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(vd->getBeginLoc())) {
            FunctionPointerChecker(vd, SM);
            VarDepthHandler(vd, SM);
        }

        return true;
    }

    bool VisitParmVarDecl(ParmVarDecl* pvd) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(pvd->getBeginLoc())) {
            FunctionPointerChecker(pvd, SM);
        }

        return true;
    }

    bool VisitFieldDecl(FieldDecl* fd) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(fd->getBeginLoc())) {
            FunctionPointerChecker(fd, SM);
        }

        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr* dre) {

        SourceManager& SM = Context->getSourceManager();

        if (!SM.isInSystemHeader(dre->getBeginLoc())) {
            DeclRefExprHandler(dre, SM);
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

        PI.FunctionStack.push_back(fd->getCanonicalDecl());
        RecursiveASTVisitor::TraverseFunctionDecl(fd);
        PI.FunctionStack.pop_back();

        return true;
    }

    bool TraverseCompoundStmt(CompoundStmt* cs) {

        SourceManager& SM = Context->getSourceManager();

        ++PI.CurrentDepth;
        RecursiveASTVisitor::TraverseCompoundStmt(cs);
        --PI.CurrentDepth;

        return true;
    }

private:

    ASTContext* Context;
    ProgramInfo& PI;

    void FunctionCallDetector(CallExpr* expr, SourceManager& SM) {
        
        FunctionDecl* CalledFunction = expr->getDirectCallee()->getCanonicalDecl();

        if (!CalledFunction) {
            return;
        }

        std::string CalledFunctionName = CalledFunction->getNameAsString();

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
                    << "' calls itself in " << SM.getFilename(expr->getBeginLoc())
                    << " at line "
                    << SM.getSpellingLineNumber(expr->getBeginLoc())
                    << "\n";

                ++PI.Violations;
            }

            for (const auto* f : PI.FunctionStack) {
                
                if (CalledFunction == f && CalledFunction != CurrentFunction) {
                    llvm::outs()
                        << "-> Rule 1 Violation: Recursion (Indirect) found in function '"
                        << CurrentFunction->getNameAsString()
                        << "' calls function '" << f->getNameAsString()
                        << "' in "
                        << SM.getFilename(expr->getBeginLoc())
                        << " at line "
                        << SM.getSpellingLineNumber(expr->getBeginLoc())
                        << "\n";

                    ++PI.Violations;
                    break;
                }
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
        if (CalledFunctionName == "aligned_alloc") {
            llvm::outs() << "-> Rule 3 Violation: 'aligned_alloc' use in " << SM.getFilename(expr->getBeginLoc()) << " at line " << SM.getSpellingLineNumber(expr->getBeginLoc()) << "\n";
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


    void FunctionReturnCheck(CallExpr* ce, SourceManager& SM) {

        const Expr* WarnExpr = nullptr;
        SourceLocation Loc;
        SourceRange R1;
        SourceRange R2;

        if (ce->isUnusedResultAWarning(WarnExpr, Loc, R1, R2, *Context))
        {
            llvm::outs() << "Rule 7 Violation: Return value ignored in "
                << SM.getFilename(ce->getBeginLoc())
                << " at line "
                << SM.getSpellingLineNumber(ce->getBeginLoc())
                << "\n";
            ++PI.Violations;
        }

        return;
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


    template<typename T>
    void FunctionPointerChecker(T* type, SourceManager& SM) {

        auto IsFunctionPointer = [](QualType qt) {
            qt = qt.getCanonicalType();

            if (const auto* pt = qt->getAs<PointerType>()) {

                if (pt->getPointeeType()->isFunctionType()) {
                    return true;
                }
            }

            if (qt->isMemberFunctionPointerType()) {
                return true;
            }

            return false;
            };

        QualType qt;

        if constexpr (std::is_same<T, ParmVarDecl>::value) {
            qt = type->getOriginalType();
        }
        else {
            qt = type->getType();
        }

        if (IsFunctionPointer(qt)) {
            llvm::outs() << "-> Rule 9 Violation: Function pointer in "
                << SM.getFilename(type->getBeginLoc())
                << " at line "
                << SM.getSpellingLineNumber(type->getBeginLoc())
                << "\n";
            ++PI.Violations;
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


    void VarDepthHandler(VarDecl* vd, SourceManager& SM) {

        if (isa<ParmVarDecl>(vd) || vd->hasGlobalStorage()) {
            return;
        }

        VariableInfo vi = {vd->getNameAsString(), SM.getSpellingLineNumber(vd->getBeginLoc()), PI.CurrentDepth, -1};

        auto result = PI.VariableDepthMap.insert({vd, vi});

        if (!result.second) {
            llvm::outs() << "Key already existed\n";
        }

        return;
    }


    void DeclRefExprHandler(DeclRefExpr* dre, SourceManager& SM) {

        if (VarDecl* vd = dyn_cast<VarDecl>(dre->getDecl())) {
            
            auto iterator = PI.VariableDepthMap.find(vd);

            if (iterator != PI.VariableDepthMap.end()) {

                auto& vi = iterator->second;

                if (vi.ShallowestReferenceDepth == -1) { vi.ShallowestReferenceDepth = PI.CurrentDepth; }
                else {
                    if (PI.CurrentDepth < vi.ShallowestReferenceDepth) { vi.ShallowestReferenceDepth = PI.CurrentDepth; }
                }
            }
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
    std::vector<std::string> args = { "-std=c++17", "-w" };
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

    for (const auto& variable : PI.VariableDepthMap) {

        const auto& vi = variable.second;

        if (vi.ShallowestReferenceDepth != -1 && vi.ShallowestReferenceDepth > vi.DeclarationDepth) {
            llvm::outs() << "-> Rule 6 Violation:"
                << " Variable " << vi.VariableName << " is defined at depth " << vi.DeclarationDepth
                << ", but it's shallowest reference is " << vi.ShallowestReferenceDepth
                << ", variable is not declared at lowest possible scope\n";
            ++PI.Violations;
        }
    }

    llvm::outs() << "Total violations detected: " << PI.Violations << "\n";

    return 0;
}