#pragma once
#pragma once

#include <string>
#include <vector>
#include "resource.h"

struct ScreenShotOptions
{
	bool ShowHelp;
	bool HasUnrecognizedOption;
	std::wstring DestinationFile;
	std::wstring Bucket;
	std::wstring Key;
};
ScreenShotOptions ProcessOptions(std::vector<std::wstring> args);