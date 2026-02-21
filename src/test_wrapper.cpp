#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Minimal BlitzCC running!" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << "arg[" << i << "] = " << argv[i] << std::endl;
  }
  return 0;
}
