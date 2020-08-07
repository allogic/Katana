#include <Windows.h>

LRESULT CALLBACK WindowProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
  WNDCLASSA wndClass {};
  wndClass.lpfnWndProc = WindowProcA;
  wndClass.hInstance = hInstance;
  wndClass.lpszClassName = "DemoWindowWndClass";

  RegisterClassA(&wndClass);

  HWND hwnd = CreateWindowExA(
    0,
    "DemoWindowWndClass",
    "DemoWindow",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    nullptr,
    nullptr,
    hInstance,
    nullptr
  );

  ShowWindow(hwnd, nCmdShow);

  MSG msg {};

  while (GetMessageA(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  return 0;
}