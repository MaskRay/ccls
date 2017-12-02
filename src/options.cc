#include "options.h"

#include <loguru.hpp>

#include <iostream>

std::unordered_map<std::string, std::string> ParseOptions(int argc,
                                                          char** argv) {
  std::unordered_map<std::string, std::string> output;

  std::string previous_arg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg[0] != '-') {
      if (previous_arg.size() == 0) {
        LOG_S(FATAL) << "Invalid arguments; switches must start with -";
        exit(1);
      }

      output[previous_arg] = arg;
      previous_arg = "";
    } else {
      output[arg] = "";
      previous_arg = arg;
    }
  }

  return output;
}

bool HasOption(const std::unordered_map<std::string, std::string>& options,
               const std::string& option) {
  return options.find(option) != options.end();
}