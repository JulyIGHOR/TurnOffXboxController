/*
 * Copyright (c) 2013-2021, Ighor July, julyighor@gmail.com
 * https://github.com/JulyIghor/TurnOffXboxController
 * Donate Bitcoin 1PswUbmymM22Xx7qi7xuMwRKyTc7sf62Zb
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the <organization>.
 * 4. Neither the name of the Ighor July nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY Ighor July "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Ighor July BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "windows.h"
#include "xinput.h"

#pragma comment(lib, "XInput.lib")

// context menu IDs
#define ID_EXIT 2000
#define ID_TURN_OFF_CONTROLLERS 2001
#define ID_OPEN_REPO 2002

const char* gClassName = "XboxTurnOffController";
const char* gProgramName = "Turn Off Xbox 360 Controller v1.2";

void turnOffController(HINSTANCE hXInputDLL)
{
    for (short i = 0; i < 4; ++i)
    {
        XINPUT_STATE state;
        memset(&state, 0, sizeof(XINPUT_STATE));

        if (XInputGetState(i, &state) == ERROR_SUCCESS)
        {
            typedef DWORD(WINAPI * XInputPowerOffController_t)(DWORD i);
            XInputPowerOffController_t realXInputPowerOffController = (XInputPowerOffController_t)GetProcAddress(hXInputDLL, (LPCSTR)103);
            realXInputPowerOffController(i);
        }

        ZeroMemory(&state, sizeof(XINPUT_STATE));
    }
}

LRESULT WINAPI wndProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NOTIFYICONDATAA nid = {
        .cbSize{sizeof(NOTIFYICONDATA)},
        .hWnd{hWnd},
        .uID{1},
        .uFlags{NIF_MESSAGE | NIF_ICON | NIF_TIP},
        .uCallbackMessage{WM_USER},
        .hIcon{LoadIcon(GetModuleHandle(NULL), "IDI_ICON1")},
    };

    strncpy(nid.szTip, gProgramName, 64);

    switch (msg)
    {
    case WM_CREATE:
        {
            if (!Shell_NotifyIcon(NIM_ADD, &nid))
            {
                MessageBox(hWnd, "Failed to display icon in system tray", gProgramName, MB_OK | MB_ICONEXCLAMATION);
            }
            break;
        }

    case WM_COMMAND:
        if (HIWORD(wParam) == 0)
        {
            switch (LOWORD(wParam))
            {
            case ID_EXIT:
                {
                    Shell_NotifyIcon(NIM_DELETE, &nid);
                    DestroyWindow(hWnd);
                    break;
                }
            case ID_TURN_OFF_CONTROLLERS:
                {
                    turnOffController(*(HINSTANCE*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
                    break;
                }
            case ID_OPEN_REPO:
                {
                    ShellExecute(NULL, "open", "https://github.com/JulyIghor/TurnOffXboxController/issues", NULL, NULL, SW_SHOWNORMAL);
                    break;
                }
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    // callback message from notification
    case WM_USER:
        switch (LOWORD(lParam))
        {
        case WM_RBUTTONDOWN:
            {
                HMENU hPopup = CreatePopupMenu();
                InsertMenu(hPopup, 0, MF_BYPOSITION | MF_STRING, ID_EXIT, "E&xit");
                InsertMenu(hPopup, 0, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
                InsertMenu(hPopup, 0, MF_BYPOSITION | MF_STRING, ID_TURN_OFF_CONTROLLERS, "Turn &off all Xbox 360 controllers");
                InsertMenu(hPopup, 0, MF_BYPOSITION | MF_STRING, ID_OPEN_REPO, "Send &feedback or a bug report");
                SetForegroundWindow(hWnd);

                POINT pt;
                GetCursorPos(&pt);

                TrackPopupMenu(hPopup, TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x, pt.y, 0, hWnd, NULL);
                break;
            }
        case WM_LBUTTONDBLCLK:
            {
                turnOffController(*(HINSTANCE*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
                break;
            }
        }
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    CreateEvent(NULL, FALSE, FALSE, "TurnOffXboxControllerInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(
            NULL,
            "Application is already started. It is working as system tray icon.\nUse context menu to turn them of, or double click the tray icon.\nIf you wish to turn off all controllers by a desktop shortcut, just create a shortcut of this app with a /silent parameter",
            gProgramName,
            MB_OK | MB_ICONINFORMATION);

        return 0;
    }

    HINSTANCE hXInputDLL = LoadLibraryA("XInput1_3.dll");

    if (hXInputDLL == NULL)
    {
        MessageBox(NULL,
                   "Xbox 360 controller driver not found (XInput1_3.dll)\n\n"
                   "If your controllers are either than 360 model, just hold X button to turn it off",
                   gProgramName,
                   MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }

    int exitCode = 0;
    if (strlen(lpCmdLine))
        turnOffController(hXInputDLL);
    else
    {

        WNDCLASSEX wndClass{
            .cbSize{sizeof(WNDCLASSEX)},
            .style{0},
            .lpfnWndProc{wndProcedure},
            .hInstance{hInstance},
            .lpszClassName{gClassName},
        };

        if (!RegisterClassEx(&wndClass))
        {
            MessageBox(NULL, "wndClass registration failed\n", gProgramName, MB_OK | MB_ICONEXCLAMATION);
            exitCode = 1;
        }
        else
        {
            HWND hWnd = CreateWindowEx(0, gClassName, gProgramName, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

            if (!hWnd)
            {
                MessageBox(NULL, "Window creation failed\n", gProgramName, MB_OK | MB_ICONEXCLAMATION);
                exitCode = 2;
            }
            else
            {
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)&hXInputDLL);

                MSG msg;
                while (GetMessage(&msg, hWnd, 0, 0) > 0)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                exitCode = msg.wParam;
            }
        }
    }
    FreeLibrary(hXInputDLL);

    // Killing own process since XInput1_3.dll threads stuck and won't exit on Windows 10
    // If you know another way to fix it, feel free to send commit here https://github.com/JulyIghor/TurnOffXboxController
    TerminateProcess(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId()), 0);

    return exitCode;
}
