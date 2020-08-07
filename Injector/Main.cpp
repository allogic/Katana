#include <cstdio>
#include <string>

#include <Windows.h>
#include <TlHelp32.h>

DWORD FindProcessId(const char* processname)
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  DWORD result = NULL;

  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

  pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if (!Process32First(hProcessSnap, &pe32))
  {
    CloseHandle(hProcessSnap);          // clean the snapshot object
    std::printf("!!! Failed to gather information on system processes! \n");
    return(NULL);
  }

  do
  {
    std::printf("Checking process %s\n", pe32.szExeFile);
    if (0 == strcmp(processname, pe32.szExeFile))
    {
      result = pe32.th32ProcessID;
      break;
    }
  } while (Process32Next(hProcessSnap, &pe32));

  CloseHandle(hProcessSnap);

  return result;
}

HANDLE Inject(const char* pFileName, int pid)
{
  // directly inject dll befor process deadspace3.exe starts

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

int main()
{
  const char* pFileName = "C:\\Users\\burmi\\Downloads\\Katana\\x64\\Debug\\DeadSpace3DLL.dll";
  DWORD pid = FindProcessId("DemoProcess.exe");

  return (bool)Inject(pFileName, pid);
}