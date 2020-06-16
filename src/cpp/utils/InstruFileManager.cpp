#include "InstruFileManager.h"

#include <libgen.h>
#include <unistd.h>

#include "llvm/Support/FileSystem.h"

#include "InstruOptionManager.h"

InstruFileManager *InstruFileManager::Instance;

void InstruFileManager::Initialize() {
  Instance = new InstruFileManager();
  llvm::sys::fs::create_directory(InstruOptionManager::OutputDir);
}

InstruFileManager *InstruFileManager::GetInstance() {
  if (!Instance)
    Initialize();
  return Instance;
}

std::string InstruFileManager::getTempFileName(std::string Suffix) {
  std::string Name = InstruOptionManager::OutputDir + "/" +
                     Basename(InstruOptionManager::InputFile) + "." +
                     std::to_string(TempCounter) + "." + Suffix;
  TempCounter++;
  return Name;
}

std::string InstruFileManager::getSampleFileName(std::string SampleId, std::string Suffix) {
  std::string Name = InstruOptionManager::OutputDir + "/" +
                     Basename(InstruOptionManager::InputFile) + "." +
                     SampleId + "." + Suffix;
  return Name;
}

void InstruFileManager::saveTemp(std::string Phase, bool Status) {
  if (InstruOptionManager::SaveTemp) {
    std::string StatusString = Status ? ".success.c" : ".fail.c";
    llvm::sys::fs::copy_file(InstruOptionManager::InputFile,
                             getTempFileName(Phase) + StatusString);
  }
}

void InstruFileManager::saveSample(std::string SampleId) {
  if (InstruOptionManager::SaveSample) {
    llvm::sys::fs::copy_file(InstruOptionManager::InputFile,
                             getSampleFileName(SampleId, "c"));
  }
}

std::string InstruFileManager::Readlink(std::string &Name) {
  std::string buffer(64, '\0');
  ssize_t len;
  while ((len = ::readlink(Name.c_str(), &buffer[0], buffer.size())) ==
         static_cast<ssize_t>(buffer.size())) {
    buffer.resize(buffer.size() * 2);
  }
  if (len == -1) {
    return Name;
  }
  buffer.resize(len);
  return buffer;
}

std::string InstruFileManager::Dirname(std::string &Name) {
  char *Cstr = new char[Name.length() + 1];
  strcpy(Cstr, Name.c_str());
  ::dirname(Cstr);
  std::string Dir(Cstr);
  delete[] Cstr;
  return Dir;
}

std::string InstruFileManager::Basename(std::string &Name) {
  int Idx = Name.rfind('/', Name.length());
  if (Idx != std::string::npos) {
    return (Name.substr(Idx + 1, Name.length() - Idx));
  }
  return Name;
}

void InstruFileManager::Finalize() {
  delete Instance;
}
