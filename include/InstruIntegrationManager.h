#ifndef INSTRU_INTEGRATION_MANAGER_H
#define INSTRU_INTEGRATION_MANAGER_H

#include <map>
#include <string>
#include <vector>

/// \brief Manages build system integration
class InstruIntegrationManager {
public:
  static void Initialize();
  static void Finalize();
  static InstruIntegrationManager *GetInstance() { return Instance; }

  void capture();
  std::vector<std::string> &getOriginFilePaths() { return Origins; }
  std::vector<const char *> getCC1Args(std::string &FileName);

  std::map<std::string, std::vector<std::string>> CompilationDB;

private:
  static InstruIntegrationManager *Instance;
  std::vector<std::string> Origins;
  std::string WrapperDir;

  void buildCompilationDB();

  std::string CaptureFilePath;
};

#endif // INSTRU_INTEGRATION_MANAGER_H
