#ifndef INSTRU_FRONTEND_H
#define INSTRU_FRONTEND_H

#include "clang/Parse/ParseAST.h"

#include <string>

/// \brief Provides an independent frontend for any action on AST
class InstruFrontend {
public:
  static bool Parse(std::string &FileName, clang::ASTConsumer *C);
};

#endif // INSTRU_FRONTEND_H
