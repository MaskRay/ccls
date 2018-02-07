#pragma once

#include <string_view.h>

#include <string>

void EnableRecording(std::string path);
void RecordInput(std::string_view content);
void RecordOutput(std::string_view content);