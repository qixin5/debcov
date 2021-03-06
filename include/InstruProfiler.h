#ifndef INSTRU_PROFILER_H
#define INSTRU_PROFILER_H

#include "llvm/Support/Timer.h"

/// \brief Keeps track of performance information that is used in preparing the report
class InstruProfiler {
public:
  static void Initialize();
  static InstruProfiler *GetInstance();
  static void Finalize();

  void incrementGlobalReductionCounter();
  void incrementSuccessfulGlobalReductionCounter();
  void incrementLocalReductionCounter();
  void incrementSuccessfulLocalReductionCounter();
  void setBestSampleId(int Id);
  int getBestSampleId();
  void setBestSizeRScore(float SRScore);
  float getBestSizeRScore();
  void setBestAttackSurfaceRScore(float ARScore);
  float getBestAttackSurfaceRScore();
  void setBestRScore(float RScore);
  float getBestRScore();
  void setBestGScore(float GScore);
  float getBestGScore();
  void setBestOScore(float OScore);
  float getBestOScore();

  int getGlobalReductionCounter() { return GlobalReductionCounter; }
  int getSuccessfulGlobalReductionCounter() {
    return SuccessfulGlobalReductionCounter;
  }
  int getLocalReductionCounter() { return LocalReductionCounter; }
  int getSuccessfulLocalReductionCounter() {
    return SuccessfulLocalReductionCounter;
  }

  llvm::Timer &getChiselTimer() { return ChiselTimer; }
  llvm::Timer &getLearningTimer() { return LearningTimer; }
  llvm::Timer &getOracleTimer() { return OracleTimer; }

  llvm::TimeRecord &getChiselTimeRecord() { return ChiselTimeRecord; }
  llvm::TimeRecord &getLearningTimeRecord() { return LearningTimeRecord; }
  llvm::TimeRecord &getOracleTimeRecord() { return OracleTimeRecord; }

  void beginChisel();
  void endChisel();

  void beginOracle();
  void endOracle();

  void beginLearning();
  void endLearning();

private:
  InstruProfiler() {}
  ~InstruProfiler() {}

  static InstruProfiler *Instance;

  int GlobalReductionCounter = 0;
  int SuccessfulGlobalReductionCounter = 0;
  int LocalReductionCounter = 0;
  int SuccessfulLocalReductionCounter = 0;
  int BestSampleId = -1;
  float BestSRScore = -1;
  float BestARScore = -1;
  float BestRScore = -1;
  float BestGScore = -1;
  float BestOScore = -1;

  llvm::TimeRecord ChiselTimeRecord;
  llvm::TimeRecord LearningTimeRecord;
  llvm::TimeRecord OracleTimeRecord;

  llvm::Timer ChiselTimer;
  llvm::Timer LearningTimer;
  llvm::Timer OracleTimer;
};

#endif // INSTRU_PROFILER_H
