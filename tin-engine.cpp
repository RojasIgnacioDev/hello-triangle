#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <chrono>

#include <tinengine.h>

// screen resolutions
#define SCREEN_WIDTH 1366
#define SCREEN_HEIGHT 768

// global declarations
IDXGISwapChain* swapchain;          // the pointer to the swap chain interface (used with a backbuffer to avoid screen tearing)
ID3D11Device* dev;                  // the pointer to our Direct3D device interface (it is a virtual representation of the video adapter
ID3D11DeviceContext* devcon;        // the pointer to our Direct3D device context (manages the GPU and the rendering pipeline)
ID3D11RenderTargetView* backbuffer; // the back buffer used in the swap chain
ID3D11VertexShader* pVS;            // vertex shader
ID3D11PixelShader* pPS;             // pixel shader
ID3D11Buffer* pVBuffer;             // vertex buffer 
ID3D11InputLayout* pInputLayout;    // input layout for rendering

// function prototypes
void InitD3D(HWND hWnd);     // sets up and initializes Direct3D
void CleanD3D(void);         // closes Direct3D and releases memory
void InitPipeline(void);
void InitInputLayout(void);
void InitGraphics(void);

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Initializes Direct3D
void InitD3D(HWND hWnd) {
    /* Direct3D initialization */
    // struct that holds information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;              // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // how swap chain is to be used
    scd.OutputWindow = hWnd;                            // the window to be used
    scd.SampleDesc.Count = 4;                           // how many multisamples (antialiasing
    scd.Windowed = TRUE;                                // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(
        NULL,// indicates a graphics adapter, NULL for using the dafault adapter
        D3D_DRIVER_TYPE_HARDWARE,//	The obviously best choice. This uses the advanced GPU hardware for rendering.
        NULL,
        NULL,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &dev,
        NULL,
        &devcon);

    /* Set the render target */
    // get backbuffer address
    ID3D11Texture2D* pBackBuffer;

    swapchain->GetBuffer(
        0,                         // the number of the back buffer to get
        __uuidof(ID3D11Texture2D), // the id number identifying the ID3D11Texture2D COM object
        (LPVOID*)&pBackBuffer      // set a void pointer because we could ask later for different COM objects
    );

    // use the back buffer adress to create the render target
    dev->CreateRenderTargetView(
        pBackBuffer, // pointer to the texture pointer
        NULL,        // struct that describes the render target (no needed)
        &backbuffer
    );

    // frees the memory and closes all the threads used by a COM object
    // (we are not deleting the back buffer, it only closes the texture we used to access it
    pBackBuffer->Release();

    // sets the render target, more specifically, it sets multiple render targets
    devcon->OMSetRenderTargets(
        1,           // number of targets to render (can be more)
        &backbuffer, // pointer to a list of render target view objects (one for this case)
        NULL         // the documentation says that this parameter is for more advanced stuff
    );

    /* Set the viewport */
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    devcon->RSSetViewports(1, &viewport);

    // initialize pipeline and graphics
    InitPipeline();
    InitGraphics();
}

// Cleans up Direct3D and COM objects
void CleanD3D() {
    // turn off full-screen
    swapchain->SetFullscreenState(FALSE, NULL);

    // releases all existing COM object
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();

    // pVS and pPS are COM objects so they must be released

    pVS->Release();    // an exception is throw here because InitPipeline() was never called, therefore pVS was never initialized
    pPS->Release();    // an exception is throw here because InitPipeline() was never called, therefore pPS was never initialized
}

// Initializes the rendering pipeline
void InitPipeline() {
    // load and compile the two shaders
    ID3D10Blob* VS;
    ID3D10Blob* PS;
    D3DX11CompileFromFile(
        L"shaders.shader", // file containing the shaders code
        0, 0,
        "VShader", // entry function name of the shader
        "vs_4_0",
        0, 0, 0,
        &VS, // blob object containing the compiled shader
        0, 0
    );
    D3DX11CompileFromFile(
        L"shaders.shader", // file containing the shaders code
        0, 0,
        "PShader", // entry function name of the shader
        "ps_4_0",
        0, 0, 0,
        &PS, // blob object containing the compiled shader
        0, 0
    );

    // encapsulate both shaders into shader objects
    dev->CreateVertexShader(
        VS->GetBufferPointer(), // pointer to the compiled shader blob object
        VS->GetBufferSize(),    // the size of the file data
        NULL,                   // ADVANCED
        &pVS                    // address to the shader object
    );
    dev->CreatePixelShader(
        PS->GetBufferPointer(), // pointer to the compiled shader blob object
        PS->GetBufferSize(),    // the size of the file data
        NULL,                   // ADVANCED
        &pPS                    // address to the shader object
    );

    // set both shaders to be active
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // create the input layout object
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {
            "POSITION",                  // semantic as a string that tells the GPU what the value is used for
            0,                           // semantic index. To avoid confusion, we would have each property have a different number here.
            DXGI_FORMAT_R32G32B32_FLOAT, // data format, this one adapts to the vertices
            0,                           // input slot. Advanced
            0,                           // how many bytes into the struct the element is
            D3D11_INPUT_PER_VERTEX_DATA, // what the element is used as
            0                            // not used with D3D11_INPUT_PER_VERTEX_DATA, so have it as 0
        },
        {
            "COLOR",                        // semantic as a string that tells the GPU what the value is used for
            0,                              // semantic index. To avoid confusion, we would have each property ha            
            DXGI_FORMAT_R32G32B32A32_FLOAT, // data format, this one adapts to the vertices
            0,                              // input slot. Advanced
            D3D11_APPEND_ALIGNED_ELEMENT,   // how many bytes into the struct the element is
            D3D11_INPUT_PER_VERTEX_DATA,    // what the element is used as
            0}                              // not used with D3D11_INPUT_PER_VERTEX_DATA, so have it as 0
    };
    dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pInputLayout);
    devcon->IASetInputLayout(pInputLayout);
}

void InitInputLayout() {

}

void InitGraphics() {
    float x = 0.7f;
    float y = 0.4f;
    // Vertices 
    Vertex vertices[] = {
        Vertex{0.0f, x, 0.0f, Color(1.0f, 0.0f, 0.0f, 1.0f)},
        Vertex{y, -y, 0.0f, Color(0.0f, 1.0f, 0.0f, 1.0f)},
        Vertex{-y, -y, 0.0f, Color(0.0f, 0.0f, 1.0f, 1.0f)},
    };

    /* create vertex buffer COM object */
    // vertex buffer description
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

    bd.Usage = D3D11_USAGE_DYNAMIC;             // write access by CPU and GPU
    bd.ByteWidth = sizeof(vertices)  ;          // size of the vertices
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;    // use as vertex buffer
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // allow CPU to write in the buffer

    dev->CreateBuffer(&bd, NULL, &pVBuffer);

    // map the vertex buffer 
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(
        ms.pData,        // destination 
        vertices,        // source
        sizeof(vertices) // number of bytes to copy
    );
    devcon->Unmap(pVBuffer, NULL);
}

UINT GetTimeInMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

UINT GetTimeInSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

void RenderFrame() {
    // do cool 3D rendering stuff here

    // draw the vertices
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    devcon->IASetVertexBuffers(
        0, // advanced
        1, // how many buffers we are setting
        &pVBuffer, // pointer to an array of vertex buffers
        &stride, // the size of a single vertex in each vertex buffer
        &offset // the number of bytes into the vertex buffer we should start rendering from
    );

    // select which primitive type we are using
    devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // draw the vertex buffer to the back buffer
    devcon->Draw(3, 0);

    // stop doing cool 3D rendering stuff here

    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch (message)
    {
        // this message is read when the window is closed
    case WM_DESTROY:
    {
        // close the application entirely
        PostQuitMessage(0);
        return 0;
    } break;
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// The entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass1";

    // register the window class
    RegisterClassEx(&wc);

    // to adjust window client area
    RECT wr = { 0,0,800,600 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
        L"WindowClass1",    // name of the window class
        L"The best game engine ever",   // title of the window
        WS_OVERLAPPEDWINDOW,    // window style
        0,    // x position of the window
        0,    // y position of the window
        SCREEN_WIDTH,    // width of the window
        SCREEN_HEIGHT,    // height of the window            
        NULL,    // we have no parent window, NULL
        NULL,    // we aren't using menus, NULL
        hInstance,    // application handle
        NULL);    // used with multiple windows, NULL

    ShowWindow(hWnd, nCmdShow);

    // initialize Direct3D 
    InitD3D(hWnd);

    // enter main loop  

    // struct that holds Windows messages
    MSG msg;

    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            // translate keystrokes into the right format
            TranslateMessage(&msg);

            // send message to WindowProc method
            DispatchMessage(&msg);

            // check to see if it's time to quit
            if (msg.message == WM_QUIT)
                break;
        }
        else {
            // Run game code here
        }

        RenderFrame();
    }

    CleanD3D();

    return msg.wParam;
}