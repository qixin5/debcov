#include "BlockInstrumentation.h"

#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <unordered_map>

#include "clang/Lex/Lexer.h"

#include "InstruFileManager.h"
#include "InstruOptionManager.h"
#include "InstruProfiler.h"
#include "SourceManager.h"

using ArraySubscriptExpr = clang::ArraySubscriptExpr;
using BinaryOperator = clang::BinaryOperator;
using BreakStmt = clang::BreakStmt;
using CallExpr = clang::CallExpr;
using CastExpr = clang::CastExpr;
using ContinueStmt = clang::ContinueStmt;
using CompoundStmt = clang::CompoundStmt;
using Decl = clang::Decl;
using DeclGroupRef = clang::DeclGroupRef;
using DeclStmt = clang::DeclStmt;
using DeclRefExpr = clang::DeclRefExpr;
using DoStmt = clang::DoStmt;
using Expr = clang::Expr;
using ExplicitCastExpr = clang::ExplicitCastExpr;
using FieldDecl = clang::FieldDecl;
using ForStmt = clang::ForStmt;
using FunctionDecl = clang::FunctionDecl;
using GotoStmt = clang::GotoStmt;
using IfStmt = clang::IfStmt;
using LabelDecl = clang::LabelDecl;
using LabelStmt = clang::LabelStmt;
using MemberExpr = clang::MemberExpr;
using NullStmt = clang::NullStmt;
using ParenExpr = clang::ParenExpr;
using ReturnStmt = clang::ReturnStmt;
using SourceRange = clang::SourceRange;
using SourceLocation = clang::SourceLocation;
using Stmt = clang::Stmt;
using UnaryOperator = clang::UnaryOperator;
using WhileStmt = clang::WhileStmt;
using VarDecl = clang::VarDecl;
using ValueDecl = clang::ValueDecl;
using SwitchStmt = clang::SwitchStmt;
using SwitchCase = clang::SwitchCase;


void BlockInstrumentation::Initialize(clang::ASTContext &Ctx) {
  InstruTransformation::Initialize(Ctx);
  CollectionVisitor = new BlockCollectionVisitor(this, this->getTheRewriter());
}

void BlockInstrumentation::Finalize() {
  CollectionVisitor->WriteChangesToFiles();
}

bool BlockInstrumentation::HandleTopLevelDecl(DeclGroupRef DR) {
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
void BlockInstrumentation::HandleTranslationUnit(clang::ASTContext &Ctx) {
  //spdlog::get("Logger")->info("Reached HandleTranslationUnit");
}


bool BlockCollectionVisitor::VisitFunctionDecl(FunctionDecl* fd) {
  spdlog::get("Logger")->info("Visit Function Decl: {}", fd->getNameInfo().getAsString());

  if (fd->isThisDeclarationADefinition() && fd->hasBody()) {
    if (RwTool.isInMainFile(fd->getLocation())) {
      //Insert function-end instru code
      std::string fd_sig = GetFuncSignature(fd);
      std::string fd_sig_print_str0 = "printf(\"<FUNC-END>\\n\");\n";
      if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(fd->getBody())) {
	SourceLocation loc = cs->getEndLoc();
	if (loc.isMacroID()) {
	  clang::SourceManager& theSM = RwTool.GetSourceManager();
	  const clang::LangOptions& theLO = RwTool.GetLangOptions();
	  clang::Lexer::isAtEndOfMacroExpansion(loc, theSM, theLO, &loc);
	  loc = clang::Lexer::getLocForEndOfToken(loc, 0, theSM, theLO);
	}
	if (loc.isValid()) {
	  RwTool.InsertTextBefore(loc, fd_sig_print_str0);
	}
      }
      
      //Instru branch
      const CompoundStmt* fd_body_cs = llvm::dyn_cast<CompoundStmt>(fd->getBody());
      std::string src_fname = RwTool.GetSrcFilename(fd->getLocation());
      //std::string metrics_cmt = GetMetricsString(llvm::dyn_cast<Stmt>(fd_body_cs), 0, "func_body");
      InstruStmt(fd_body_cs, src_fname, 0, "func_body", -1, true, "", ""); //parent branch is empty
      
      
      //Insert function-begin instru code
      std::string fd_sig_print_str1 = "printf(\"<FUNC-BEGIN>"+fd_sig+"\\n\");\n";
      if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(fd->getBody())) {
	if (cs->body_empty()) {
	  SourceLocation loc = cs->getEndLoc();
	  if (loc.isMacroID()) {
	    clang::SourceManager& theSM = RwTool.GetSourceManager();
	    const clang::LangOptions& theLO = RwTool.GetLangOptions();
	    clang::Lexer::isAtEndOfMacroExpansion(loc, theSM, theLO, &loc);
	    loc = clang::Lexer::getLocForEndOfToken(loc, 0, theSM, theLO);
	  }
	  if (loc.isValid()) {
	    RwTool.InsertTextBefore(loc, fd_sig_print_str1);
	  }
	}
	else {
	  const Stmt* first_child = cs->body_front();
	  SourceLocation loc = first_child->getBeginLoc();
	  if (loc.isMacroID()) {
	    clang::SourceManager& theSM = RwTool.GetSourceManager();
	    const clang::LangOptions& theLO = RwTool.GetLangOptions();
	    clang::Lexer::isAtStartOfMacroExpansion(loc, theSM, theLO, &loc);
	    loc = clang::Lexer::GetBeginningOfToken(loc, theSM, theLO);
	  }
	  if (loc.isValid()) {
	    RwTool.InsertTextBefore(loc, fd_sig_print_str1);
	  }
	}
      }
      
    }
  }
  return true;
}

std::string BlockCollectionVisitor::GetFuncSignature(FunctionDecl* fd) {
  std::string fd_sig = "";
  std::string src_name = RwTool.GetSrcFilename(fd);
  std::string fd_name = fd->getNameInfo().getAsString();
  std::string param_types = "(";
  unsigned fd_param_num = fd->getNumParams();
  for (unsigned i=0; i<fd_param_num; i++) {
    if (i != 0) { param_types += ","; }
    param_types += clang::QualType::getAsString(fd->getParamDecl(i)->getType().split(), clang::PrintingPolicy{ {} });
  }
  param_types += ")";
  fd_sig += src_name;
  fd_sig += ":" + fd_name;
  fd_sig += param_types;
  return fd_sig;
}

bool BlockCollectionVisitor::IsBranchLeaf(const Stmt* stmt) {
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    CompoundStmt::const_body_iterator bi, be;
    for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
      if (const CompoundStmt* bi_cs = llvm::dyn_cast<CompoundStmt>(*bi)) { //Go into the block
	if (!IsBranchLeaf(bi_cs)) { 
	  return false; 
	}
      }
      
      /*
	else if (const SwitchCase* bi_sc = llvm::dyn_cast<SwitchCase>(*bi)) {
	const Stmt* bi_sc_substmt = bi_sc->getSubStmt();
	if (llvm::isa<CompoundStmt>(bi_sc_substmt)) { //Go into the block
	if (!IsBranchLeaf(bi_sc_substmt)) { 
	return false;
	}
	}
	else if (llvm::isa<IfStmt>(bi_sc_substmt) || llvm::isa<DoStmt>(bi_sc_substmt) || 
	llvm::isa<ForStmt>(bi_sc_substmt) || llvm::isa<SwitchStmt>(bi_sc_substmt) || 
	llvm::isa<WhileStmt>(bi_sc_substmt)) {
	return false;
	}
	}

	else if (const LabelStmt* bi_ls = llvm::dyn_cast<LabelStmt>(*bi)) {
	const Stmt* bi_ls_substmt = bi_ls->getSubStmt();
	if (llvm::isa<CompoundStmt>(bi_ls_substmt)) { //Go into the block
	if (!IsBranchLeaf(bi_ls_substmt)) {
	return false;
	}
	}
	else if (llvm::isa<IfStmt>(bi_ls_substmt) || llvm::isa<DoStmt>(bi_ls_substmt) || 
	llvm::isa<ForStmt>(bi_ls_substmt) || llvm::isa<SwitchStmt>(bi_ls_substmt) || 
	llvm::isa<WhileStmt>(bi_ls_substmt)) {
	return false;
	}
	}
      */
      
      else if (llvm::isa<IfStmt>(*bi) || llvm::isa<DoStmt>(*bi) || llvm::isa<ForStmt>(*bi) ||
	       llvm::isa<SwitchStmt>(*bi) || llvm::isa<WhileStmt>(*bi) || llvm::isa<SwitchCase>(*bi) ||
	       llvm::isa<LabelStmt>(*bi)) {
	return false;
      }
    }
    return true;
  }
  return false;
}

std::string BlockCollectionVisitor::GetStmtTypeCountString(const Stmt* stmt) {
  int counts_size = 14;
  int* counts = new int[counts_size];
  for (int i=0; i<counts_size; i++) { counts[i] = 0; }
  
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    counts = GetStmtTypeCountForBranch(cs);
  }
  
  //BreakStmt: 0; CompoundStmt: 1; ContinueStmt: 2, DeclStmt: 3; DoStmt: 4;
  //ForStmt: 5; GotoStmt: 6; IfStmt: 7; NullStmt: 8; ReturnStmt: 9; 
  //SwitchCase: 10; SwitchStmt: 11; LabelStmt: 12; WhileStmt: 13
  std::string count_str = 
    "breakstmt("+std::to_string(counts[0])+")," +
    "compoundstmt("+std::to_string(counts[1])+")," +
    "continuestmt("+std::to_string(counts[2])+")," +
    "declstmt("+std::to_string(counts[3])+")," +
    "dostmt("+std::to_string(counts[4])+")," +
    "forstmt("+std::to_string(counts[5])+")," +
    "gotostmt("+std::to_string(counts[6])+")," +
    "ifstmt("+std::to_string(counts[7])+")," +
    "nullstmt("+std::to_string(counts[8])+")," +
    "returnstmt("+std::to_string(counts[9])+")," +
    "switchcase("+std::to_string(counts[10])+")," +
    "switchstmt("+std::to_string(counts[11])+")," +
    "labelstmt("+std::to_string(counts[12])+")," +
    "whilestmt("+std::to_string(counts[13])+")";
  
  return count_str;
}

int* BlockCollectionVisitor::GetStmtTypeCountForBranch(const CompoundStmt* cs) {
  int counts_size = 14;
  int* counts = new int[counts_size];
  for (int i=0; i<counts_size; i++) { counts[i] = 0; }
  
  CompoundStmt::const_body_iterator bi, be;
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    //Top-level only, except for compound statement
    if (llvm::isa<BreakStmt>(*bi)) { counts[0] += 1; }
    
    else if (llvm::isa<CompoundStmt>(*bi)) {
      counts[1] += 1;
      const CompoundStmt* bi_cs = llvm::dyn_cast<CompoundStmt>(*bi);
      addCount(counts, GetStmtTypeCountForBranch(bi_cs), counts_size);
    }
    
    else if (llvm::isa<ContinueStmt>(*bi)) { counts[2] += 1; }
    else if (llvm::isa<DeclStmt>(*bi)) { counts[3] += 1; }
    else if (llvm::isa<DoStmt>(*bi)) { counts[4] += 1; }
    else if (llvm::isa<ForStmt>(*bi)) { counts[5] += 1; }
    else if (llvm::isa<GotoStmt>(*bi)) { counts[6] += 1; }
    else if (llvm::isa<IfStmt>(*bi)) { counts[7] += 1; }
    else if (llvm::isa<NullStmt>(*bi)) { counts[8] += 1; }
    else if (llvm::isa<ReturnStmt>(*bi)) { counts[9] += 1; }
    else if (llvm::isa<SwitchCase>(*bi)) { counts[10] += 1; }
    else if (llvm::isa<SwitchStmt>(*bi)) { counts[11] += 1; }
    else if (llvm::isa<LabelStmt>(*bi)) { counts[12] += 1; }
    else if (llvm::isa<WhileStmt>(*bi)) { counts[13] += 1; }
  }
  
  return counts;
}

std::string BlockCollectionVisitor::GetExprTypeCountString(const Stmt* stmt) {
  int counts_size = 10;
  int* counts = new int[counts_size];
  for (int i=0; i<counts_size; i++) { counts[i] = 0; }
  
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    counts = GetExprTypeCountForBranch(cs);
  }
  
  //Generate the string
  std::string count_str = 
    "arraysubscriptexpr("+std::to_string(counts[0])+")," +
    "binaryop-ptr("+std::to_string(counts[1])+")," +
    "binaryop-compute("+std::to_string(counts[2])+")," +
    "binaryop-relation("+std::to_string(counts[3])+")," +
    "binaryop-assign("+std::to_string(counts[4])+")," +
    "callexpr("+std::to_string(counts[5])+")," +
    "explicitcastexpr("+std::to_string(counts[6])+")," +
    "unaryop-increment("+std::to_string(counts[7])+")," +
    "unaryop-decrement("+std::to_string(counts[8])+")," +
    "unaryop-arithmetic("+std::to_string(counts[9])+")";
  
  return count_str;
}

int* BlockCollectionVisitor::GetExprTypeCountForBranch(const CompoundStmt* cs) {
  int counts_size = 10;
  int* counts = new int[counts_size];
  for (int i=0; i<counts_size; i++) { counts[i] = 0; }
  
  CompoundStmt::const_body_iterator bi, be;
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    //Only visit top-level exprs for non-primitive stmts, except for compound stmt
    if (const CompoundStmt* bi_cs = llvm::dyn_cast<CompoundStmt>(*bi)) {
      addCount(counts, GetExprTypeCountForBranch(bi_cs), counts_size); //Go in
    }
    if (const IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(*bi)) {
      addCount(counts, GetExprTypeCount(if_stmt->getCond()), counts_size);
    }
    else if (const DoStmt* ds = llvm::dyn_cast<DoStmt>(*bi)) {
      addCount(counts, GetExprTypeCount(ds->getCond()), counts_size);
    }
    else if (const ForStmt* fs = llvm::dyn_cast<ForStmt>(*bi)) {
      addCount(counts, GetExprTypeCount(fs->getInit()), counts_size);
      addCount(counts, GetExprTypeCount(fs->getCond()), counts_size);
      addCount(counts, GetExprTypeCount(fs->getInc()), counts_size);
    }
    else if (const SwitchStmt* ss = llvm::dyn_cast<SwitchStmt>(*bi)) {
      addCount(counts, GetExprTypeCount(ss->getCond()), counts_size);
    }
    else if (const WhileStmt* ws = llvm::dyn_cast<WhileStmt>(*bi)) {
      addCount(counts, GetExprTypeCount(ws->getCond()), counts_size);
    }
    else if (const SwitchCase* sc = llvm::dyn_cast<SwitchCase>(*bi)) {
      //No need to look at SwitchCase, we only care about the top-level
    }
    else if (const LabelStmt* ls = llvm::dyn_cast<LabelStmt>(*bi)) {
      //No need to look at LabelStmt, we only care about the top-level
    }
    else {
      //GetExprTypeCount handles exprs
      addCount(counts, GetExprTypeCount(*bi), counts_size);
    }
  }
  
  return counts;
}

void BlockCollectionVisitor::addCount(int* c0, int* c1, int size) {
  //int size = sizeof(c0)/sizeof(*c0);
  for (int i=0; i<size; i++) {
    c0[i] += c1[i];
  }
}

int* BlockCollectionVisitor::GetExprTypeCount(const Stmt* stmt) {
  //ArraySubscriptExpr: 0;
  //BinaryOperator-ptr: 1; BinaryOperator-compute: 2; BinaryOperator-relation: 3;
  //BinaryOperator-assign: 4; CallExpr: 5; CastExpr: 6;
  //UnaryOperator-increment: 7; UnaryOperator-increment: 8; UnaryOperator-arith: 9;
  int counts_size = 10;
  int* counts = new int[counts_size];
  for (int i=0; i<counts_size; i++) { counts[i] = 0; }
  if (stmt == NULL) { return counts; }
  
  if (const BreakStmt* break_stmt = llvm::dyn_cast<BreakStmt>(stmt)) {
    //Do nothing ... (no sub-structure to explore)
  }
  
  else if (const ContinueStmt* cont_stmt = llvm::dyn_cast<ContinueStmt>(stmt)) {
    //Do nothing ... (no sub-structure to explore)
  }
  
  else if (const DeclStmt* decl_stmt = llvm::dyn_cast<DeclStmt>(stmt)) {
    if (decl_stmt->isSingleDecl()) {
      const Decl* decl = decl_stmt->getSingleDecl();
      if (const VarDecl* var_decl = llvm::dyn_cast<VarDecl>(decl)) {
	if (var_decl->hasInit()) {
	  //Visit initializer
	  addCount(counts, GetExprTypeCount(var_decl->getInit()), counts_size);
	}
      }
      //I don't think we need to handle other decls ...
    }
  }
  
  else if (const GotoStmt* goto_stmt = llvm::dyn_cast<GotoStmt>(stmt)) {
    //Do nothing ... (no need to explore the label part)
  }
  
  else if (const NullStmt* null_stmt = llvm::dyn_cast<NullStmt>(stmt)) {
    //Do nothing ...
  }
  
  else if (const ReturnStmt* return_stmt = llvm::dyn_cast<ReturnStmt>(stmt)) {
    addCount(counts, GetExprTypeCount(return_stmt->getRetValue()), counts_size);
  }
  
  else if (const ArraySubscriptExpr* ase = llvm::dyn_cast<ArraySubscriptExpr>(stmt)) {
    //===============
    //llvm::outs() << "ArraySubscriptExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    counts[0] += 1;
    addCount(counts, GetExprTypeCount(ase->getBase()), counts_size);
    addCount(counts, GetExprTypeCount(ase->getIdx()), counts_size);
    //Need to call getLHS() & getRHS()?
  }
  else if (const BinaryOperator* bop = llvm::dyn_cast<BinaryOperator>(stmt)) {
    //===============
    //llvm::outs() << "BinaryOp: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    if (bop->isPtrMemOp()) {
      counts[1] += 1;
    }
    else if (bop->isMultiplicativeOp() ||
	     bop->isAdditiveOp() ||
	     bop->isShiftOp() ||
	     bop->isBitwiseOp()) { 
      counts[2] += 1; 
    }
    else if (bop->isRelationalOp() ||
	     bop->isEqualityOp() ||
	     bop->isComparisonOp() ||
	     bop->isLogicalOp()) {
      counts[3] += 1; 
    }
    else if (bop->isAssignmentOp() ||
	     bop->isCompoundAssignmentOp() ||
	     bop->isShiftAssignOp()) {
      counts[4] += 1; 
    }
    
    //Visit children
    addCount(counts, GetExprTypeCount(bop->getLHS()), counts_size);
    addCount(counts, GetExprTypeCount(bop->getRHS()), counts_size);
  }
  else if (const CallExpr* callexpr = llvm::dyn_cast<CallExpr>(stmt)) {
    //===============
    //llvm::outs() << "CallExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    counts[5] += 1;
    addCount(counts, GetExprTypeCount(callexpr->getCallee()), counts_size);
    const Expr *const * args = callexpr->getArgs();
    unsigned arg_num = callexpr->getNumArgs();
    for (int i=0; i<arg_num; i++) {
      addCount(counts, GetExprTypeCount(args[i]), counts_size);
    }
  }
  else if (const CastExpr* castexpr = llvm::dyn_cast<CastExpr>(stmt)) {
    //===============
    //llvm::outs() << "CastExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    if (const ExplicitCastExpr* ecastexpr = llvm::dyn_cast<ExplicitCastExpr>(stmt)) {
      counts[6] += 1; 
    }
    
    addCount(counts, GetExprTypeCount(castexpr->getSubExpr()), counts_size);
  }
  else if (const MemberExpr* memexpr = llvm::dyn_cast<MemberExpr>(stmt)) {
    addCount(counts, GetExprTypeCount(memexpr->getBase()), counts_size);
    //addCount(counts, GetExprTypeCount(memexpr->getMemberDecl()), counts_size); //This is a decl
  }
  else if (const UnaryOperator* uop = llvm::dyn_cast<UnaryOperator>(stmt)) {
    //===============
    //llvm::outs() << "UnaryOperator: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    if (uop->isIncrementOp()) {
      counts[7] += 1;
    }
    else if (uop->isDecrementOp()) {
      counts[8] += 1;
    }
    else if (uop->isArithmeticOp()) {
      counts[9] += 1;
    }      
    //Visit Children
    addCount(counts, GetExprTypeCount(uop->getSubExpr()), counts_size);
  }
  else if (const ParenExpr* pe = llvm::dyn_cast<ParenExpr>(stmt)) {
    //Directly visit children
    addCount(counts, GetExprTypeCount(pe->getSubExpr()), counts_size);
  }
  else if (const DeclRefExpr* dre = llvm::dyn_cast<DeclRefExpr>(stmt)) {
    //Do nothing ...
  }
  return counts;
}

std::string BlockCollectionVisitor::GetNamesString(const Stmt* stmt) {
  std::vector<std::string> names;
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    GetNamesForBranch(cs, names);
  }
  
  //Generate the string
  std::string names_str = "";
  int names_size = names.size();
  for (int i=0; i<names_size; i++) {
    if (i!=0) { names_str += ","; }
    names_str += names.at(i);
  }
  
  return names_str;
}

void BlockCollectionVisitor::addVectorElems(std::vector<std::string> &vec0, std::vector<std::string> &vec1) {
  int vec1_size = vec1.size();
  for (int i=0; i<vec1_size; i++) {
    vec0.push_back(vec1.at(i));
  }
}

void BlockCollectionVisitor::GetNamesForBranch(const CompoundStmt* cs, std::vector<std::string> &names) {
  CompoundStmt::const_body_iterator bi, be;
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    if (const CompoundStmt* bi_cs = llvm::dyn_cast<CompoundStmt>(*bi)) {
      GetNamesForBranch(bi_cs, names);
    }
    else if (const IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(*bi)) {
      GetNames(if_stmt->getCond(), names);
    }
    else if (const DoStmt* ds = llvm::dyn_cast<DoStmt>(*bi)) {
      GetNames(ds->getCond(), names);
    }
    else if (const ForStmt* fs = llvm::dyn_cast<ForStmt>(*bi)) {
      GetNames(fs->getInit(), names);
      GetNames(fs->getCond(), names);
      GetNames(fs->getInc(), names);
    }
    else if (const SwitchStmt* ss = llvm::dyn_cast<SwitchStmt>(*bi)) {
      GetNames(ss->getCond(), names);
    }
    else if (const WhileStmt* ws = llvm::dyn_cast<WhileStmt>(*bi)) {
      GetNames(ws->getCond(), names);
    }
    else if (const SwitchCase* sc = llvm::dyn_cast<SwitchCase>(*bi)) {
      //No need to look at SwitchCase, we only care about the top-level
    }
    else if (const LabelStmt* ls = llvm::dyn_cast<LabelStmt>(*bi)) {
      //No need to look at LabelStmt, we only care about the top-level
    }
    else {
      //GetNames handles primitive stmts and exprs with recursion
      GetNames(*bi, names);
    }
  }
}

void BlockCollectionVisitor::GetNames(const Stmt* stmt, std::vector<std::string> &names) {
  if (stmt == NULL) { return; }
  
  if (const BreakStmt* break_stmt = llvm::dyn_cast<BreakStmt>(stmt)) {
    //Do nothing ... (no sub-structure to explore)
  }
  
  else if (const ContinueStmt* cont_stmt = llvm::dyn_cast<ContinueStmt>(stmt)) {
    //Do nothing ... (no sub-structure to explore)
  }
  
  else if (const DeclStmt* decl_stmt = llvm::dyn_cast<DeclStmt>(stmt)) {
    if (decl_stmt->isSingleDecl()) {
      const Decl* decl = decl_stmt->getSingleDecl();
      if (const VarDecl* var_decl = llvm::dyn_cast<VarDecl>(decl)) {
	names.push_back(var_decl->getNameAsString()+"(v)");
	if (var_decl->hasInit()) {
	  GetNames(var_decl->getInit(), names); //Visit initializer
	}
      }
      //I don't think we need to handle other decls ...
    }
  }
  
  else if (const GotoStmt* goto_stmt = llvm::dyn_cast<GotoStmt>(stmt)) {
    //Do nothing ... (no need to explore the label part)
  }
  
  else if (const NullStmt* null_stmt = llvm::dyn_cast<NullStmt>(stmt)) {
    //Do nothing ...
  }
  
  else if (const ReturnStmt* return_stmt = llvm::dyn_cast<ReturnStmt>(stmt)) {
    GetNames(return_stmt->getRetValue(), names);
  }
  
  else if (const ArraySubscriptExpr* ase = llvm::dyn_cast<ArraySubscriptExpr>(stmt)) {
    //===============
    //llvm::outs() << "ArraySubscriptExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    //Visit children
    GetNames(ase->getBase(), names);
    GetNames(ase->getIdx(), names);
    //Need to call getLHS() & getRHS()?
  }
  else if (const BinaryOperator* bop = llvm::dyn_cast<BinaryOperator>(stmt)) {
    //===============
    //llvm::outs() << "BinaryOp: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    //Visit children
    GetNames(bop->getLHS(), names);
    GetNames(bop->getRHS(), names);
  }
  else if (const CallExpr* callexpr = llvm::dyn_cast<CallExpr>(stmt)) {
    //===============
    //llvm::outs() << "CallExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    GetNames(callexpr->getCallee(), names);
    const Expr *const * args = callexpr->getArgs();
    unsigned arg_num = callexpr->getNumArgs();
    for (int i=0; i<arg_num; i++) {
      GetNames(args[i], names);
    }
  }
  else if (const CastExpr* castexpr = llvm::dyn_cast<CastExpr>(stmt)) {
    //===============
    //llvm::outs() << "CastExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    GetNames(castexpr->getSubExpr(), names);
  }
  else if (const MemberExpr* memexpr = llvm::dyn_cast<MemberExpr>(stmt)) {
    GetNames(memexpr->getBase(), names); //Handle base part
    const ValueDecl* decl = memexpr->getMemberDecl(); //Handle decl part
    if (const VarDecl* var_decl = llvm::dyn_cast<VarDecl>(decl)) {
      names.push_back(var_decl->getNameAsString()+"(v)");
      if (var_decl->hasInit()) {
	GetNames(var_decl->getInit(), names); //Visit initializer                               
      }
    }
    else if (const FieldDecl* field_decl = llvm::dyn_cast<FieldDecl>(decl)) {
      names.push_back(field_decl->getNameAsString()+"(v)");
    }
    else if (const FunctionDecl* func_decl = llvm::dyn_cast<FunctionDecl>(decl)) {
      names.push_back(func_decl->getNameAsString()+"(f)");
    }
  }
  else if (const UnaryOperator* uop = llvm::dyn_cast<UnaryOperator>(stmt)) {
    //===============
    //llvm::outs() << "UnaryOperator: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    //Visit Children
    GetNames(uop->getSubExpr(), names);
  }
  else if (const ParenExpr* pe = llvm::dyn_cast<ParenExpr>(stmt)) {
    //===============
    //llvm::outs() << "ParenExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    GetNames(pe->getSubExpr(), names);
  }
  else if (const DeclRefExpr* dre = llvm::dyn_cast<DeclRefExpr>(stmt)) {
    //===============
    //llvm::outs() << "DeclRefExpr: " << RwTool.GetPrettyPrintText(stmt) << "\n";
    //===============
    
    const ValueDecl* value_decl = dre->getDecl();
    if (const VarDecl* var_decl = llvm::dyn_cast<VarDecl>(value_decl)) {
      names.push_back(var_decl->getNameAsString()+"(v)");
    }
    else if (const FieldDecl* field_decl = llvm::dyn_cast<FieldDecl>(value_decl)) {
      names.push_back(field_decl->getNameAsString()+"(v)");
    }
    else if (const FunctionDecl* func_decl = llvm::dyn_cast<FunctionDecl>(value_decl)) {
      names.push_back(func_decl->getNameAsString()+"(f)");
    }
  }
}

std::string BlockCollectionVisitor::GetGotoLabels(const Stmt* stmt) {
  std::vector<std::string> labels;
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    GetGotoLabelsForBranch(cs, labels);
  }
  
  //Generate the string                                                 
  std::string labels_str = "";
  int labels_size = labels.size();
  for (int i=0; i<labels_size; i++) {
    if (i!=0) { labels_str += ","; }
    labels_str += labels.at(i);
  }
  
  return labels_str;
}

void BlockCollectionVisitor::GetGotoLabelsForBranch(const CompoundStmt* cs, std::vector<std::string> &labels) {
  CompoundStmt::const_body_iterator bi, be;
  for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
    //Only care about the top-level, except for compound stmt
    if (const CompoundStmt* bi_cs = llvm::dyn_cast<CompoundStmt>(*bi)) {
      GetGotoLabelsForBranch(bi_cs, labels);
    }
    else if (const GotoStmt* gts = llvm::dyn_cast<GotoStmt>(*bi)) {
      labels.push_back(gts->getLabel()->getNameAsString());
    }
  }
}

std::string BlockCollectionVisitor::GetMetricsString(const Stmt* stmt, int depth, std::string branch_type, std::string label_branch_label) {
  std::string cover_input_cmt = "//COVERINPUT:";
  std::string cover_appr_cmt = "//COVERAPPROACH:";
  std::string depth_cmt = "//DEPTH:" + std::to_string(depth);
  std::string branch_type_cmt = "//BRANCHTYPE:" + branch_type;
  
  //Compute # of Lines
  SourceLocation begin_loc = stmt->getBeginLoc();
  SourceLocation end_loc = stmt->getEndLoc();
  unsigned begin_lnum = (begin_loc.isValid()) ? (RwTool.getSrcLocation(begin_loc).first) : -1;
  unsigned end_lnum = (end_loc.isValid()) ? (RwTool.getSrcLocation(end_loc).first) : -1;
  unsigned llength = ((begin_lnum == -1) || (end_lnum == -1)) ? -1 : (end_lnum-begin_lnum+1);
  std::string branch_length_cmt = "//BRANCHLENGTH:" + std::to_string(llength);
  
  //Check if it's a leaf
  std::string is_leaf_cmt = "//ISLEAF:";
  if (IsBranchLeaf(stmt)) { is_leaf_cmt += "true"; }
  else { is_leaf_cmt += "false"; }
  
  //Get Stmt Count
  std::string stmt_count_cmt = "//STMTCOUNT:";
  stmt_count_cmt += GetStmtTypeCountString(stmt);
  
  //Get Expr Count
  std::string expr_count_cmt = "//EXPRCOUNT:";
  expr_count_cmt += GetExprTypeCountString(stmt);
  
  //Get Names (variable and function)
  std::string names_cmt = "//NAMES:";
  names_cmt += GetNamesString(stmt);
  
  //Add label (for non-label branch, this should be empty)
  std::string decl_label_cmt = "//DECLLABEL:";
  decl_label_cmt += label_branch_label;
  
  //Get Goto Labels
  std::string goto_label_cmt = "//GOTOLABELS:";
  goto_label_cmt += GetGotoLabels(stmt);
  
  return cover_input_cmt +"\n"+ cover_appr_cmt +"\n"+ depth_cmt +"\n"+
    branch_type_cmt +"\n"+ branch_length_cmt +"\n"+ is_leaf_cmt +"\n"+
    stmt_count_cmt +"\n"+ expr_count_cmt + "\n" + names_cmt + "\n" +
    decl_label_cmt +"\n"+ goto_label_cmt;
}

/* add_instru_str_if_compound: if stmt is a compound statement, do we instru it or not? If stmt is a compound statement which is also from a compound statement, then we don't. 
   branch_type: If stmt is a compound statement, what is its type?
   par_branch_id: What is stmt's branch id (the branch where stmt is in)?
   par_branch_type: What is stmt's branch type (the branch where stmt is in)? 
   label_branch_label: If stmt is a compound statement and is a label branch, this is the label string.
*/
void BlockCollectionVisitor::InstruStmt(const Stmt* stmt, std::string src_fname,
		  int depth, std::string branch_type, int par_branch_id,
		  bool add_instru_str_if_compound, std::string par_branch_type,
		  std::string label_branch_label) {
  //Process by type
  //Compound Stmt
  if (const CompoundStmt* cs = llvm::dyn_cast<CompoundStmt>(stmt)) {
    int cs_branch_id = block_count; //the current branch id
    
    if (add_instru_str_if_compound) { //If we choose to instru this compound stmt (e.g., as if-then-body)
      InstruBranch(cs, src_fname, depth, branch_type, par_branch_id, label_branch_label); //curr_branch is cs's parent
      CompoundStmt::const_body_iterator bi, be;
      for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
	InstruStmt(*bi, src_fname, depth, "compound_stmt_body", cs_branch_id, false, branch_type, ""); //The current branch id (cs_branch_id) is its children's parent branch id. If the stmt is compound, do not instru it (use false), but visit inner stmts. Since the children stmts are from a compound stmt rather than a label stmt, empty label is used.
      }
    }
    else { //If this is an unnecessary block (e.g., if (...) { { s; } })
      CompoundStmt::const_body_iterator bi, be;
      for (bi=cs->body_begin(), be=cs->body_end(); bi!=be; bi++) {
	InstruStmt(*bi, src_fname, depth, branch_type, par_branch_id, false, par_branch_type, label_branch_label); //This block is ignored, so use this block's parent id (which is curr_branch_id). Also, use this block's enclosing branch's type (which is curr_branch_type)
      }
    }
  }
  
  //If Stmt
  else if (const IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(stmt)) {
    InstruStmt(if_stmt->getThen(), src_fname, depth+1, "if_then_body", par_branch_id, true, par_branch_type, ""); //If-body's par branch is if's par branch, so pass par_branch_id here.
    if (if_stmt->hasElseStorage()) {
      InstruStmt(if_stmt->getElse(), src_fname, depth+1, "if_else_body", par_branch_id, true, par_branch_type, "");
    }
  }
  
  //Do Stmt
  else if (const DoStmt* ds = llvm::dyn_cast<DoStmt>(stmt)) {
    InstruStmt(ds->getBody(), src_fname, depth+1, "dowhile_stmt_body", par_branch_id, true, par_branch_type, "");
  }
  
  //For Stmt
  else if (const ForStmt* fs = llvm::dyn_cast<ForStmt>(stmt)) {
    InstruStmt(fs->getBody(), src_fname, depth+1, "for_stmt_body", par_branch_id, true, par_branch_type, "");
  }
  
  //Switch Stmt
  else if (const SwitchStmt* ss = llvm::dyn_cast<SwitchStmt>(stmt)) {
    InstruStmt(ss->getBody(), src_fname, depth+1, "switch_stmt_body", par_branch_id, true, par_branch_type, "");
  }
  
  //While Stmt
  else if (const WhileStmt* ws = llvm::dyn_cast<WhileStmt>(stmt)) {
    InstruStmt(ws->getBody(), src_fname, depth+1, "while_stmt_body", par_branch_id, true, par_branch_type, "");
  }
  
  //Switch Case
  else if (const SwitchCase* sc = llvm::dyn_cast<SwitchCase>(stmt)) {
    //***************
    //RQ: Should use depth+1 or depth? Should use which parent id?
    //Too many nested levels if increasing depth?
    //***************
    InstruStmt(sc->getSubStmt(), src_fname, depth+1, "switch_case_body", par_branch_id, true, par_branch_type, ""); //The substmt should be a block including all cased stmts (such block should be generated by label normalizer)
  }
  
  //Label Stmt
  else if (const LabelStmt* ls = llvm::dyn_cast<LabelStmt>(stmt)) {
    //***************
    //RQ: Should use depth+1 or depth? Should use which parent id?
    //Too many nested levels if increasing depth?
    //***************
    std::string label_str = ls->getDecl()->getNameAsString();
    InstruStmt(ls->getSubStmt(), src_fname, depth+1, "label_stmt_body", par_branch_id, true, par_branch_type, label_str); //The substmt should be a block including all stmts up to the end of the control-flow (such block should be generated by label normalizer)
  }
}

void BlockCollectionVisitor::InstruBranch(const CompoundStmt* cs, std::string src_fname,
		    int depth, std::string branch_type, int par_id,
		    std::string label_branch_label) {
  if (!cs->body_empty()) {
    //Begin instrument
    std::string metrics_str = GetMetricsString(cs, depth, branch_type, label_branch_label);
    const Stmt* first_child = cs->body_front();
    std::string branch_id = src_fname + ":Branch" + std::to_string(block_count);
    std::string instru_str0 = "//" + branch_id + "(" + std::to_string(par_id) + ")" + "-BEGIN\n" //begin header
      + metrics_str + "\n" + //branch metrics
      "printf(\"<BRANCHID>"+branch_id+"\\n\");\n"; //print stmt
    InsertBefore(first_child, instru_str0);
    
    //End instrument
    const Stmt* last_child = cs->body_back();
    std::string instru_str1 = "\n//" + branch_id + "(" + std::to_string(par_id) + ")" + "-END\n"; //end header
    InsertAfter(last_child, instru_str1);
    block_count += 1;
  }
}

void BlockCollectionVisitor::InsertBefore(const Stmt* stmt, std::string instru_str) {
  SourceLocation stmt_begin = stmt->getBeginLoc();
  if (stmt_begin.isValid()) {
    if (stmt_begin.isMacroID()) {
      clang::SourceManager& theSM = RwTool.GetSourceManager();
      const clang::LangOptions& theLO = RwTool.GetLangOptions();
      clang::Lexer::isAtStartOfMacroExpansion(stmt_begin, theSM, theLO, &stmt_begin);
      stmt_begin = clang::Lexer::GetBeginningOfToken(stmt_begin, theSM, theLO);
    }
  }
  
  if (stmt_begin.isValid()) {
    RwTool.InsertTextBefore(stmt_begin, instru_str);
  }
}

void BlockCollectionVisitor::InsertAfter(const Stmt* stmt, std::string instru_str) {
  SourceLocation stmt_end = stmt->getEndLoc();
  if (stmt_end.isValid()) {
    if (stmt_end.isMacroID()) {
      clang::SourceManager& theSM = RwTool.GetSourceManager();
      const clang::LangOptions& theLO = RwTool.GetLangOptions();
      clang::Lexer::isAtEndOfMacroExpansion(stmt_end, theSM, theLO, &stmt_end);
      stmt_end = clang::Lexer::getLocForEndOfToken(stmt_end, 0, theSM, theLO);
    }
  }
  
  SourceLocation stmt_end1 = RwTool.FindLocationAfterToken(stmt_end, clang::tok::semi);
  if (stmt_end1.isValid()) { stmt_end = stmt_end1; }
  if (stmt_end.isValid()) {
    RwTool.InsertTextAfter(stmt_end, instru_str);
  }
}


bool BlockCollectionVisitor::WriteChangesToFiles() {
  bool rslt = RwTool.WriteChangesToFiles();
  return rslt;
}
