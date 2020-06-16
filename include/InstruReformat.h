#ifndef INSTRU_REFORMAT_H
#define INSTRU_REFORMAT_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Format/Format.h"

#include "InstruTransformation.h"

/// \brief Represents a reformatting action
class InstruReformat : public InstruTransformation {
public:
  InstruReformat() {}
  ~InstruReformat() {}

private:
  void Initialize(clang::ASTContext &Ctx);
  void HandleTranslationUnit(clang::ASTContext &Ctx);
  void doReformatting(std::string FileName, clang::format::FormatStyle FS);
  std::vector<clang::tooling::Range> createRanges(llvm::MemoryBuffer *Code);
};

#endif // INSTRU_REFORMAT_H
