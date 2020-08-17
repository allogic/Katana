#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>
#include <psapi.h>
#include <d3d11.h>
#include <profileapi.h>

#include <CommonStates.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <SimpleMath.h>
#include <GraphicsMemory.h>

#define _STR(V) #V
#define STR(V) _STR(V)

#define TRACE_IF(EXPR)                        \
{                                             \
  HRESULT hr = (EXPR);                        \
  if (hr)                                     \
    printf("Error[0x%x]" STR(EXPR) "\n", hr); \
}

#define MEASURE_BEGIN(NAME)                     \
LARGE_INTEGER measure_begin_##NAME {};          \
QueryPerformanceCounter(&measure_begin_##NAME);

#define MEASURE_END(NAME)                                                                       \
LARGE_INTEGER measure_end_##NAME {};                                                            \
LARGE_INTEGER system_frequency_##NAME {};                                                       \
LARGE_INTEGER measure_duration_##NAME {};                                                       \
QueryPerformanceCounter(&measure_end_##NAME);                                                   \
QueryPerformanceFrequency(&system_frequency_##NAME);                                            \
measure_duration_##NAME.QuadPart = measure_end_##NAME.QuadPart - measure_begin_##NAME.QuadPart; \
measure_duration_##NAME.QuadPart *= 1000000;                                                    \
measure_duration_##NAME.QuadPart /= system_frequency_##NAME.QuadPart;                           \
printf(STR(NAME) "[%lldms]\n", measure_duration_##NAME.QuadPart);

template<unsigned NumModules>
int QueryModules(DWORD pid)
{
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
  HMODULE hMods[NumModules]{};
  DWORD cbNeeded{};

  if (EnumProcessModulesEx(hProc, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
  {
    for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
    {
      CHAR modName[MAX_PATH]{};

      if (GetModuleFileNameExA(hProc, hMods[i], modName, sizeof(modName) / sizeof(CHAR)))
        printf("Module[0x%p] %s\n", &hMods[i], modName);
    }
  }

  return 0;
}

template<unsigned PageSize>
void DumpMemory(DWORD pid, uintptr_t from, int numPages)
{
  HANDLE hProc = OpenProcess(PROCESS_VM_READ, 0, pid);
  char pBuffer[PageSize] {};
  const SIZE_T bytesToRead = sizeof(char) * PageSize;
  SIZE_T bytesRead {};

  for (int i = 0; i < numPages; i += PageSize)
  {
    uintptr_t addr = from + i;

    if (!ReadProcessMemory(hProc, (void*)addr, pBuffer, bytesToRead, &bytesRead) || bytesToRead != bytesRead)
      printf("Error[0x%p]", (void*)addr);
    else
    {
      printf("[0x%x]", addr);

      for (int j = 0; j < PageSize; j++)
        printf("%d ", pBuffer[j]);

      for (int j = 0; j < PageSize; j++)
        printf("%c", pBuffer[j]);
    }

    printf("\n");
  }
}

DWORD WINAPI ConsoleThread(HMODULE hModule)
{
  AllocConsole();

  freopen("CONOUT$", "w", stdout);

  DWORD pid = GetCurrentProcessId();
  HWND hWnd = FindWindowExA(nullptr, nullptr, "DeadSpace3WndClass", "Dead Space™ 3");
  //hWnd hWnd = FindWindowExA(nullptr, nullptr, "DemoWindowWndClass", "DemoWindow");
  DWORD threadId = GetWindowThreadProcessId(hWnd, &pid);
  HMODULE hMod = GetModuleHandleA(0);

  printf("Pid[%d]\n", pid);
  printf("Thread[%d]\n", threadId);
  printf("hWnd[%p]\n", hWnd);
  printf("hMod[%p]\n", hMod);

  QueryModules<1024>(pid);
  DumpMemory<16>(pid, 0x400000, 16);

  using namespace DirectX;
  using namespace DirectX::SimpleMath;

  const int width = 1280;
  const int height = 720;

  const void* pDxShaderByteCode = nullptr;
  size_t dxByteCodeLength = 0;

  D3D_FEATURE_LEVEL dxFeature;
  D3D_FEATURE_LEVEL pDxFeatures[] { D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0 };

  IDXGIFactory* pDxFactory = nullptr;
  IDXGIAdapter* pDxAdapter = nullptr;
  IDXGIOutput* pDxOutput = nullptr;
  IDXGISwapChain* pDxSwapChain = nullptr;

  DXGI_MODE_DESC dxBufferModeDesc {};
  dxBufferModeDesc.Width = width;
  dxBufferModeDesc.Height = height;
  dxBufferModeDesc.RefreshRate = DXGI_RATIONAL{ 60, 1 };
  dxBufferModeDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
  dxBufferModeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  dxBufferModeDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;

  DXGI_SWAP_CHAIN_DESC dxSwapChainDesc {};
  dxSwapChainDesc.BufferDesc = dxBufferModeDesc;
  dxSwapChainDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
  dxSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  dxSwapChainDesc.BufferCount = 1;
  dxSwapChainDesc.OutputWindow = hWnd;
  dxSwapChainDesc.Windowed = 1;
  dxSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD;
  dxSwapChainDesc.Flags = 0;

  D3D11_TEXTURE2D_DESC dxRenderTextureDesc2D {};
  dxRenderTextureDesc2D.Width = width;
  dxRenderTextureDesc2D.Height = height;
  dxRenderTextureDesc2D.MipLevels = 1;
  dxRenderTextureDesc2D.ArraySize = 1;
  dxRenderTextureDesc2D.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
  dxRenderTextureDesc2D.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
  dxRenderTextureDesc2D.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
  dxRenderTextureDesc2D.BindFlags =
    D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET |
    D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
  dxRenderTextureDesc2D.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
  dxRenderTextureDesc2D.MiscFlags = 0;

  ID3D11Device* pDxDevice = nullptr;
  ID3D11DeviceContext* pDxContext = nullptr;
  ID3D11InputLayout* pDxInputLayout = nullptr;
  ID3D11Texture2D* pDxBackBufferTexture2D = nullptr;
  ID3D11Texture2D* pDxRenderTexture2D = nullptr;
  ID3D11RenderTargetView* pDxRenderTargetView2D = nullptr;
  ID3D11DepthStencilView* pDxDepthStencilView2D = nullptr;

  CommonStates* pDxStates = nullptr;
  BasicEffect* pDxEffect = nullptr;
  PrimitiveBatch<VertexPositionColor>* pDxBatch = nullptr;

  TRACE_IF(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDxFactory));
  TRACE_IF(pDxFactory->EnumAdapters(0, &pDxAdapter));
  TRACE_IF(pDxAdapter->EnumOutputs(0, &pDxOutput));
  TRACE_IF(D3D11CreateDeviceAndSwapChain(
    nullptr,
    D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    0,
    pDxFeatures,
    1,
    D3D11_SDK_VERSION,
    &dxSwapChainDesc,
    &pDxSwapChain,
    &pDxDevice,
    &dxFeature,
    &pDxContext
  ));
  TRACE_IF(pDxSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pDxBackBufferTexture2D));
  TRACE_IF(pDxDevice->CreateTexture2D(&dxRenderTextureDesc2D, nullptr, &pDxRenderTexture2D));
  TRACE_IF(pDxDevice->CreateRenderTargetView(pDxBackBufferTexture2D, nullptr, &pDxRenderTargetView2D));

  pDxStates = new CommonStates(pDxDevice);
  pDxEffect = new BasicEffect(pDxDevice);

  pDxEffect->SetVertexColorEnabled(true);
  pDxEffect->GetVertexShaderBytecode(&pDxShaderByteCode, &dxByteCodeLength);

  TRACE_IF(pDxDevice->CreateInputLayout(
    VertexPositionColor::InputElements,
    VertexPositionColor::InputElementCount,
    pDxShaderByteCode,
    dxByteCodeLength,
    &pDxInputLayout
  ));
  
  pDxBatch = new PrimitiveBatch<VertexPositionColor>(pDxContext);

  Matrix world;
  Matrix view;
  Matrix proj;

  world = Matrix::Identity;
  //view = Matrix::CreateLookAt(-Vector3::UnitZ, Vector3::Zero, Vector3::UnitY);
  view = Matrix::CreateLookAt(Vector3{ 5.f, 5.f, 5.f }, Vector3::Zero, Vector3::UnitY);
  //proj = Matrix::CreateOrthographicOffCenter(-0.5f, 0.5f, -0.5f, 0.5f, 0.001f, 1.f);
  proj = Matrix::CreatePerspectiveFieldOfView(
    XM_PI / 4.f,
    float(width) / float(height),
    0.001f, 1.f
  );

  CD3D11_VIEWPORT viewPort(0.f, 0.f, (float)width, (float)height);
  pDxContext->RSSetViewports(1, &viewPort);

  while (true)
  {
    //MEASURE_BEGIN(MainLoop);

    pDxContext->OMSetBlendState(pDxStates->Opaque(), nullptr, 0xFFFFFFFF);
    pDxContext->OMSetDepthStencilState(pDxStates->DepthNone(), 0);
    pDxContext->RSSetState(pDxStates->CullNone());

    pDxEffect->Apply(pDxContext);

    pDxContext->IASetInputLayout(pDxInputLayout);
    
    //float clearColor[4] { 0.0f, 0.5f, 0.f, 1.f };
    //
    //pDxContext->ClearRenderTargetView(pDxRenderTargetView2D, clearColor);
    //pDxContext->ClearDepthStencilView(
    //  pDxDepthStencilView2D,
    //  D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL,
    //  1.0f,
    //  0
    //);

    pDxContext->OMSetRenderTargets(1, &pDxRenderTargetView2D, pDxDepthStencilView2D);
    
    pDxEffect->SetView(view);
    pDxEffect->SetProjection(proj);
    pDxEffect->SetWorld(world);
    
    Vector3 xaxis(2.f, 0.f, 0.f);
    Vector3 yaxis(0.f, 0.f, 2.f);
    Vector3 origin = Vector3::Zero;

    XMFLOAT4 color{ 1.f, 0.f, 0.f, 1.f };

    size_t divisions = 20;

    pDxBatch->Begin();

    for (size_t i = 0; i <= divisions; ++i)
    {
      float fPercent = float(i) / float(divisions);
      fPercent = (fPercent * 2.0f) - 1.0f;

      Vector3 scale = xaxis * fPercent + origin;

      VertexPositionColor v1(scale - yaxis, color);
      VertexPositionColor v2(scale + yaxis, color);
      pDxBatch->DrawLine(v1, v2);
    }
    for (size_t i = 0; i <= divisions; i++)
    {
      float fPercent = float(i) / float(divisions);
      fPercent = (fPercent * 2.0f) - 1.0f;

      Vector3 scale = yaxis * fPercent + origin;

      VertexPositionColor v1(scale - xaxis, color);
      VertexPositionColor v2(scale + xaxis, color);
      pDxBatch->DrawLine(v1, v2);
    }

    pDxBatch->End();

    //pDxBatch->Begin();
    //
    //pDxBatch->DrawLine(
    //  VertexPositionColor(XMFLOAT3{ -0.5f, 0.f, 0.f }, XMFLOAT4{ 1.f, 0.f, 0.f, 1.f }),
    //  VertexPositionColor(XMFLOAT3{ 0.5f, -0.5f, 0.f }, XMFLOAT4{ 1.f, 0.f, 0.f, 1.f })
    //);
    //
    //pDxBatch->End();
    
    pDxSwapChain->Present(0, 0 /* DXGI_PRESENT_DO_NOT_SEQUENCE */);

    //pDxOutput->WaitForVBlank();

    //MEASURE_END(MainLoop, /* interval */);
  }

  return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    CloseHandle(
      CreateThread(
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)ConsoleThread,
        hModule,
        0,
        nullptr
      )
    );
    break;
  }

  return TRUE;
}

