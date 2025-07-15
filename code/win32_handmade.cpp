#include <Windows.h>
#include <stdint.h>

#define local_persist static
#define internal static 
#define global_variable static

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    int Width;
    int Height;
    int Pitch;
    void *Memory;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;  
};

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

//TODO: This is a global for now.
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    // Byte Inversion. Meaning -> uint32_t PIXEL = [ RR GG BB 00 ] IN MEMORY LOOKS LIKE THIS [00 BB GG RR]
    uint8_t *Row = (uint8_t *)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for (int X = 0; X < Buffer.Width; ++X)
        {
            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset); 

            // Memory:   XX BB GG RR
            // Register: RR GG [ BB ] XX
            //               <- shift 8 bits to left to get green
            *Pixel++ = (uint32_t)((Green << 8) | Blue); 
        }

        Row += Buffer.Pitch;
    }
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{    
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);   
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;

    // MINUS HEIGHT IS FOR READING PIXELS FROM TOP TO BOTTOM AND NOT OTHERWISE
    // Read here: https://learn.microsoft.com/ru-ru/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; 
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32; // 4 bytes instead of 3 (1R, 1G, 1B + 1 free byte for ALIGNMENT)
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Width*Height)*Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * Buffer->BytesPerPixel;
    // TODO: Probably clear this to black ??
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  */
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory, &Buffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
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
            
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);

            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, X, Y, Width, Height);
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

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = L"HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
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

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);

                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
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
