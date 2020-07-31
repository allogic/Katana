#include <Windows.h>

#include <Process.h>

#include <cstdio>
#include <string>

HANDLE Inject(const char* pFileName, int pid)
{
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

  LPVOID dllPathAddr = VirtualAllocEx(hProcess, nullptr, _MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  WriteProcessMemory(hProcess, dllPathAddr, pFileName, std::strlen(pFileName), nullptr);

  LPVOID loadLibAddr = GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryA");

  HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, dllPathAddr, 0, nullptr);

  WaitForSingleObject(hThread, INFINITE);

  DWORD exitCode;
  GetExitCodeThread(hThread, &exitCode);

  CloseHandle(hThread);

  VirtualFreeEx(hProcess, dllPathAddr, 0, MEM_RELEASE);

  CloseHandle(hProcess);

  return (HANDLE)exitCode;
}

typedef int (__cdecl *ProcExecute)(int argc, char** argv);

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::printf("Invalide arguments\n");
    return -1;
  }

  HINSTANCE hInstance = LoadLibraryA(argv[1]);

  if (!hInstance)
  {
    std::printf("Failed loading module\n");
    return -1;
  }

  auto hProc = (ProcExecute)GetProcAddress(hInstance, "Execute");

  if (!hProc)
  {
    std::printf("Failed loading procedure\n");
    return -1;
  }
  
  return hProc(argc - 2, &argv[2]);
}