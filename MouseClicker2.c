#include <stdio.h>
#include <windows.h>
#include <tchar.h>

// 全局变量
BOOL g_bRunning = FALSE;    // 连点运行状态
DWORD g_dwInterval = 1;     // 点击间隔(毫秒)，默认1ms
HANDLE g_hClickThread = NULL; // 连点线程句柄
HWND g_hEditInterval = NULL;  // 间隔输入框句柄
HWND g_hBtnStart = NULL;      // 开始按钮句柄
HWND g_hBtnStop = NULL;       // 停止按钮句柄

// 鼠标连点线程函数
DWORD WINAPI ClickThread(LPVOID lpParam)
{
    while (g_bRunning)
    {
        // 模拟鼠标左键单击
        mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        // 等待指定间隔
        Sleep(g_dwInterval);
        
        // 检测ESC键，按下则停止
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            g_bRunning = FALSE;
        }
    }
    return 0;
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        // 创建界面控件
        {
            // 静态文本 - 提示文字
            CreateWindow(_T("STATIC"), _T("点击间隔(毫秒)："), 
                WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                20, 20, 120, 30, hWnd, NULL, GetModuleHandle(NULL), NULL);

            // 输入框 - 间隔设置
            g_hEditInterval = CreateWindow(_T("EDIT"), _T("1"), 
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                140, 20, 80, 30, hWnd, NULL, GetModuleHandle(NULL), NULL);

            // 开始按钮
            g_hBtnStart = CreateWindow(_T("BUTTON"), _T("开始连点(空格)"), 
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 70, 120, 40, hWnd, (HMENU)1, GetModuleHandle(NULL), NULL);

            // 停止按钮（初始禁用）
            g_hBtnStop = CreateWindow(_T("BUTTON"), _T("停止连点(ESC)"), 
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED,
                150, 70, 120, 40, hWnd, (HMENU)2, GetModuleHandle(NULL), NULL);
        }
        break;

    case WM_COMMAND:
        // 按钮点击事件
        switch (LOWORD(wParam))
        {
        case 1: // 开始按钮
            {
                // 获取输入框的间隔值
                TCHAR szInterval[16] = {0};
                GetWindowText(g_hEditInterval, szInterval, 16);
                g_dwInterval = _ttoi(szInterval);
                // 防止输入0或负数
                if (g_dwInterval < 1) g_dwInterval = 1;

                // 启动连点
                if (!g_bRunning)
                {
                    g_bRunning = TRUE;
                    // 创建连点线程
                    g_hClickThread = CreateThread(NULL, 0, ClickThread, NULL, 0, NULL);
                    // 切换按钮状态
                    EnableWindow(g_hBtnStart, FALSE);
                    EnableWindow(g_hBtnStop, TRUE);
                    SetWindowText(hWnd, _T("鼠标连点器 - 运行中"));
                }
            }
            break;

        case 2: // 停止按钮
            g_bRunning = FALSE;
            // 等待线程结束
            if (g_hClickThread)
            {
                WaitForSingleObject(g_hClickThread, INFINITE);
                CloseHandle(g_hClickThread);
                g_hClickThread = NULL;
            }
            // 切换按钮状态
            EnableWindow(g_hBtnStart, TRUE);
            EnableWindow(g_hBtnStop, FALSE);
            SetWindowText(hWnd, _T("鼠标连点器 - 已停止"));
            break;
        }
        break;

    case WM_KEYDOWN:
        // 空格键启动（和按钮功能一致）
        if (wParam == VK_SPACE && !g_bRunning)
        {
            SendMessage(hWnd, WM_COMMAND, MAKELONG(1, 0), 0);
        }
        // ESC键停止（和按钮功能一致）
        else if (wParam == VK_ESCAPE && g_bRunning)
        {
            SendMessage(hWnd, WM_COMMAND, MAKELONG(2, 0), 0);
        }
        break;

    case WM_CLOSE:
        // 关闭前停止连点
        if (g_bRunning)
        {
            g_bRunning = FALSE;
            if (g_hClickThread)
            {
                WaitForSingleObject(g_hClickThread, INFINITE);
                CloseHandle(g_hClickThread);
            }
        }
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 注册窗口类
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("MouseClickerClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, _T("窗口类注册失败！"), _T("错误"), MB_ICONERROR);
        return 1;
    }

    // 创建主窗口
    HWND hWnd = CreateWindow(
        _T("MouseClickerClass"),
        _T("鼠标连点器 - 已停止"),
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, // 禁用最大化按钮
        CW_USEDEFAULT, CW_USEDEFAULT,
        300, 180, // 窗口大小
        NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, _T("窗口创建失败！"), _T("错误"), MB_ICONERROR);
        return 1;
    }

    // 显示窗口
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}