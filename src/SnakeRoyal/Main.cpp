#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>
#include "resource.h"

#include "Config.h"
#include "Game.h"
#include "Utils.h"
#include "Logging.h"
#include "Network.h"

// Data
static HWND _hWnd;

// Functions
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static bool ProcessCommandLine(LPTSTR lpCmdLine);
static void Init(HINSTANCE hInstance);
static void Shutdown();
static void GameLoop();

// Main
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_QUIT:
    case WM_DESTROY:
        DestroyWindow(hWnd);
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        gGame.draw();
        break;
    case WM_KILLFOCUS:
        gGame.setFocus(false);
        break;
    case WM_SETFOCUS:
        gGame.setFocus(true);
        break;
    case WM_ERASEBKGND:
        return TRUE;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    if (!ProcessCommandLine(lpCmdLine))
    {
        return EXIT_FAILURE;
    }

    Init(hInstance);
    GameLoop();
    Shutdown();

    return EXIT_SUCCESS;
}

static bool ProcessCommandLine(LPTSTR lpCmdLine)
{
    std::vector<std::string> args;

    // Convert.
    {
        int argc;
        LPWSTR* szArgList = CommandLineToArgvW(lpCmdLine, &argc);

        for (int i = 0; i < argc; i++)
        {
            args.push_back(Utils::toMBString(szArgList[i]));
        }

        LocalFree(szArgList);
    }

    // Process
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (args[i] == "host")
            {
                std::string host = NETWORK_DEFAULT_HOST;
                uint16_t port = NETWORK_DEFAULT_PORT;

                int n = 0;
                if (i + 2 < args.size())
                {
                    host = args[i + 2];
                    ++n;
                }
                if (i + 1 < args.size())
                {
                    if (args[i + 1][0] != '-' && args[i + 1][1] != '-')
                    {
                        port = static_cast<uint16_t>(atol(args[i + 1].c_str()));
                        ++n;
                    }
                }

                gNetwork.startServer(host.c_str(), port);
                i += n;
            }
            else if (args[i] == "join")
            {
                if (i + 1 >= args.size())
                {
                    logPrint("ERROR: Invalid parameters: join <address>\n");
                    return false;
                }

                std::string host = args[i + 1];
                uint16_t port = NETWORK_DEFAULT_PORT;

                size_t portPos = host.find_last_of(':');
                if (portPos != host.npos)
                {
                    port = static_cast<uint16_t>(atol(host.c_str() + portPos + 1));
                    host = host.substr(0, portPos);
                }

                gNetwork.startClient(host.c_str(), port);
                ++i;
            }
            else if (args[i] == "--headless")
            {
                gGame.setHeadless(true);
            }
        }
    }

    return true;
}

static void Init(HINSTANCE hInstance)
{
    if (gGame.getHeadless() == false)
    {
        WNDCLASSEX wc{};

        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpszClassName = L"DenuvoGame";
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpfnWndProc = WndProc;
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.style = 0;
        wc.cbWndExtra = 0;
        wc.cbClsExtra = 0;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

        RegisterClassEx(&wc);
        _hWnd = CreateWindowW(wc.lpszClassName,
            L"Snake Royal",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            WINDOW_SIZE_W,
            WINDOW_SIZE_H,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        gGame.init(_hWnd);
        return;
    }

    gGame.init(nullptr);
}

static void Shutdown()
{
}

static void GameLoop()
{
    MSG msg{};

    double accumulator = 0.0;
    double lastTime = Utils::getTime();

    bool exitLoop = false;

    while (!exitLoop)
    {
        if (gGame.getHeadless() == false)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    exitLoop = true;
                    continue;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        double currentTime = Utils::getTime();
        double elapsed = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += elapsed;

        gNetwork.update();

        if (accumulator < GAME_TICK_RATE)
        {
            std::this_thread::yield();
        }
        else
        {
            while (accumulator >= GAME_TICK_RATE)
            {
                gGame.update();
                accumulator -= GAME_TICK_RATE;
            }
        }

        if (gGame.getHeadless() == false)
        {
            // Force redrawing
            RedrawWindow(_hWnd, nullptr, nullptr, RDW_INVALIDATE);
        }

        gNetwork.flush();
    }
}
