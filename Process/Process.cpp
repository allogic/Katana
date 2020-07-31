#include "Process.h"

int Execute(int argc, char** argv)
{  
  STARTUPINFOA startupInfo {};
  PROCESS_INFORMATION processInfo {};

  if (argc < 0)
  {
    std::printf("Invalide arguments\n");
    return -1;
  }

  bool processCreated = CreateProcessA(
    nullptr,
    argv[0],
    nullptr,
    nullptr,
    false,
    CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
    nullptr, nullptr,
    &startupInfo,
    &processInfo
  );

  if (!processCreated)
  {
    return -1;
  }

  WaitForSingleObject(processInfo.hProcess, INFINITE);

  DWORD exitCode;
  GetExitCodeThread(processInfo.hThread, &exitCode);

  CloseHandle(processInfo.hProcess);
  CloseHandle(processInfo.hThread);

  return exitCode;
}