#ifndef INSTRU_OPTION_MANAGER_H
#define INSTRU_OPTION_MANAGER_H

#include <string>
#include <vector>

/// \brief Manages and processes all the command-line options that are passed to Chisel
class InstruOptionManager {
public:
  static std::string BinFile;
  static std::vector<std::string> InputFiles;
  static std::vector<std::string> BuildCmd;
  static std::string InputFile;
  static std::string OracleFile;
  static std::string EvalResultFile;
  static std::string OutputDir;
  static bool Build;
  static bool SaveTemp;
  static bool SaveSample;
  static bool SkipLearning;
  static bool SkipDelayLearning;
  static bool NoCache;
  static bool SkipGlobalDep;
  static bool SkipLocalDep;
  static bool SkipDCE;
  static bool Profile;
  static bool Debug;
  static bool Stat;
  static int MaxSamples;
  static int MaxIters;
  static float Alpha;
  static float Beta;
  static float K;
  static std::string Granu;
  
  static void showUsage();
  static void handleOptions(int argc, char *argv[]);
};

#endif // INSTRU_OPTION_MANAGER_H
