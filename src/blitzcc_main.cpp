#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void usage() {
  std::cout << "BltzNxt BlitzCC Wrapper v0.6.0" << std::endl;
  std::cout << "Usage: blitzcc [options] <source_file>" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -q            Quiet mode" << std::endl;
  std::cout << "  -d            Debug mode (Compile and Run)" << std::endl;
  std::cout << "  -c            Syntax check only" << std::endl;
  std::cout << "  -k            List keywords" << std::endl;
  std::cout << "  -o <file>     Output executable" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage();
    return 1;
  }

  std::string sourceFile;
  std::string outputFile;
  bool debug = false;
  bool syntaxCheck = false;
  bool listKeywords = false;
  bool quiet = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-d")
      debug = true;
    else if (arg == "-c")
      syntaxCheck = true;
    else if (arg == "-k")
      listKeywords = true;
    else if (arg == "-q" || arg == "+q")
      quiet = true;
    else if (arg == "-o") {
      if (i + 1 < argc)
        outputFile = argv[++i];
    } else {
      sourceFile = arg;
    }
  }

  fs::path exePath = fs::absolute(fs::path(argv[0]));
  fs::path binPath = exePath.parent_path();
  fs::path sdkPath = binPath.parent_path();
  fs::path bbcPath = binPath / "bbc_cpp.exe";

  // Fallback for dev environment
  if (!fs::exists(bbcPath)) {
    bbcPath = binPath / "../../_build/bbc_cpp.exe";
    if (!fs::exists(bbcPath)) {
      bbcPath = "bbc_cpp.exe";
    }
  }

  if (listKeywords) {
    std::stringstream cmd;
    cmd << "\"\"" << bbcPath.string() << "\" --keywords\"";
    return std::system(cmd.str().c_str());
  }

  if (sourceFile.empty()) {
    std::cerr << "Error: No source file specified." << std::endl;
    return 1;
  }

  // 1. Transpile
  std::string tempCpp = "_build/temp_ide.cpp";
  fs::create_directories("_build");

  std::stringstream transpileCmd;
  transpileCmd << "\"\"" << bbcPath.string() << "\" \"" << sourceFile << "\" > "
               << tempCpp << "\"";

  if (!quiet)
    std::cout << "Transpiling: " << sourceFile << "..." << std::endl;

  int res = std::system(transpileCmd.str().c_str());
  if (res != 0)
    return res;

  if (syntaxCheck)
    return 0;

  // 2. Compile
  if (outputFile.empty()) {
    outputFile = fs::path(sourceFile).replace_extension(".exe").string();
  }

  std::stringstream compileCmd;
  fs::path gppPath = sdkPath / "tools/mingw64/bin/g++.exe";
  if (!fs::exists(gppPath))
    gppPath = "g++"; // Fallback to PATH

  compileCmd << "\"\"" << gppPath.string() << "\" -O2 " << tempCpp;
  compileCmd << " -I\"" << (sdkPath / "src/runtime").string() << "\"";
  compileCmd << " -I\""
             << (sdkPath / "libs/SDL3/x86_64-w64-mingw32/include").string()
             << "\"";
  compileCmd << " -L\"" << (sdkPath / "lib").string() << "\" -lbbruntime";
  compileCmd << " -L\""
             << (sdkPath / "libs/SDL3/x86_64-w64-mingw32/lib").string()
             << "\" -lSDL3 -lopengl32 -lglu32 -lole32 -luuid -lshell32";
  compileCmd << " -o \"" << outputFile << "\"\"";

  if (!quiet)
    std::cout << "Compiling: " << outputFile << "..." << std::endl;

  res = std::system(compileCmd.str().c_str());
  if (res != 0)
    return res;

  // 3. Run if requested
  if (debug) {
    if (!quiet)
      std::cout << "Running: " << outputFile << "..." << std::endl;
    std::string runCmd = "\"" + outputFile + "\"";
    return std::system(runCmd.c_str());
  }

  return 0;
}
