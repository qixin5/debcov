#include "InstruProfiler.h"

#include "llvm/Support/Timer.h"

InstruProfiler *InstruProfiler::Instance;

void InstruProfiler::Initialize() {
  Instance = new InstruProfiler();

  Instance->ChiselTimer.init("ChiselTimer", "");
  Instance->OracleTimer.init("OracleTimer", "");
  Instance->LearningTimer.init("LearningTimer", "");
}

InstruProfiler *InstruProfiler::GetInstance() {
  if (!Instance)
    Initialize();
  return Instance;
}

void InstruProfiler::Finalize() { delete Instance; }

void InstruProfiler::incrementGlobalReductionCounter() { GlobalReductionCounter++; }

void InstruProfiler::incrementSuccessfulGlobalReductionCounter() {
  SuccessfulGlobalReductionCounter++;
}

void InstruProfiler::incrementLocalReductionCounter() { LocalReductionCounter++; }

void InstruProfiler::incrementSuccessfulLocalReductionCounter() {
  SuccessfulLocalReductionCounter++;
}

void InstruProfiler::setBestSampleId(int Id) {
  BestSampleId = Id;
}

int InstruProfiler::getBestSampleId() {
  return BestSampleId;
}

void InstruProfiler::setBestSizeRScore(float SRScore) {
  BestSRScore = SRScore;
}

float InstruProfiler::getBestSizeRScore() {
  return BestSRScore;
}

void InstruProfiler::setBestAttackSurfaceRScore(float ARScore) {
  BestARScore = ARScore;
}

float InstruProfiler::getBestAttackSurfaceRScore() {
  return BestARScore;
}

void InstruProfiler::setBestRScore(float RScore) {
  BestRScore = RScore;
}

float InstruProfiler::getBestRScore() {
  return BestRScore;
}

void InstruProfiler::setBestGScore(float GScore) {
  BestGScore = GScore;
}

float InstruProfiler::getBestGScore() {
  return BestGScore;
}

void InstruProfiler::setBestOScore(float OScore) {
  BestOScore = OScore;
}

float InstruProfiler::getBestOScore() {
  return BestOScore;
}

void InstruProfiler::beginChisel() {
  assert(ChiselTimer.isInitialized());
  ChiselTimer.startTimer();
}

void InstruProfiler::endChisel() {
  assert(ChiselTimer.isRunning());
  ChiselTimer.stopTimer();
  ChiselTimeRecord += ChiselTimer.getTotalTime();
  ChiselTimer.clear();
}

void InstruProfiler::beginOracle() {
  assert(OracleTimer.isInitialized());
  OracleTimer.startTimer();
}

void InstruProfiler::endOracle() {
  assert(OracleTimer.isRunning());
  OracleTimer.stopTimer();
  OracleTimeRecord += OracleTimer.getTotalTime();
  OracleTimer.clear();
}

void InstruProfiler::beginLearning() {
  assert(LearningTimer.isInitialized());
  LearningTimer.startTimer();
}

void InstruProfiler::endLearning() {
  assert(LearningTimer.isRunning());
  LearningTimer.stopTimer();
  LearningTimeRecord += LearningTimer.getTotalTime();
  LearningTimer.clear();
}
