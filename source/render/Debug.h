#pragma once

#include <string>

bool hasGLError(std::string &errorText);

void printGLError(const char* file, int line);

#define PRINT_GL_ERROR() printGLError(__FILE__, __LINE__);