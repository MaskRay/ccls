#pragma once

#include <string>
#include <vector>

std::vector<std::string> GetFilesInFolder(std::string folder);
std::vector<std::string> ReadLines(std::string filename);
void ParseTestExpectation(std::string filename, std::string* expected_output);

void Fail(const std::string& message);