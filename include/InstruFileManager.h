#ifndef INSTRU_FILE_MANAGER_H
#define INSTRU_FILE_MANAGER_H

#include <string>
#include <vector>

/// \brief Wrapper for low-level file manipulations
class InstruFileManager {
public:
  static void Initialize();
  static void Finalize();
  static InstruFileManager *GetInstance();
  static std::string Readlink(std::string &Name);
  static std::string Dirname(std::string &Name);
  static std::string Basename(std::string &Name);

  std::string getTempFileName(std::string Suffix);
  std::string getSampleFileName(std::string SampleId, std::string Suffix);
  void saveTemp(std::string Phase, bool Status);
  void saveSample(std::string SampleId);

private:
  InstruFileManager() {}
  ~InstruFileManager() {}

  static InstruFileManager *Instance;

  int TempCounter = 0;
};

#endif // INSTRU_FILE_MANAGER_H
