#include <string>
#include <iostream>
#include <optional>
#include <utility>

#include "../lib/CLI11.hpp"


int main(int argc, char *argv[]) {
  CLI::App app("This is a test", "test");

  std::uint64_t gaga = 1000;
  app.add_option("--gaga", gaga, "A gaga option")->envname("GAGA")->transform(CLI::AsSizeValue(1));
  
  std::string gugus = "Hallo gugus";
  app.add_option("--gugus", gugus, "A gugus string")->envname("GUGUS");

  app.parse(argc, argv);

  if (app.get_option("--gaga")->empty()) {
    std::cerr << "gaga is empty, default=" << gaga << std::endl;
  }
  else {
    std::cerr << "gaga is set, value=" << gaga  << std::endl;
  }

  if (app.get_option("--gugus")->empty()) {
    std::cerr << "gugus is empty, default=" << gugus << std::endl;
  }
  else {
    std::cerr << "gugus is set, value=" << gugus << std::endl;
  }
  return 0;
}
    
