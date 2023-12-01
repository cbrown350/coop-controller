#include "CoopLogger.h"
#include <vector>
#include <memory>
#include <ELogCoopLogger.h>

std::unique_ptr<std::vector<std::unique_ptr<CoopLogger>>> CoopLogger::loggers = nullptr;
Print *CoopLogger::defaultPrintStream = &Serial; 