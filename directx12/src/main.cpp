#include <windows.h>
#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <stdexcept>
// #include "tinygltf/tiny_gltf.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT2 texCoord;
    XMFLOAT3 normal;
};

struct ConstantBuffer {
    XMMATRIX worldViewProj;
};

// Global variables
const int WIDTH = 1280;
const int HEIGHT = 720;
UINT frameIndex = 0;
ComPtr<ID3D12Fence> fence;
HANDLE fenceEvent;
UINT64 fenceValue = 0;
UINT64 vertexCount = 3; // TODO
ComPtr<ID3D12Device> device;
ComPtr<ID3D12CommandQueue> commandQueue;
ComPtr<IDXGISwapChain3> swapChain;
ComPtr<ID3D12DescriptorHeap> rtvHeap;
ComPtr<ID3D12Resource> renderTargets[2];
ComPtr<ID3D12CommandAllocator> commandAllocator;
ComPtr<ID3D12GraphicsCommandList> commandList;
ComPtr<ID3D12PipelineState> pipelineState;
ComPtr<ID3D12RootSignature> rootSignature;
UINT rtvDescriptorSize;
ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
ComPtr<ID3D12Resource> constantBuffer;
UINT8* constantBufferData;
ComPtr<ID3D12Resource> textureBuffer;
ComPtr<ID3D12DescriptorHeap> srvHeap;

// Forward declarations
void InitD3D(HWND hwnd);
// void LoadGLTFModel(const char* filename);
void CreatePipelineState();
void Render();
void WaitForGpu();
void CleanupD3D();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DX12GLTFRenderer";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        0, L"DX12GLTFRenderer", L"GLTF Model Viewer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        WIDTH, HEIGHT, nullptr, nullptr, hInstance, nullptr
    );

    InitD3D(hwnd);
    // LoadGLTFModel("model.gltf");
    CreatePipelineState();

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Render();
        }
    }

    WaitForGpu();
    CleanupD3D();
    return 0;
}

void InitD3D(HWND hwnd) {
    UINT dxgiFactoryFlags = 0;
    
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory6> factory;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

    // Create device
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &hardwareAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) break;
    }

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = WIDTH;
    swapChainDesc.Height = HEIGHT;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    factory->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );
    swapChain1.As(&swapChain);

    // Create descriptor heaps
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create frame resources
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 2; i++) {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

    // Create command list
    device->CreateCommandList(
        0,                                  // Node mask (0 for single GPU)
        D3D12_COMMAND_LIST_TYPE_DIRECT,     // Command list type
        commandAllocator.Get(),             // Command allocator
        pipelineState.Get(),                // Initial pipeline state (optional, can be nullptr)
        IID_PPV_ARGS(&commandList)
    );

    // Initially close the command list
    commandList->Close();

    // initialize srvHeap
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1; // For texture
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(
        &srvHeapDesc,
        IID_PPV_ARGS(&srvHeap)
    );

    // Create fence
    device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence)
    );

    fenceValue = 1;

    // Create event handle for fence synchronization
    fenceEvent = CreateEvent(
        nullptr,    // default security attributes
        FALSE,      // auto-reset event
        FALSE,      // initially non-signaled
        nullptr     // unnamed event
    );
}

/*void LoadGLTFModel(const char* filename) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!ret) {
        // Handle error
        return;
    }

    // For simplicity, we'll assume the model has one mesh with one primitive
    const tinygltf::Mesh& mesh = model.meshes[0];
    const tinygltf::Primitive& primitive = mesh.primitives[0];

    // Get vertex data
    std::vector<Vertex> vertices;
    const float* positions = reinterpret_cast<const float*>(&model.bufferViews[
        model.accessors[primitive.attributes.find("POSITION")->second].bufferView].data[0]);
    const float* texCoords = reinterpret_cast<const float*>(&model.bufferViews[
        model.accessors[primitive.attributes.find("TEXCOORD_0")->second].bufferView].data[0]);
    const float* normals = reinterpret_cast<const float*>(&model.bufferViews[
        model.accessors[primitive.attributes.find("NORMAL")->second].bufferView].data[0]);

    size_t vertexCount = model.accessors[primitive.attributes.find("POSITION")->second].count;
    for (size_t i = 0; i < vertexCount; i++) {
        Vertex vertex;
        vertex.position = XMFLOAT3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
        vertex.texCoord = XMFLOAT2(texCoords[i * 2], texCoords[i * 2 + 1]);
        vertex.normal = XMFLOAT3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
        vertices.push_back(vertex);
    }

    // Create vertex buffer
    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    
    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)
    );

    // Copy vertex data to the buffer
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
    vertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexBufferSize;

    // Load texture
    if (model.textures.size() > 0) {
        const tinygltf::Texture& texture = model.textures[0];
        const tinygltf::Image& image = model.images[texture.source];
        
        // Create texture resource and upload texture data
        // (Implementation details omitted for brevity)
    }
}*/

void CreatePipelineState() {
    // Create a static sampler
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Create root signature
    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstantBufferView(0); // For WorldViewProj matrix
    
    CD3DX12_DESCRIPTOR_RANGE descRange;
    descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[1].InitAsDescriptorTable(1, &descRange); // For texture

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    HRESULT hr;
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> signatureError1;

    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &signatureError1);

    if (FAILED(hr)) {
        if (signatureError1) {
            OutputDebugStringA(reinterpret_cast<char*>(signatureError1->GetBufferPointer()));
        }

        // Throw an exception or handle the error
        throw std::runtime_error("Root signature serialization failed");
    }

    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

    if (FAILED(hr)) {
        throw std::runtime_error("Root signature creation failed");
    }

    // Create pipeline state
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    // Compile shaders
    ComPtr<ID3DBlob> vertexShaderError;

    hr = D3DCompileFromFile(L"shaders/shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, &vertexShaderError);

    if (FAILED(hr)) {
        if (vertexShaderError) {
            OutputDebugStringA(reinterpret_cast<char*>(vertexShaderError->GetBufferPointer()));
        }

        // Throw an exception or handle the error
        throw std::runtime_error("Vertex shader compilation failed");
    }

    ComPtr<ID3DBlob> pixelShaderError;
    hr = D3DCompileFromFile(L"shaders/shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, &pixelShaderError);

    if (FAILED(hr)) {
        if (pixelShaderError) {
            OutputDebugStringA(reinterpret_cast<char*>(pixelShaderError->GetBufferPointer()));
        }

        // Throw an exception or handle the error
        throw std::runtime_error("Pixel shader compilation failed");
    }

    // Define input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Create pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

    if (FAILED(hr)) {
        // Get detailed error information
        //if (SUCCEEDED(hr = pipelineState.As(&device))) {
        //    // Additional error handling if needed
        //}
        
        throw std::runtime_error("Failed to create pipeline state");
    }

    // initialize constantBufferData
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer)
    );

    // Map the buffer and keep it mapped
    CD3DX12_RANGE readRange(0, 0);
    constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferData));
}

void Render() {
    // Reset command allocator and command list
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), pipelineState.Get());

    // Set necessary state
    auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT));
    auto scissorRects = CD3DX12_RECT(0, 0, WIDTH, HEIGHT);
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRects);

    // Update constant buffer with new transform matrices
    XMMATRIX world = XMMatrixRotationY(static_cast<float>(GetTickCount64()) / 1000.0f);
    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 3.0f, -5.0f, 1.0f),
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f)
    );
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, WIDTH / static_cast<float>(HEIGHT), 0.1f, 100.0f);
    XMMATRIX worldViewProjection = XMMatrixMultiply(XMMatrixMultiply(world, view), projection);

    ConstantBuffer cb;
    cb.worldViewProj = XMMatrixTranspose(worldViewProjection);
    memcpy(constantBufferData, &cb, sizeof(cb));

    // Indicate that the back buffer will be used as a render target
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        frameIndex,
        rtvDescriptorSize
    );

    // Clear the render target
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // Bind texture
    commandList->SetGraphicsRootDescriptorTable(1, srvHeap->GetGPUDescriptorHandleForHeapStart());

    // Draw the model
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->DrawInstanced(vertexCount, 1, 0, 0);

    // Indicate that the back buffer will now be used to present
    auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    commandList->ResourceBarrier(1, &barrier2);

    // Close the command list and execute it
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame
    swapChain->Present(1, 0);

    // Wait for GPU to finish
    WaitForGpu();
}

void WaitForGpu() {
    // Signal and increment the fence value
    commandQueue->Signal(fence.Get(), fenceValue);
    fenceValue++;

    // Wait until the previous frame is finished
    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void CleanupD3D() {
    WaitForGpu();
    CloseHandle(fenceEvent);
}
