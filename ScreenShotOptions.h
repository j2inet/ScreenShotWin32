#pragma once
#pragma once

#include <string>
#include <vector>
#include "resource.h"


enum FILE_TYPES {
	FILE_TYPE_BMP, FILE_TYPE_PNG, FILE_TYPE_JPG
};

extern std::wstring FILE_TYPE_ARG_PNG;
extern std::wstring FILE_TYPE_ARG_BMP;
extern std::wstring FILE_TYPE_ARG_JPG;

struct ScreenShotOptions
{	
	FILE_TYPES FileType;
	std::wstring DestinationFile;
	std::wstring Bucket;
	std::wstring Key;
	bool ShowHelp;
	bool HasUnrecognizedOption;

};

extern const std::wstring  FILE_TYPE_ARGUMENT;

extern ScreenShotOptions defaultScreenshotOptions;
ScreenShotOptions LoadDefaultOptions();
ScreenShotOptions ParseLines(std::vector<std::wstring> args, ScreenShotOptions options = defaultScreenshotOptions);
ScreenShotOptions ProcessOptions(std::vector<std::wstring> args);