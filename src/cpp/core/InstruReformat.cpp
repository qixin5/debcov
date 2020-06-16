#include "InstruReformat.h"

#include "clang/Format/Format.h"
#include "clang/Tooling/Inclusions/HeaderIncludes.h"

#include "InstruOptionManager.h"

void InstruReformat::Initialize(clang::ASTContext &Ctx) {
  InstruTransformation::Initialize(Ctx);
}

std::vector<clang::tooling::Range>
InstruReformat::createRanges(llvm::MemoryBuffer *Code) {
  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> InMemoryFileSystem(
      new llvm::vfs::InMemoryFileSystem);
  clang::FileManager Files(clang::FileSystemOptions(), InMemoryFileSystem);
  clang::SourceManager *SM = &Context->getSourceManager();
  InMemoryFileSystem.get()->addFileNoOwn("temp", 0, Code);
  clang::FileID ID = SM->createFileID(
      Files.getFile("temp"), clang::SourceLocation(), clang::SrcMgr::C_User);

  unsigned Offset = SM->getFileOffset(SM->getLocForStartOfFile(ID));
  unsigned Length = SM->getFileOffset(SM->getLocForEndOfFile(ID)) - Offset;
  return {clang::tooling::Range(Offset, Length)};
}

void InstruReformat::doReformatting(std::string FileName,
                              clang::format::FormatStyle FS) {
  std::unique_ptr<llvm::MemoryBuffer> Code =
      std::move(llvm::MemoryBuffer::getFile(FileName).get());
  clang::tooling::Replacements Rs =
      reformat(FS, Code->getBuffer(), createRanges(Code.get()), FileName);
  clang::tooling::applyAllReplacements(Rs, TheRewriter);
  TheRewriter.overwriteChangedFiles();
}

void InstruReformat::HandleTranslationUnit(clang::ASTContext &Ctx) {
  doReformatting(InstruOptionManager::InputFile, clang::format::getLLVMStyle());
}
