#include <Windows.h>
#include <stdint.h>

#define local_persist static
#define internal static 
#define global_variable static

//TODO: This is a global for now.
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable void *BitmapMemory;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirdGradient(int XOffset, int YOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;

    // Byte Inversion. Meaning -> uint32_t PIXEL = [ RR GG BB 00 ] IN MEMORY LOOKS LIKE THIS [00 BB GG RR]
    int Pitch = Width*BytesPerPixel;
    uint8_t *Row = (uint8_t*)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint32_t *Pixel = (uint32_t*)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset); 

            // Memory:   XX BB GG RR
            // Register: RR GG [ BB ] XX
            //               <- shift 8 bits to left to get green
            *Pixel++ = (uint32_t)((Green << 8) | Blue); 
        }

        Row += Pitch;
    }
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height)
{
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;

    StretchDIBits(DeviceContext,
                /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                */
                  0, 0, BitmapWidth, BitmapHeight,
                  0, 0, WindowWidth, WindowHeight,
                  BitmapMemory, &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{    
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);   
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;

    // MINUS HEIGHT IS FOR READING PIXELS FROM TOP TO BOTTOM AND NOT OTHERWISE
    // Read here: https://learn.microsoft.com/ru-ru/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight; 
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32; // 4 bytes instead of 3 (1R, 1G, 1B + 1 free byte for ALIGNMENT)
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Width*Height)*BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: Probably clear this to black ??
}

// Try without CALLBACK keyword
LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_CREATE:
        {
        } break;

        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this with a message to the user?
            Running = false;
            OutputDebugString(L"WM_DESTROY\n");
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this as an error - recreate window?
            Running = false;
            OutputDebugString(L"WM_CLOSE\n");
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugString(L"WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
            //OutputDebugString(L"defualt\n");
        } break;
    }

    return(Result);
}

int CALLBACK
wWinMain(HINSTANCE Instance, 
         HINSTANCE PrevInstance, 
         PWSTR CommandLine, 
         int ShowCode)
{
    WNDCLASS WindowClass = {};

    // TODO: Check if those 3 flags still matter.
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = L"HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(
            0,
            L"HandmadeHeroWindowClass",
            L"Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

        if (Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            Running = true;
            while (Running)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderWeirdGradient(XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;
            }
        }
        else
        {
            //TODO: Logging
        }
    }
    else 
    {
        //TODO: Logging
    }
    return(0);
}
