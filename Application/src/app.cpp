#include <windows.h>

#include <engine/knight_engine.h>
#include <engine/utils/logger/logger.h>

int CALLBACK WinMain(
    _In_ HINSTANCE     hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR         lpCmdLine,
    _In_ int           nCmdShow)
{
#ifdef _DEBUG
    KFE_LOGGER_CREATE_DESC logDesc{};
    logDesc.LogPrefix       = "SweetLog";
    logDesc.EnableTerminal  = true;
    logDesc.LogPath         = "logs";
    INIT_GLOBAL_LOGGER(&logDesc);
#endif

    kfe::KFE_WINDOW_CREATE_DESC desc{};
    desc.Height = 500u;
    desc.Width  = 500u;
    kfe::KFEWindows windows{desc};

    if (!windows.Initialize())
    {
        return E_FAIL;
    }

    while (true)
    {
        auto state = windows.ProcessMessage();
        if (state == kfe::EProcessedMessage::QuitInvoked)
        {
            return S_OK;
        }
    }

    return S_OK;
}
