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
  if ((bool)hr)                                    \
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

  void const* pDxShaderByteCode = nullptr;
  size_t dxByteCodeLength = 0;

  D3D_FEATURE_LEVEL dxFeature;
  D3D_FEATURE_LEVEL pDxFeatures[] { D3D_FEATURE_LEVEL_11_0 };

  IDXGIFactory2* pDxFactory = nullptr;
  IDXGIAdapter* pDxAdapter = nullptr;

  //const D3D11_RASTERIZER_DESC dxRasterDesc
  //{
  //  D3D11_FILL_MODE::D3D11_FILL_SOLID,
  //  D3D11_CULL_MODE::D3D11_CULL_NONE,
  //  0, 0,
  //  0.f, 0.f,
  //  0, 0, 0, 0,
  //};

  D3D11_TEXTURE2D_DESC dxRenderTextureDesc2D;
  dxRenderTextureDesc2D.Width = width;
  dxRenderTextureDesc2D.Height = height;
  dxRenderTextureDesc2D.MipLevels = 0;
  dxRenderTextureDesc2D.ArraySize = 1;
  dxRenderTextureDesc2D.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
  dxRenderTextureDesc2D.SampleDesc = DXGI_SAMPLE_DESC { 1, 0 };
  dxRenderTextureDesc2D.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
  dxRenderTextureDesc2D.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
  dxRenderTextureDesc2D.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
  dxRenderTextureDesc2D.MiscFlags = 0;

  D3D11_RENDER_TARGET_VIEW_DESC dxRenderTextureViewDesc2D;
  dxRenderTextureViewDesc2D.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
  dxRenderTextureViewDesc2D.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
  dxRenderTextureViewDesc2D.Texture2D = D3D11_TEX2D_RTV { 0 };

  ID3D11Device* pDxDevice = nullptr;
  ID3D11DeviceContext* pDxContext = nullptr;
  ID3D11InputLayout* pDxInputLayout = nullptr;
  //ID3D11RasterizerState* pDxRasterizerState = nullptr;
  ID3D11Texture2D* pDxRenderTexture2D = nullptr;
  ID3D11RenderTargetView* pDxRenderTargetView2D = nullptr;
  ID3D11DepthStencilView* pDxDepthStencilView2D = nullptr;

  //GraphicsMemory* mpDxGraphicsMemory = nullptr;
  CommonStates* pDxStates = nullptr;
  BasicEffect* pDxEffect = nullptr;
  PrimitiveBatch<VertexPositionColor>* pDxBatch = nullptr;

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
  TRACE_IF(pDxDevice->CreateTexture2D(
    &dxRenderTextureDesc2D,
    nullptr,
    &pDxRenderTexture2D
  ));
  TRACE_IF(pDxDevice->CreateRenderTargetView(
    pDxRenderTexture2D,
    &dxRenderTextureViewDesc2D,
    &pDxRenderTargetView2D
  ));
  //TRACE_IF(pDxDevice->CreateRasterizerState(
  //  &dxRasterDesc,
  //  &mpDxRasterizerState
  //));

  //pDxGraphicsMemory = new GraphicsMemory(pDxDevice, pDxDevice->Reso);

  pDxStates = new CommonStates(pDxDevice);
  
  pDxEffect = new BasicEffect(pDxDevice);
  pDxEffect->SetVertexColorEnabled(true);  
  pDxEffect->GetVertexShaderBytecode(&pDxShaderByteCode, &dxByteCodeLength);
  
  pDxDevice->CreateInputLayout(
    VertexPositionColor::InputElements,
    VertexPositionColor::InputElementCount,
    pDxShaderByteCode,
    dxByteCodeLength,
    &pDxInputLayout
  );
  
  pDxBatch = new PrimitiveBatch<VertexPositionColor>(pDxContext);

  Matrix world;
  Matrix view;
  Matrix proj;

  world = Matrix::Identity;
  view = Matrix::CreateLookAt(-Vector3::UnitZ, Vector3::Zero, Vector3::UnitY);
  proj = Matrix::CreateOrthographicOffCenter(-0.5, 0.5, -0.5, 0.5, 0.0001f, 1.f);
  //proj = Matrix::CreatePerspectiveFieldOfView(
  //  XM_PI / 4.f,
  //  float(width) / float(height),
  //  0.1f, 10.f
  //);

  pDxContext->OMSetBlendState(pDxStates->Opaque(), nullptr, 0xFFFFFFFF);
  pDxContext->OMSetDepthStencilState(pDxStates->DepthNone(), 0);

  pDxContext->RSSetState(pDxStates->CullNone());

  pDxEffect->SetView(view);
  pDxEffect->SetProjection(proj);
  pDxEffect->SetWorld(world);
  
  pDxEffect->Apply(pDxContext);

  pDxContext->IASetInputLayout(pDxInputLayout);

  while (true)
  {
    float clearColor[4] { 1.f, 0.f, 0.f, 1.f };

    pDxContext->ClearRenderTargetView(pDxRenderTargetView2D, clearColor);
    pDxContext->ClearDepthStencilView(
      pDxDepthStencilView2D,
      D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL,
      1.0f,
      0
    );

    pDxContext->OMSetRenderTargets(1, &pDxRenderTargetView2D, pDxDepthStencilView2D);

    //CD3D11_VIEWPORT viewPort(0.0f, 0.0f,
    //  (float)width, (float)height);
    //pDxContext->RSSetViewports(1, &viewPort);
    //
    //pDxBatch->Begin();
    //
    //pDxBatch->DrawLine(
    //  VertexPositionColor(XMFLOAT3(-0.5f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f)),
    //  VertexPositionColor(XMFLOAT3(0.5f, -0.5f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f))
    //);
    //
    //pDxBatch->End();

    //pDxContext->RSSetState(pDxStates->CullNone());
    //pDxContext->RSSetState(mpDxRasterizerState);
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

