#include "versionUtil.h"

#include "version.h"

#pragma message("Updated binary version: " VERSION)

std::string version() { return VERSION; }
