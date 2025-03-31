#include <iostream>
#include <fstream>
#include <vector>
#include <string>  
#include <sstream>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#include <Windows.h>
#include <cstring>
#include "ScreenShot.h"

using namespace Microsoft::WRL;
using namespace std;


std::vector<std::wstring> packageArgs(int argc, wchar_t** argv)
{
    std::vector<std::wstring> argVector;
    for (auto i = 0; i < argc; ++i)
    {
        argVector.push_back(std::wstring(argv[i]));
    }
    return argVector;
}

void SavePng(std::wstring fileName, int width, int height, std::vector<unsigned char>& image)
{
    ComPtr<IWICImagingFactory> pFactory = NULL;
    ComPtr<IWICBitmapEncoder> piEncoder = NULL;
    ComPtr<IWICBitmapFrameEncode> piBitmapFrame = NULL;
    ComPtr<IPropertyBag2> pPropertybag = NULL;
    ComPtr< IWICStream> piStream;

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFactory)
    );

    TOF(hr = pFactory->CreateStream(&piStream));    
    TOF(hr = piStream->InitializeFromFilename(fileName.c_str(), GENERIC_WRITE));
    TOF(hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &piEncoder));
    TOF(hr = piEncoder->Initialize(piStream.Get(), WICBitmapEncoderNoCache));

    TOF(hr = piEncoder->CreateNewFrame(&piBitmapFrame, &pPropertybag));
    if (SUCCEEDED(hr))
    {
        //encoder options should be set here. 
        hr = piBitmapFrame->Initialize(pPropertybag.Get());
    }
    piBitmapFrame->SetSize(width, height);
    WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;
    TOF(hr = piBitmapFrame->SetPixelFormat(&formatGUID));
    hr = IsEqualGUID(formatGUID, GUID_WICPixelFormat32bppBGRA) ? S_OK : E_FAIL;
    if (hr == E_FAIL)
    {
        //Well, this is emberassing. We are not prepared to handle the 
        //case of getting back a pixel format that we didn't expect. 
        //There's no recovering from this. Let's just exit. 
        throw ArgumentOutOfRangeException(__FILEW__, __LINE__, L"Unexpected pixel format returned");
    }

    UINT cbStride = (width * 32 + 7) / 8/***WICGetStride***/;
    //UINT cbBufferSize = height * cbStride;

    TOF(hr = piBitmapFrame->WritePixels(height, cbStride, static_cast<DWORD>(image.size()), image.data()));
    TOF(hr = piBitmapFrame->Commit());
    TOF(hr = piEncoder->Commit());
}

void RedirectIOToConsole() {
    //from https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
    //Create a console for this application
    AllocConsole();

    // Get STDOUT handle
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
    FILE* COutputHandle = _fdopen(SystemOutput, "w");

    // Get STDERR handle
    HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
    FILE* CErrorHandle = _fdopen(SystemError, "w");

    // Get STDIN handle
    HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
    FILE* CInputHandle = _fdopen(SystemInput, "r");

    //make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
    ios::sync_with_stdio(true);

    // Redirect the CRT standard input, output, and error handles to the console
    freopen_s(&CInputHandle, "CONIN$", "r", stdin);
    freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
    freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

    //Clear the error state for each of the C++ standard stream objects. We need to do this, as
    //attempts to access the standard streams before they refer to a valid target will cause the
    //iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
    //to always occur during startup regardless of whether anything has been read from or written to
    //the console or not.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
}

std::vector<BYTE> GetResourceBytes(int id)
{
    
    HMODULE hModule = nullptr;// ::GetModuleHandle(NULL);
    HRSRC hResource = FindResourceW(hModule, MAKEINTRESOURCE(id), RT_RCDATA);
    auto err = GetLastError();
    HGLOBAL hResourceBytes = LoadResource(hModule, hResource);
    auto size = SizeofResource(hModule, hResource);
    std::vector<unsigned char> retVal(size);
    LPVOID resourceData = LockResource(hResource);
    std::memcpy(retVal.data(), resourceData, retVal.size());
    
    UnlockResource(hResource);
    return retVal;
}

const wchar_t BOM = 0xFEFF;

std::vector<WORD> GetResourceWords(int id)
{
    HMODULE hModule = nullptr;// ::GetModuleHandle(NULL);
    std::vector<WORD> retVal;
    HRSRC hResource = FindResourceW(hModule, MAKEINTRESOURCE(id), RT_RCDATA);
    if (hResource)
    {
        auto err = GetLastError();
        HGLOBAL hResourceBytes = LoadResource(hModule, hResource);
        if (hResourceBytes != NULL)
        {
            auto size = SizeofResource(hModule, hResource);
            retVal.resize((size + 1) / 2);
            LPVOID resourceData = LockResource(hResource);
            WORD* data = new WORD[size];
            memcpy(data, resourceData, size);

            std::memcpy(retVal.data(), resourceData, size);
            if ((!retVal.empty()) && retVal[0] == BOM)
            {
                retVal.erase(retVal.begin());
            }
            UnlockResource(hResource);
        }
    }
    return retVal;
}

std::wstring GetResourceDocumentW(int id)
{
    std::vector<WORD> wordList = GetResourceWords(id);
    auto retVal = std::wstring(wordList.begin(), wordList.end());
    return retVal;
}

void ShowHelp()
{
    auto helpString = GetResourceDocumentW(IDS_HELPSTRING);
    auto ansiString = GetResourceDocumentW(IDS_LOGOSTRING);
    std::vector<WCHAR> windowTitle(1024);
    if (!GetConsoleTitle(windowTitle.data(), static_cast<WORD>(windowTitle.size())) && GetLastError() == ERROR_SUCCESS) {
        //launched in the console.
        std::wcout << "In console";
        std::wcout << ansiString << L"\r\n\r\n" << helpString << L"\r\n\r\n";
    }
    else
    {
        FreeConsole();
        if (AllocConsole())
        {
            HANDLE hConsole = GetConsoleWindow();
            DWORD dConsoleMode;

            
            auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            auto hIn = GetStdHandle(STD_INPUT_HANDLE);
/*
            auto hRealOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,0, CREATE_NEW, 0, 0);
            auto hRealIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, 0, 0);

            if (hRealOut != hOut)
            {
                SetStdHandle(STD_OUTPUT_HANDLE, hRealOut);
                hOut = hRealOut;
            }
            if (hRealIn != hIn)
            {
                SetStdHandle(STD_INPUT_HANDLE, hRealIn);
                hIn = hRealIn;
            }
        */

            if (GetConsoleMode(hOut, &dConsoleMode))
            {
                dConsoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING| DISABLE_NEWLINE_AUTO_RETURN;
                SetConsoleMode(hOut, dConsoleMode);
            }
         
            DWORD bytesWritten;

            WriteConsole(hOut, helpString.c_str(), helpString.size(), &bytesWritten, NULL);
            WriteConsole(hOut, L" \r\n\r\n", 2, &bytesWritten, NULL);
            WriteConsole(hOut, ansiString.c_str(), ansiString.size(), &bytesWritten, NULL);
            //WriteFile(hOut, ansiString.c_str(), ansiString.size(), &bytesWritten, FALSE);
            Sleep(15000);
        }
    }
}

void SaveBitmap(std::wstring fileName, int width, int height, std::vector<BYTE>& image)
{
    BITMAPFILEHEADER bmfHdr = { 0 };
    BITMAPINFOHEADER bi = { 0 };
    LPBITMAPINFOHEADER lpbi = NULL;
    HANDLE fh, hDib = NULL, hOldPal2 = NULL;
    DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrImportant = 0;
    bi.biClrUsed = 256;

    dwBmBitsSize = ((width * 4)) * height;    
    std::vector<BYTE> BI_HEADER(sizeof(BITMAPINFOHEADER));
    lpbi = reinterpret_cast<LPBITMAPINFOHEADER>( BI_HEADER.data());
    *lpbi = bi;

    fh = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (fh == INVALID_HANDLE_VALUE)
        return;

    bmfHdr.bfType = 0x4D42; // "BM"
    dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
    bmfHdr.bfSize = dwDIBSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(fh, (LPSTR)lpbi, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    WriteFile(fh, (LPSTR)image.data(), static_cast<DWORD>(image.size()), &dwWritten, NULL);
    CloseHandle(fh);
}


void CaptureScreen(std::wstring fileName)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    int width = GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, 96);
    int height = GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, 96);
    //int height = (int)GetSystemMetrics(SM_CYVIRTUALSCREEN); // get the width and height of the screen
    //int width = GetSystemMetrics(SM_CXVIRTUALSCREEN); // only capture left monitor for dual screen setups, for both screens use (int)GetSystemMetrics(SM_CXVIRTUALSCREEN);
    
    std::vector<BYTE> image(width * height * 4);
    {

        HDC hdc = GetDC(NULL); // get the desktop device context
        
        HDC cdc = CreateCompatibleDC(hdc); // create a device context to use yourself
        int dWidth = GetDeviceCaps(cdc, PHYSICALWIDTH);
        HBITMAP hbitmap = CreateCompatibleBitmap(hdc, width, height); // create a bitmap
        SelectObject(cdc, hbitmap); // use the previously created device context with the bitmap
        BITMAPINFOHEADER bmi = { 0 };
        bmi.biSize = sizeof(BITMAPINFOHEADER);
        bmi.biPlanes = 1;
        bmi.biBitCount = 32;
        bmi.biWidth = width;
        bmi.biHeight = -height; // flip image upright
        bmi.biCompression = BI_RGB;
        bmi.biSizeImage = 4 * width * height;
        BitBlt(cdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY); // copy from desktop device context to bitmap device context
        ReleaseDC(NULL, hdc);
        int scanLines = GetDIBits(cdc, hbitmap, 0, height, image.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
        DeleteObject(hbitmap);
        DeleteDC(cdc);
    }
    //image should now have the RGBA bytes for the image. 
    SaveBitmap(fileName, width, height, image);
    SavePng(fileName + L".png", width, height, image);
}

int WinMain(HINSTANCE  hInstance, HINSTANCE  hPrevInstance, LPSTR args, int nCmdShow)
{
    HRESULT hr = CoInitialize(NULL);

    int cmdCount;
    //auto wargs = reinterpret_cast<LPWSTR>(&args);
    auto argv = CommandLineToArgvW(GetCommandLine(), &cmdCount);
    std::vector<std::wstring> argList;
    for (auto i = 1; i < cmdCount; ++i)
    {
        argList.push_back(argv[i]);
    }

    auto options = ProcessOptions(argList);
    if (options.ShowHelp)
    {
        ShowHelp();
    }
    else
    {
        try {
            CaptureScreen(options.DestinationFile);
        }
        catch (COMException exc)
        {
            exc.WriteToLog();
        }

        CoUninitialize();
        return 0;
    }
}