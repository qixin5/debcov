#include "InstruReport.h"

#include <spdlog/spdlog.h>

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include "InstruFileManager.h"
#include "InstruIntegrationManager.h"
#include "InstruOptionManager.h"
#include "InstruProfiler.h"
#include "InstruStatsManager.h"

void InstruReport::print() {
  InstruProfiler *Prof = InstruProfiler::GetInstance();

  int TotalWidth = 52;
  int LeftColWidth = 25;
  const char *RightColFmt = "%21.1f";
  std::string Bar(TotalWidth, '=');

  spdlog::get("Logger")->info(Bar);
  spdlog::get("Logger")->info("{:^52}", "Report");
  spdlog::get("Logger")->info(Bar);
  spdlog::get("Logger")->info("{:>25}{:>21.1f}s", "Total Time :",
                              Prof->getChiselTimeRecord().getWallTime());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample Id :",
                              Prof->getBestSampleId());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample SR-Score :",
                              Prof->getBestSizeRScore());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample AR-Score :",
                              Prof->getBestAttackSurfaceRScore());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample R-Score :",
                              Prof->getBestRScore());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample G-Score :",
                              Prof->getBestGScore());
  spdlog::get("Logger")->info("{:>25}{:>22}", "Best Sample O-Score :",
                              Prof->getBestOScore());
}
