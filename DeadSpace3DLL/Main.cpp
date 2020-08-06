#include <cstdio>
#include <cstdlib>

#include <d3d11.h>

#include <CommonStates.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <SimpleMath.h>
#include <GraphicsMemory.h>

#define _STR(V) #V
#define STR(V) _STR(V)

#define TRACE_IF(EXPR)                             \
{                                                  \
  HRESULT hr = (EXPR);                             \
  if (hr)                                          \
    std::printf("Error[0x%x]" STR(EXPR) "\n", hr); \
}

DWORD WINAPI ConsoleThread(HMODULE hModule)
{
  AllocConsole();

  std::freopen("CONOUT$", "w", stdout);

  DWORD pid = GetCurrentProcessId();
  HWND hwnd = FindWindowExA(nullptr, nullptr, "DeadSpace3WndClass", "Dead Space™ 3");
  DWORD threadId = GetWindowThreadProcessId(hwnd, &pid);

  std::printf("Thread[%d] Process[%d] Hwnd[%p]\n", threadId, pid, hwnd);

  using namespace DirectX;
  using namespace DirectX::SimpleMath;

  const int width = 1280;
  const int height = 720;

  D3D_FEATURE_LEVEL dxFeature;
  D3D_FEATURE_LEVEL pDxFeatures[]
  {
    D3D_FEATURE_LEVEL_11_0
  };

  IDXGIFactory2* pDxFactory = nullptr;
  IDXGIAdapter* pDxAdapter = nullptr;

  const D3D11_RASTERIZER_DESC dxRasterDesc
  {
    D3D11_FILL_MODE::D3D11_FILL_SOLID,
    D3D11_CULL_MODE::D3D11_CULL_NONE,
    0, 0,
    0.f, 0.f,
    0, 0, 0, 0,
  };

  ID3D11Device* pDxDevice = nullptr;
  ID3D11DeviceContext* pDxContext = nullptr;
  ID3D11InputLayout* pDxInputLayout = nullptr;
  ID3D11RasterizerState* mpDxRasterizerState = nullptr;

  GraphicsMemory* mpDxGraphicsMemory = nullptr;
  CommonStates* pDxStates = nullptr;
  BasicEffect* pDxEffect = nullptr;
  PrimitiveBatch<VertexPositionColor>* pDxBatch = nullptr;

  Matrix world;
  Matrix view;
  Matrix proj;

  TRACE_IF(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDxFactory));
  TRACE_IF(D3D11CreateDeviceAndSwapChain(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    0,
    pDxFeatures,
    1,
    D3D11_SDK_VERSION,
    nullptr,
    nullptr,
    &pDxDevice,
    &dxFeature,
    &pDxContext
  ));
  TRACE_IF(pDxDevice->CreateRasterizerState(
    &dxRasterDesc,
    &mpDxRasterizerState
  ));

  //pDxGraphicsMemory = new GraphicsMemory(pDxDevice, pDxDevice->Reso);

  pDxStates = new CommonStates(pDxDevice);

  pDxEffect = new BasicEffect(pDxDevice);
  pDxEffect->SetVertexColorEnabled(true);
  
  void const* shaderByteCode;
  size_t byteCodeLength;
  
  pDxEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
  
  pDxDevice->CreateInputLayout(
    VertexPositionColor::InputElements,
    VertexPositionColor::InputElementCount,
    shaderByteCode, byteCodeLength,
    &pDxInputLayout
  );
  
  pDxBatch = new PrimitiveBatch<VertexPositionColor>(pDxContext);

  world = Matrix::Identity;
  view = Matrix::CreateLookAt(-Vector3::UnitZ, Vector3::Zero, Vector3::UnitY);
  proj = Matrix::CreateOrthographicOffCenter(-0.5, 0.5, -0.5, 0.5, 0.0001f, 1000.f);
  //proj = Matrix::CreatePerspectiveFieldOfView(
  //  XM_PI / 4.f,
  //  float(width) / float(height),
  //  0.1f, 10.f
  //);

  pDxEffect->SetView(view);
  pDxEffect->SetProjection(proj);
  pDxEffect->SetWorld(world);

  pDxEffect->Apply(pDxContext);

  //pDxContext->OMSetBlendState(pDxStates->Opaque(), nullptr, 0xFFFFFFFF);
  //pDxContext->OMSetDepthStencilState(pDxStates->DepthNone(), 0);
  //pDxContext->RSSetState(pDxStates->CullNone());

  pDxContext->IASetInputLayout(pDxInputLayout);

  while (true)
  {
    pDxBatch->Begin();

    pDxBatch->DrawLine(
      VertexPositionColor(XMFLOAT3(-0.5f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f)),
      VertexPositionColor(XMFLOAT3(0.5f, -0.5f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f))
    );

    pDxBatch->End();

    //pDxContext->RSSetState(pDxStates->CullNone());
    pDxContext->RSSetState(mpDxRasterizerState);
  }

  return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ConsoleThread, hModule, 0, nullptr));
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

