#include "InstruStatsManager.h"

#include <fstream>

#include "clang/AST/Expr.h"

#include "InstruFrontend.h"

int InstruStatsManager::NumOfWords = 0;
int InstruStatsManager::NumOfStatements = 0;
int InstruStatsManager::NumOfFunctions = 0;

void InstruStatsManager::ComputeStats(std::vector<std::string> &FileNames) {
  NumOfWords = 0;
  NumOfStatements = 0;
  NumOfFunctions = 0;

  for (auto &FileName : FileNames) {
    StatsComputer *S = new StatsComputer();
    InstruFrontend::Parse(FileName, S);

    std::ifstream IFS(FileName.c_str());
    std::string Word;
    while (IFS >> Word) {
      NumOfWords++;
    }
  }
}

void InstruStatsManager::ComputeStats(std::string &FileName) {
  NumOfWords = 0;
  NumOfStatements = 0;
  NumOfFunctions = 0;
  StatsComputer *S = new StatsComputer();
  InstruFrontend::Parse(FileName, S);

  std::ifstream IFS(FileName.c_str());
  std::string Word;
  while (IFS >> Word) {
    NumOfWords++;
  }
}

void InstruStatsManager::Print() {
  llvm::outs() << "# Functions  : " << NumOfFunctions << "\n";
  llvm::outs() << "# Statements : " << NumOfStatements << "\n";
}

void InstruStatsManager::IncreaseNumOfFunctions() { NumOfFunctions++; }

void InstruStatsManager::IncreaseNumOfStatements() { NumOfStatements++; }

bool InstruStatsManager::isCountableStatement(clang::Stmt *S) {
  if (clang::Expr *E = llvm::dyn_cast<clang::Expr>(S)) {
    if (clang::BinaryOperator *BO = llvm::dyn_cast<clang::BinaryOperator>(E)) {
      return BO->isAssignmentOp();
    } else if (clang::UnaryOperator *UO =
                   llvm::dyn_cast<clang::UnaryOperator>(E)) {
      return UO->isIncrementDecrementOp();
    } else if (clang::CallExpr *CE = llvm::dyn_cast<clang::CallExpr>(E)) {
      return true;
    } else {
      return false;
    }
  } else if (clang::CompoundStmt *C = llvm::dyn_cast<clang::CompoundStmt>(S)) {
    return false;
  } else if (clang::NullStmt *N = llvm::dyn_cast<clang::NullStmt>(S)) {
    return false;
  } else if (clang::LabelStmt *L = llvm::dyn_cast<clang::LabelStmt>(S)) {
    return false;
  } else if (clang::DeclStmt *D = llvm::dyn_cast<clang::DeclStmt>(S)) {
    if (D->isSingleDecl()) {
      if (clang::VarDecl *VD =
              llvm::dyn_cast<clang::VarDecl>(D->getSingleDecl())) {
        return VD->hasInit();
      }
    }
    return false;
  } else if (clang::SwitchCase *SC = llvm::dyn_cast<clang::SwitchCase>(S)) {
    return false;
  } else {
    return true;
  }
}

void StatsComputer::Initialize(clang::ASTContext &Ctx) {
  Visitor = new StatsVisitor();
}

bool StatsComputer::HandleTopLevelDecl(clang::DeclGroupRef D) {
  for (clang::DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    Visitor->TraverseDecl(*I);
  }
  return true;
}

bool StatsVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  if (FD->isThisDeclarationADefinition())
    InstruStatsManager::IncreaseNumOfFunctions();
  return true;
}

bool StatsVisitor::VisitStmt(clang::Stmt *S) {
  if (InstruStatsManager::isCountableStatement(S))
    InstruStatsManager::IncreaseNumOfStatements();
  return true;
}
