#include <Windows.h>

#define local_persist static
#define internal static 
#define global_variable static

// Try without CALLBACK keyword
LRESULT CALLBACK
MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_CREATE:
        {
            ;
        } break;
        case WM_SIZE:
        { 
            OutputDebugString(L"WM_SIZE\n");
        } break;

        case WM_DESTROY:
        {
            OutputDebugString(L"WM_DESTROY\n");
        } break;

        case WM_CLOSE:
        {
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

            local_persist DWORD Operation = WHITENESS;

            if (Operation == WHITENESS) { Operation = BLACKNESS; }
            else { Operation = WHITENESS; }

            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            EndPaint(Window, &Paint);

            SetPixel(DeviceContext, 100, 100, RGB(255, 0, 0));
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
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = L"HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
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

        if (WindowHandle)
        {
            MSG Message;
            while (BOOL MessageResult = GetMessage(&Message, 0, 0, 0) > 0)
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
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
