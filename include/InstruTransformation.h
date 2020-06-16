#ifndef INSTRU_TRANSFORMATION_H
#define INSTRU_TRANSFORMATION_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/Rewrite/Core/Rewriter.h"

/// \brief Represesnts a transformation action on an AST
class InstruTransformation : public clang::ASTConsumer {
public:
  InstruTransformation() {}
  ~InstruTransformation() {}

  clang::Rewriter& getTheRewriter();

protected:
  virtual void Initialize(clang::ASTContext &Ctx);
  std::vector<clang::Stmt *> getAllChildren(clang::Stmt *S);
  std::vector<clang::Stmt *> getAllPrimitiveChildrenStmts(clang::CompoundStmt *S); 

  clang::ASTContext *Context;
  clang::Rewriter TheRewriter;
};

#endif // TRANSFORMATION_H
