#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdlib.h>
#include <time.h>

#include "InstruFileManager.h"
#include "InstruFrontend.h"
#include "InstruIntegrationManager.h"
#include "InstruOptionManager.h"
#include "InstruProfiler.h"
#include "InstruReformat.h"
#include "InstruReport.h"
#include "InstruStatsManager.h"
#include "StmtInstrumentation.h"
#include "BlockInstrumentation.h"

void initialize() {
  InstruFileManager::Initialize();

  auto ConsolSink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      InstruOptionManager::OutputDir + "/log.txt", true);
  //auto Logger = std::make_shared<spdlog::logger>(
  //    "Logger", spdlog::sinks_init_list{ConsolSink, FileSink});
  auto Logger = std::make_shared<spdlog::logger>(
	"Logger", spdlog::sinks_init_list{ConsolSink});
  
  ConsolSink->set_pattern("%v");
  if (InstruOptionManager::Debug) {
    ConsolSink->set_level(spdlog::level::debug);
    FileSink->set_level(spdlog::level::debug);
    Logger->set_level(spdlog::level::debug);
  } else {
    ConsolSink->set_level(spdlog::level::info);
    FileSink->set_level(spdlog::level::info);
    Logger->set_level(spdlog::level::info);
  }
  spdlog::register_logger(Logger);

  InstruIntegrationManager::Initialize();
  InstruProfiler::Initialize();
  for (auto &File : InstruOptionManager::InputFiles)
    spdlog::get("Logger")->info("Input: {}", File);
  //spdlog::get("Logger")->info("Log Output Directory: {}", InstruOptionManager::OutputDir);
}

void finalize() {
  InstruIntegrationManager::Finalize();
  InstruFileManager::Finalize();
  InstruProfiler::Finalize();
}

int instruOneFileByStmt(std::string &File) {
  //spdlog::get("Logger")->info("Insrument File: {}", File);
  InstruOptionManager::InputFile = File;

  InstruStatsManager::ComputeStats(InstruOptionManager::InputFile);

  //spdlog::get("Logger")->info("Start instrumentation");
  StmtInstrumentation *SI = new StmtInstrumentation();
  InstruFrontend::Parse(InstruOptionManager::InputFile, SI);

  //spdlog::get("Logger")->info("End instrumentation");

  return 0;
}

int instruOneFileByBlock(std::string &File) {
  //spdlog::get("Logger")->info("Insrument File: {}", File);
  InstruOptionManager::InputFile = File;

  InstruStatsManager::ComputeStats(InstruOptionManager::InputFile);

  //spdlog::get("Logger")->info("Start instrumentation");
  BlockInstrumentation *BI = new BlockInstrumentation();
  InstruFrontend::Parse(InstruOptionManager::InputFile, BI);

  //spdlog::get("Logger")->info("End instrumentation");

  return 0;
}


int main(int argc, char *argv[]) {
  srand(time(0)); //Set random seed as time
  
  InstruOptionManager::handleOptions(argc, argv);
  initialize();

  InstruProfiler::GetInstance()->beginChisel();

  InstruStatsManager::ComputeStats(InstruOptionManager::InputFiles);
  int wc0 = std::numeric_limits<int>::max();
  int wc = InstruStatsManager::GetNumOfWords();

  if (InstruOptionManager::Stat) {
    InstruStatsManager::Print();
    return 0;
  }

  std::string InstruGranu = InstruOptionManager::Granu;
  
  for (auto &File : InstruOptionManager::InputFiles) { //Reduce each file
    if (InstruGranu == "statement") { instruOneFileByStmt(File); }
    else if (InstruGranu == "block") { instruOneFileByBlock(File); }
    else { spdlog::get("Logger")->info("Unknown Instrumentation Granuality: {}" , InstruGranu); }
  }

  InstruProfiler::GetInstance()->endChisel();

  //InstruReport::print();
  finalize();

  return 0;
}
