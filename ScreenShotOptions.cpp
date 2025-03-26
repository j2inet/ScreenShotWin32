#pragma once
#include "ScreenShotOptions.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>

ScreenShotOptions ProcessOptions(std::vector<std::wstring> args)
{
	ScreenShotOptions options{ false,false, L"screenshot.bmp" };
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
			else
			{
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