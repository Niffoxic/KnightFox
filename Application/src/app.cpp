#include <windows.h>

#include "game/game.h"

#include <engine/system/exception/base_exception.h>

int CALLBACK WinMain(
    _In_ HINSTANCE     hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR         lpCmdLine,
    _In_ int           nCmdShow)
{
    try
    {
        kfe::KFE_ENGINE_CREATE_DESC desc{};
        desc.WindowsDesc.Width  = 800u;
        desc.WindowsDesc.Height = 600u;

        GameApplication application{ desc };

        if (!application.Init()) return E_FAIL;
        return application.Execute();
    }
    catch (const kfe::BaseException& ex)
    {
        MessageBoxA(nullptr, ex.what(), "PixelFox Exception", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }
    catch (const std::exception& ex)
    {
        MessageBoxA(nullptr, ex.what(), "Standard Exception", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        MessageBoxA(nullptr, "Unknown fatal error occurred.", "PixelFox Crash", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }

    return S_OK;
}
