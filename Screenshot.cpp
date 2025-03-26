#include <iostream>
#include <vector>
#include <string>  
#include <sstream>
#include <stdlib.h>


#include <wrl/client.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#include <Windows.h>
#include "ScreenShotOptions.h"


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

    hr = pFactory->CreateStream(&piStream);
    hr = piStream->InitializeFromFilename(fileName.c_str(), GENERIC_WRITE);
    hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &piEncoder);
    hr = piEncoder->Initialize(piStream.Get(), WICBitmapEncoderNoCache);

    hr = piEncoder->CreateNewFrame(&piBitmapFrame, &pPropertybag);
    if (SUCCEEDED(hr))
    {
        //encoder options should be set here. 
        hr = piBitmapFrame->Initialize(pPropertybag.Get());
    }
    piBitmapFrame->SetSize(width, height);
    WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;
    hr = piBitmapFrame->SetPixelFormat(&formatGUID);
    hr = IsEqualGUID(formatGUID, GUID_WICPixelFormat32bppBGRA) ? S_OK : E_FAIL;

    UINT cbStride = (width * 32 + 7) / 8/***WICGetStride***/;
    UINT cbBufferSize = height * cbStride;

    hr = piBitmapFrame->WritePixels(height, cbStride, static_cast<DWORD>(image.size()), image.data());
    hr = piBitmapFrame->Commit();
    hr = piEncoder->Commit();

}

void ShowHelp()
{
    HMODULE hModule = nullptr;// ::GetModuleHandle(NULL);
    HRSRC hResource = FindResourceW(hModule, MAKEINTRESOURCE(IDS_HELPSTRING), RT_RCDATA);
    auto err = GetLastError();
    HGLOBAL hgString = LoadResource(hModule, hResource);
    LPCWSTR helpCString = (LPCWSTR)LockResource(hgString);
    std::wstring helpString = std::wstring((LPWSTR)helpCString);
    UnlockResource(hgString);
    std::wcout << helpCString;
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
        return 0;
    }

    CaptureScreen(options.DestinationFile);    
    return 0;
}