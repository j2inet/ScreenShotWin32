#pragma once
#include "ScreenShotOptions.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>




ScreenShotOptions defaultScreenshotOptions
{
	FILE_TYPE_PNG,	
	L"ScreenShot.bmp",
	L"",
	L"",
	false,
	false,
};


std::wstring FILE_TYPE_ARG_PNG = L"png";
std::wstring FILE_TYPE_ARG_BMP = L"bmp";
std::wstring FILE_TYPE_ARG_JPG = L"jpg";


ScreenShotOptions LoadDefaultOptions()
{
	ScreenShotOptions options{	};
	std::wifstream argsFile("ScreenShot.args");
	std::vector<std::wstring> lines;
	if (argsFile.is_open())
	{
		for (std::wstring line; std::getline(argsFile, line);)
		{
			lines.push_back(line);
		}
		options = ParseLines(lines);
	}
	return options;
}

const std::wstring  FILE_TYPE_ARGUMENT = L"--filetype";

ScreenShotOptions ParseLines(std::vector<std::wstring> args, ScreenShotOptions defaults)
{
	ScreenShotOptions options
	{ 
		FILE_TYPE_PNG,
		L"ScreenShot.bmp",
		L"",
		L"",
		false,
		false
	};

	for (auto current : args)
	{
		if (current.find(L"--") == 0)
		{
			if (current == L"--help") {
				options.ShowHelp = true;
			}
			else if (current.size() > 9 && current.substr(0, 9) == L"--bucket:")
			{
				options.Bucket = current.substr(9);
			}
			else if (current.size() > 6 && current.substr(0, 6) == L"--key:")
			{
				options.Key = current.substr(6);
			}
			else if (current.size() > 11 && current.substr(0, 10) == L"--filename:")
			{
				options.DestinationFile = current.substr(11);
			}
			else if(current.size() > FILE_TYPE_ARGUMENT.size() && current.substr(0, FILE_TYPE_ARGUMENT.size()) == FILE_TYPE_ARGUMENT )
			{
				std::wstring fileType = current.substr(FILE_TYPE_ARGUMENT.size());
				std::transform(current.begin(), current.end(), current.begin(), [](unsigned char c) { return std::tolower(c); });
				if (fileType == FILE_TYPE_ARG_PNG)
				{
					options.FileType = FILE_TYPE_PNG;
				}
				else if (fileType == FILE_TYPE_ARG_BMP)
				{
					options.FileType = FILE_TYPE_BMP;
				}
				else if (fileType == FILE_TYPE_ARG_JPG)
				{
					options.FileType = FILE_TYPE_JPG;
				}
			}
			else {
				options.HasUnrecognizedOption = true;
			}
		}
		else
		{
			options.DestinationFile = current;
		}
	}
	return options;
}


ScreenShotOptions ProcessOptions(std::vector<std::wstring> args)
{
	ScreenShotOptions options = defaultScreenshotOptions;
	options = ParseLines(args, defaultScreenshotOptions);
	return options;
}