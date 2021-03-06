#ifndef INSTRU_STATS_MANAGER_H
#define INSTRU_STATS_MANAGER_H

#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"

/// \brief Responsible for computing statistical information for a file
class InstruStatsManager {
public:
  static void ComputeStats(std::string &FileName);
  static void ComputeStats(std::vector<std::string> &FileNames);
  static void Print();

  static int GetNumOfWords() { return NumOfWords; }
  static int GetNumOfStatements() { return NumOfStatements; }
  static int GetNumOfFunctions() { return NumOfFunctions; }

  static void IncreaseNumOfFunctions();
  static void IncreaseNumOfStatements();

  static bool isCountableStatement(clang::Stmt *S);

private:
  InstruStatsManager() {}
  InstruStatsManager(std::string &FileName);
  ~InstruStatsManager() {}

  static int NumOfWords;
  static int NumOfStatements;
  static int NumOfFunctions;
};

class StatsVisitor;

class StatsComputer : public clang::ASTConsumer {
  friend class StatsVisitor;

public:
  StatsComputer() : Visitor(NULL) {}
  ~StatsComputer() { delete Visitor; }

private:
  void Initialize(clang::ASTContext &Ctx);
  bool HandleTopLevelDecl(clang::DeclGroupRef D);

  InstruStatsManager *Manager;
  StatsVisitor *Visitor;
};

class StatsVisitor : public clang::RecursiveASTVisitor<StatsVisitor> {
public:
  StatsVisitor() {}

  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitStmt(clang::Stmt *S);
};

#endif // INSTRU_STATS_MANAGER_H
