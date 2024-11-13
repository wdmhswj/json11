// #include "my_json11.h"
#include "TutorialConfig.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cout << argv[0] << " Version " << sub_project_VERSION_MAJOR << "."
              << sub_project_VERSION_MINOR << std::endl;
    std::cout << "Usage: " << argv[0] << " number" << std::endl;
    return 1;
  }

  // convert input to double
  // TODO 4: Replace atof(argv[1]) with std::stod(argv[1])
  const double inputValue = std::stod(argv[1]);


  return 0;
}
