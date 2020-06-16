#include "InstruOptionManager.h"

#include <getopt.h>
#include <string>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include "InstruFileManager.h"

const std::string usage_simple("Usage: debop [OPTIONS]... ORACLE PROGRAM");
const std::string error_message("Try 'debop --help' for more information.");

void InstruOptionManager::showUsage() {
  llvm::errs()
      << usage_simple << "\n"
      << "Options:"
      << "\n"
      << "  --help                 Show this help message\n"
      << "  --output_dir OUTDIR    Output directory\n"
      << "  --build                Integrate Debop with build system\n"
      << "  --save_temp            Save intermediate results\n"
      << "  --skip_learning        Disable decision tree learning\n"
      << "  --skip_delay_learning  Learn a new model for every iteration\n"
      << "  --no_cache             Do not cache intermediate results\n"
      << "  --skip_local_dep       Disable local dependency checking\n"
      << "  --skip_global_dep      Disable global dependency checking\n"
      << "  --skip_dce             Do not perform static unreachability "
         "analysis\n"
      << "  --no_profile           Do not print profiling report\n"
      << "  --debug                Print debug information\n"
      << "  --stat                 Count the number of statements\n"
      << "  --max_samples MAX_SAMPLES   The max number of samples generated\n"
      << "  --max_iters MAX_ITERS   The max number of sampling iterations\n"
      << "  --alpha ALPHA   The weight (b/w 0 and 1 inclusive) of AR-Score (attack surface reduction score). The weight of SR-Score (size reduction score) is (1-ALPHA). R-Score is ((1-ALPHA)*SR-Score + ALPHA*AR-Score)\n"
      << "  --beta BETA   The weight (b/w 0 and 1 inclusive) of G-Score. The weight of R-Score is (1-BETA). O-Score is ((1-BETA)*R-Score + BETA*G-Score)\n"
      << "  --kvalue KVALUE The k-value for computing density value\n"
      << "  --granularity GRANU Instrumentation Granularity\n";
}

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"output_dir", required_argument, 0, 't'},
    {"build", no_argument, 0, 'b'},
    {"save_temp", no_argument, 0, 's'},
    {"skip_learning", no_argument, 0, 'D'},
    {"skip_delay_learning", no_argument, 0, 'd'},
    {"no_cache", no_argument, 0, 'c'},
    {"skip_local_dep", no_argument, 0, 'L'},
    {"skip_global_dep", no_argument, 0, 'G'},
    {"skip_dce", no_argument, 0, 'C'},
    {"no_profile", no_argument, 0, 'p'},
    {"debug", no_argument, 0, 'v'},
    {"stat", no_argument, 0, 'S'},
    {"max_samples", required_argument, 0, 'm'},
    {"max_iters", required_argument, 0, 'i'},
    {"alpha", required_argument, 0, 'a'},
    {"beta", required_argument, 0, 'e'},
    {"kvalue", required_argument, 0, 'k'},
    {"granularity", required_argument, 0, 'g'},
    {0, 0, 0, 0}};

static const char *optstring = "ho:t:sDdcLGCpvSm:i:a:e:k:g:";

std::string InstruOptionManager::BinFile = "";
std::vector<std::string> InstruOptionManager::InputFiles;
std::vector<std::string> InstruOptionManager::BuildCmd;
std::string InstruOptionManager::InputFile = "";
std::string InstruOptionManager::OracleFile = "";
std::string InstruOptionManager::EvalResultFile = "eval_rslt.txt";
std::string InstruOptionManager::OutputDir = "debop-out";
bool InstruOptionManager::Build = false;
bool InstruOptionManager::SaveTemp = false;
bool InstruOptionManager::SaveSample = false;
bool InstruOptionManager::SkipLearning = false;
bool InstruOptionManager::SkipDelayLearning = false;
bool InstruOptionManager::NoCache = false;
bool InstruOptionManager::SkipGlobalDep = false;
bool InstruOptionManager::SkipLocalDep = false;
bool InstruOptionManager::SkipDCE = false;
bool InstruOptionManager::Profile = true;
bool InstruOptionManager::Debug = false;
bool InstruOptionManager::Stat = false;
int InstruOptionManager::MaxSamples = 5;
int InstruOptionManager::MaxIters = 100;
float InstruOptionManager::Alpha = 0.5;
float InstruOptionManager::Beta = 0.5;
float InstruOptionManager::K = 1;
std::string InstruOptionManager::Granu = "statement";

void InstruOptionManager::handleOptions(int argc, char *argv[]) {
  llvm::ErrorOr<std::string> Program = llvm::sys::findProgramByName(argv[0]);
  BinFile = InstruFileManager::Readlink(Program.get());

  char c;
  while ((c = getopt_long(argc, argv, optstring, long_options, 0)) != -1) {
    switch (c) {
    case 'h':
      showUsage();
      exit(0);

    case 't':
      InstruOptionManager::OutputDir = std::string(optarg);
      break;

    case 'b':
      Build = true;
      break;

    case 's':
      InstruOptionManager::SaveTemp = true;
      InstruOptionManager::SaveSample = true;
      break;

    case 'D':
      InstruOptionManager::SkipLearning = true;
      break;

    case 'd':
      InstruOptionManager::SkipDelayLearning = true;
      break;

    case 'c':
      InstruOptionManager::NoCache = true;
      break;

    case 'L':
      InstruOptionManager::SkipLocalDep = true;
      break;

    case 'G':
      InstruOptionManager::SkipGlobalDep = true;
      break;

    case 'C':
      InstruOptionManager::SkipDCE = true;
      break;

    case 'p':
      InstruOptionManager::Profile = false;
      break;

    case 'v':
      InstruOptionManager::Debug = true;
      break;

    case 'S':
      InstruOptionManager::Stat = true;
      break;

    case 'm':
      InstruOptionManager::MaxSamples = std::stoi(optarg);
      break;

    case 'i':
      InstruOptionManager::MaxIters = std::stoi(optarg);
      break;

    case 'a':
      InstruOptionManager::Alpha = std::stof(optarg);
      break;
      
    case 'e':
      InstruOptionManager::Beta = std::stof(optarg);
      break;

    case 'k':
      InstruOptionManager::K = std::stof(optarg);
      break;

    case 'g':
      InstruOptionManager::Granu = std::string(optarg);
      break;
      
    default:
      llvm::errs() << "Invalid option.\n";
      llvm::errs() << usage_simple << "\n";
      llvm::errs() << error_message << "\n";
      exit(1);
    }
  }

  if (optind + 2 > argc && !InstruOptionManager::Stat) {
    llvm::errs() << "debop: You must specify the oracle and input.\n";
    llvm::errs() << usage_simple << "\n";
    llvm::errs() << error_message << "\n";
    exit(1);
  } else if (optind + 1 > argc && InstruOptionManager::Stat) {
    llvm::errs() << "debop: You must specify the input.\n";
    exit(1);
  }

  if (optind + 1 > argc && InstruOptionManager::Stat) {
    llvm::errs() << "debop: You must specify the input.\n";
    exit(1);
  }

  
  int i;
  if (Stat)
    i = optind;
  else
    i = optind + 1;

  if (Build) {
    // integrate Debop with build system
    // input files will be captured by IntegrationManager
    for (; i < argc; i++)
      BuildCmd.push_back(std::string(argv[i]));
  } else {
    for (; i < argc; i++) {
      std::string Input = std::string(argv[i]);
      if (!llvm::sys::fs::exists(Input)) {
        llvm::errs() << "The specified input file " << Input
                     << " does not exist.\n";
        exit(1);
      }
      InputFiles.push_back(Input);
    }
  }

  // When Debop only computes statistics, it does not need to check the oracle
  if (InstruOptionManager::Stat)
    return;

  /*
  InstruOptionManager::OracleFile = std::string(argv[optind]);

  if (!llvm::sys::fs::exists(InstruOptionManager::OracleFile)) {
    llvm::errs() << "The specified oracle file " << InstruOptionManager::OracleFile
                 << " does not exist.\n";
    exit(1);
  } else if (!llvm::sys::fs::can_execute(InstruOptionManager::OracleFile)) {
    llvm::errs() << "The specified oracle file " << InstruOptionManager::OracleFile
                 << " is not executable.\n";
    exit(1);
  } else if (llvm::sys::ExecuteAndWait(InstruOptionManager::OracleFile,
                                       {InstruOptionManager::OracleFile})) {
    llvm::errs() << "The specified oracle file " << InstruOptionManager::OracleFile
                 << " cannot execute correctly.\n";
    exit(1);
  }
  */

  llvm::SmallString<128> OutputDirVec(OutputDir);
  llvm::sys::fs::make_absolute(OutputDirVec);
  OutputDir = OutputDirVec.str();
}
