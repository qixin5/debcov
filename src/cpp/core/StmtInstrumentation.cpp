#include "StmtInstrumentation.h"

#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <unordered_map>

#include "clang/Lex/Lexer.h"

#include "InstruFileManager.h"
#include "InstruOptionManager.h"
#include "InstruProfiler.h"
#include "SourceManager.h"

using BinaryOperator = clang::BinaryOperator;
using BreakStmt = clang::BreakStmt;
using CallExpr = clang::CallExpr;
using ContinueStmt = clang::ContinueStmt;
using CompoundStmt = clang::CompoundStmt;
using DeclGroupRef = clang::DeclGroupRef;
using DeclStmt = clang::DeclStmt;
using FunctionDecl = clang::FunctionDecl;
using GotoStmt = clang::GotoStmt;
using IfStmt = clang::IfStmt;
using LabelStmt = clang::LabelStmt;
using ReturnStmt = clang::ReturnStmt;
using SourceRange = clang::SourceRange;
using SourceLocation = clang::SourceLocation;
using Stmt = clang::Stmt;
using UnaryOperator = clang::UnaryOperator;
using WhileStmt = clang::WhileStmt;
using VarDecl = clang::VarDecl;
using Decl = clang::Decl;
using LabelDecl = clang::LabelDecl;
using Expr = clang::Expr;
using DeclRefExpr = clang::DeclRefExpr;
using ForStmt = clang::ForStmt;
using SwitchStmt = clang::SwitchStmt;
using DoStmt = clang::DoStmt;
using NullStmt = clang::NullStmt;
using SwitchCase = clang::SwitchCase;

void StmtInstrumentation::Initialize(clang::ASTContext &Ctx) {
  InstruTransformation::Initialize(Ctx);
  CollectionVisitor = new StmtCollectionVisitor(this, this->getTheRewriter());
}

void StmtInstrumentation::Finalize() {
  CollectionVisitor->WriteChangesToFiles();
}

bool StmtInstrumentation::HandleTopLevelDecl(DeclGroupRef DR) {
  //spdlog::get("Logger")->info("Reached HandleTopLevelDecl");
  clang::SourceManager &SM = Context->getSourceManager();
  for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
    if (SourceManager::IsInHeader(SM, *b)) continue;
    CollectionVisitor->TraverseDecl(*b);
  }
  CollectionVisitor->WriteChangesToFiles();
  return true;
}

//Intentionally left as empty
void StmtInstrumentation::HandleTranslationUnit(clang::ASTContext &Ctx) {
  //spdlog::get("Logger")->info("Reached HandleTranslationUnit");
}


bool StmtCollectionVisitor::VisitFunctionDecl(FunctionDecl* fd) {
  //spdlog::get("Logger")->info("Visit Function Decl: {}", fd->getNameInfo().getAsString());

  if (fd->isThisDeclarationADefinition() && fd->hasBody()) {
    if (RwTool.isInMainFile(fd->getLocation())) {

      SourceLocation fd_begin = fd->getBeginLoc();
      SourceLocation fd_end = fd->getEndLoc();
  
      if (fd_begin.isValid() && fd_end.isValid()) {
	clang::SourceManager& theSM = RwTool.GetSourceManager();
	const clang::LangOptions& theLO = RwTool.GetLangOptions();

	fd_begin = theSM.getFileLoc(fd_begin);
	fd_end = theSM.getFileLoc(fd_end);

	std::string loc_str = std::to_string(theSM.getSpellingLineNumber(fd_begin));
	loc_str += ",";
	loc_str += std::to_string(theSM.getSpellingLineNumber(fd_end));
	spdlog::get("Logger")->info("function:{},{}", loc_str, fd->getNameInfo().getAsString());
      }

      if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(fd->getBody())) {
	VisitFunctionBodyCompoundStmtForPrint(cs);
      }
      else {
	VisitStmtForPrint(fd->getBody());
      }
    }
  }
  return true;
}

std::string StmtCollectionVisitor::GetFuncSignature(FunctionDecl* fd) {
  std::string fd_sig = "";
  std::string src_name = RwTool.GetSrcFilename(fd);
  std::string fd_name = fd->getNameInfo().getAsString();
  std::string param_types = "(";
  unsigned fd_param_num = fd->getNumParams();
  for (unsigned i=0; i<fd_param_num; i++) {
    if (i != 0) { param_types += ","; }
    param_types += clang::QualType::getAsString(fd->getParamDecl(i)->getType().split(),
						clang::PrintingPolicy{ {} });
  }
  param_types += ")";
  fd_sig += src_name;
  fd_sig += ":" + fd_name;
  fd_sig += param_types;
  return fd_sig;
}


bool StmtCollectionVisitor::VisitFunctionBodyCompoundStmtForPrint(const CompoundStmt* cs) {
  PrintStartAndEndLineNumbers(cs, "fbcompound");
  CompoundStmt::const_body_iterator bi, be;
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    VisitStmtForPrint(*bi);
  }
  return true;
}

bool StmtCollectionVisitor::VisitStmtForPrint(const Stmt* stmt) {
  //if (!RwTool.isInMainFile(stmt->getBeginLoc())) { return true; }
  //std::string src_fname = RwTool.GetSrcFilename(stmt->getBeginLoc());


  if (const NullStmt* ns = llvm::dyn_cast<NullStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "null");
  }

  else if (const ReturnStmt* rs = llvm::dyn_cast<ReturnStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "return");
  }
  
  else if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "compound");
    CompoundStmt::const_body_iterator bi, be;
    for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
      VisitStmtForPrint(*bi);
    }
  }
  
  else if (const IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "if");    
    VisitStmtForPrint(if_stmt->getThen());
    if (if_stmt->hasElseStorage()) {
      VisitStmtForPrint(if_stmt->getElse());
    }
  }
  
  else if (const DoStmt* ds = llvm::dyn_cast<DoStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "dowhile");
    VisitStmtForPrint(ds->getBody());
  }
  
  else if (const ForStmt* fs = llvm::dyn_cast<ForStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "for");
    VisitStmtForPrint(fs->getBody());
  }
  
  else if (const SwitchStmt* ss = llvm::dyn_cast<SwitchStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "switch");
    VisitStmtForPrint(ss->getBody());
  }
  
  else if (const WhileStmt* ws = llvm::dyn_cast<WhileStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "while");    
    VisitStmtForPrint(ws->getBody());
  }
  
  else if (const SwitchCase* sc = llvm::dyn_cast<SwitchCase>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "switchcase");
    VisitStmtForPrint(sc->getSubStmt());
  }
  
  else if (const LabelStmt* ls = llvm::dyn_cast<LabelStmt>(stmt)) {
    PrintStartAndEndLineNumbers(stmt, "label");    
    VisitStmtForPrint(ls->getSubStmt());
  }

  else {
    PrintStartAndEndLineNumbers(stmt, "other");
  }
  
  return true;
}

void StmtCollectionVisitor::InstruNonCompoundStmtAsStmtBody(const Stmt* stmt, std::string src_fname) {
  if (llvm::isa<NullStmt>(stmt)) {} //Do nothing
  else if (llvm::isa<LabelStmt>(stmt)) {} //Do nothing
  else if (llvm::isa<SwitchCase>(stmt)) {} //Do nothing
  else if (llvm::isa<ReturnStmt>(stmt)) { InstruStmtWithBraces(stmt, src_fname, 1); }
  else { InstruStmtWithBraces(stmt, src_fname, 0); }
}

void StmtCollectionVisitor::InstruCompoundStmtAsStmtBody(const CompoundStmt* cs, std::string src_fname) {
  InstruCompoundStmtAsStmtBody(cs, src_fname, true);
}

void StmtCollectionVisitor::InstruCompoundStmtAsStmtBody(const CompoundStmt* cs, std::string src_fname, bool instru_break) {
  CompoundStmt::const_body_iterator bi, be;
  //Instrument every stmt in the body (but don't look into the stmt)
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    //NOTE: a NULLStmt here can be either (1) a real null stmt or (2) a parsing failure
    //For either case, we ignore its instrumentation
    if (llvm::isa<NullStmt>(*bi)) { continue; }
    else if (llvm::isa<BreakStmt>(*bi) && !instru_break) { continue; }
    else if (llvm::isa<SwitchCase>(*bi)) { continue; }
    else if (llvm::isa<LabelStmt>(*bi)) { continue; }
    else if (llvm::isa<CompoundStmt>(*bi)) {
      const CompoundStmt* cs0 = llvm::dyn_cast<CompoundStmt>(*bi);
      InstruCompoundStmtAsStmtBody(cs0, src_fname, true); //This is not a switch body
    }
    else if (llvm::isa<ReturnStmt>(*bi)) { InstruStmt(*bi, src_fname, 1); }
    else { InstruStmt(*bi, src_fname, 0); }
  }
}

void StmtCollectionVisitor::InstruStmt(const Stmt* stmt, std::string src_fname, int is_return_stmt) {
  std::string stmt_leaftype = IsParentStmt(stmt) ? "Parent" : "Leaf";
  std::string stmt_id = src_fname+":Stmt" + std::to_string(stmt_count);
  std::string instru_str0 = "//" +stmt_id+":"+stmt_leaftype+ "-BEGIN\n" +
    "printf(\"<STMTID>"+stmt_id+":"+stmt_leaftype+"\\n\");\n";
  if (is_return_stmt) {
    instru_str0 += "printf(\"<FUNC-END>\\n\");\n"; //Add a function end mark
  }
  std::string instru_str1 = "\n//"+stmt_id+":"+stmt_leaftype+"-END\n";
  InstruStmt(stmt, instru_str0, instru_str1);
  stmt_count += 1;
}

void StmtCollectionVisitor::InstruStmtWithBraces(const Stmt* stmt, std::string src_fname, int is_return_stmt) {
  std::string stmt_leaftype = IsParentStmt(stmt) ? "Parent" :"Leaf";
  std::string stmt_id = src_fname+":Stmt" + std::to_string(stmt_count);
  std::string instru_str0 = "{//"+stmt_id+"-ADDED-BRACE\n//" +stmt_id+":"+stmt_leaftype+ "-BEGIN\n" +
    "printf(\"<STMTID>"+stmt_id+":"+stmt_leaftype+"\\n\");\n";
  if (is_return_stmt) {
    instru_str0 += "printf(\"<FUNC-END>\\n\");\n"; //Add a function end mark
  }
  std::string instru_str1 = "\n//"+stmt_id+":"+stmt_leaftype+"-END\n}//"+stmt_id+"-ADDED-BRACE\n";
  InstruStmt(stmt, instru_str0, instru_str1);
  stmt_count += 1;
}

void StmtCollectionVisitor::PrintStartAndEndLineNumbers(const Stmt* stmt, std::string type) {
  SourceLocation stmt_begin = stmt->getBeginLoc();
  SourceLocation stmt_end = stmt->getEndLoc();

  if (stmt_begin.isValid() && stmt_end.isValid()) {
    clang::SourceManager& theSM = RwTool.GetSourceManager();
    const clang::LangOptions& theLO = RwTool.GetLangOptions();

    stmt_begin = theSM.getFileLoc(stmt_begin);
    stmt_end = theSM.getFileLoc(stmt_end);
    
    std::string loc_str = std::to_string(theSM.getSpellingLineNumber(stmt_begin));
    loc_str += ",";
    loc_str += std::to_string(theSM.getSpellingLineNumber(stmt_end));

    spdlog::get("Logger")->info("statement:{},{}", loc_str, type);
  }
}

void StmtCollectionVisitor::InstruStmt(const Stmt* stmt, std::string instru_str0, std::string instru_str1) {
  SourceLocation stmt_begin = stmt->getBeginLoc();
  SourceLocation stmt_end = stmt->getEndLoc();
  
  if (stmt_begin.isValid() && stmt_end.isValid()) {
    //Adjust to not use macro locations
    if (stmt_begin.isMacroID()) {
      clang::SourceManager& theSM = RwTool.GetSourceManager();
      const clang::LangOptions& theLO = RwTool.GetLangOptions();
      clang::Lexer::isAtStartOfMacroExpansion(stmt_begin, theSM, theLO, &stmt_begin);
      stmt_begin = clang::Lexer::GetBeginningOfToken(stmt_begin, theSM, theLO);
      //isAtStartOfMacroExpansion(stmt_begin, theSM, theLO, &stmt_begin);
      //stmt_begin = GetBeginningOfToken(stmt_begin, theSM, theLO);
    }
    
    if (stmt_end.isMacroID()) {
      clang::SourceManager& theSM = RwTool.GetSourceManager();
      const clang::LangOptions& theLO = RwTool.GetLangOptions();
      clang::Lexer::isAtEndOfMacroExpansion(stmt_end, theSM, theLO, &stmt_end);
      stmt_end = clang::Lexer::getLocForEndOfToken(stmt_end, 0, theSM, theLO);
      //isAtEndOfMacroExpansion(stmt_end, theSM, theLO, &stmt_end);
      //stmt_end = getLocForEndOfToken(stmt_end, 0, theSM, theLO);
    }
    
    SourceLocation stmt_end1 = RwTool.FindLocationAfterToken(stmt_end, clang::tok::semi);
    if (stmt_end1.isValid()) { stmt_end = stmt_end1; }
    if (stmt_begin.isValid()) {
      RwTool.InsertTextBefore(stmt_begin, instru_str0);
    }
    if (stmt_end.isValid()) {
      RwTool.InsertTextAfter(stmt_end, instru_str1);
    }
  }
}

bool StmtCollectionVisitor::IsParentStmt(const Stmt* stmt) {
  if (const IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(stmt)) { return true; }
  else if (const DoStmt* ds = llvm::dyn_cast<DoStmt>(stmt)) { return true; }
  else if (const ForStmt* fs = llvm::dyn_cast<ForStmt>(stmt)) { return true; }
  else if (const SwitchStmt* ss = llvm::dyn_cast<SwitchStmt>(stmt)) { return true; }
  else if (const WhileStmt* ws = llvm::dyn_cast<WhileStmt>(stmt)) { return true; }
  else if (const SwitchCase* sc = llvm::dyn_cast<SwitchCase>(stmt)) { return true; }
  else if (const LabelStmt* ls = llvm::dyn_cast<LabelStmt>(stmt)) { return true; }
  else { return false; }
}

bool StmtCollectionVisitor::WriteChangesToFiles() {
  bool rslt = RwTool.WriteChangesToFiles();
  return rslt;
}
