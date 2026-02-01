#include <windows.h>
#include "cross_boundary_data.h"

// TODO: capture keyboard input from user
// TODO: Parse .otf font file
// TODO: Fix mouse cursor getting stuck on window resize

#pragma pack(push, 1)
struct table_record
{
	u8 tag[4];
	u32 check_sum;
	u32 offset;
	u32 length;
};

struct open_type_font
{
	struct table_directory
	{
		u32 sfnt_version;
		u16 num_tables;
		u16 search_range;
		u16 entry_selector;
		u16 range_shift;
		struct table_record *table_records; // num_tables: amount of table records
	}tbl_dir;
};
#pragma pack(pop)

// TODO: Add macro to switch endiannes of 8 bytes correctly
#define SWITCH_ENDIANNES_16(value) value = ((value >> 8) | (value << 8))
#define SWITCH_ENDIANNES_32(value) value = ((value >> 24) | (value << 24)) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000)

enum keys
{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	COUNT
};

// globals
HDC client_area;
HBITMAP frame_bitmap_handle;
BITMAPINFO bitmap_info;
struct frame_buffer frame_buffer;
u8 main_loop_state = 1;

// NOTE: Signatures must match for hot-loading to work
#define CREATE_FUNCTION_SIGNATURE(name) void __stdcall name(struct frame_buffer *game_buffer, struct migrating_data *game_state)
#define CREATE_FUNCTION_POINTER_SIGNATURE(name) void (__stdcall *name)(struct frame_buffer *game_buffer, struct migrating_data *game_state)

typedef CREATE_FUNCTION_POINTER_SIGNATURE(dll_entry_point);

static CREATE_FUNCTION_SIGNATURE(DllStub)
{
	int *paint_pointer = (int *)frame_buffer.memory;
	
	for(int y = 0; y < frame_buffer.height; y++)
	{
		for(int x = 0; x < frame_buffer.width; x++)
		{
			*paint_pointer++ = 0x00AA00AA; //A.R.G.B
		}
	}
};

// NOTE: DispatchMessage() syscall always eventually calls into our Wndproc
LRESULT CALLBACK Wndproc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	// NOTE: Kernel calls into Wndproc that handles both queued/non-queued messages(is called into from DispatchMessage or some other syscall)
	switch(message)
	{
		case WM_ACTIVATEAPP: OutputDebugStringA("GAINED CONTROL \n");
		case WM_CREATE:
		case WM_PAINT:
		{
			OutputDebugStringA("INVALID_CLIENT_AREA_DETECTED_REPAINTING \n");
		}break;
		case WM_SYSCOMMAND:
		{
			if ((wparam & 0xFFF0) != SC_CLOSE)
			{
				break;
			}
			//fallthrough
		};
		case WM_DESTROY:
		{
			main_loop_state = 0;
			PostQuitMessage(0);
		}
		case WM_QUIT:
		{
			
		}break;
		case WM_SIZE:
		{
			if(frame_buffer.memory)
			{
				DeleteObject(frame_bitmap_handle); //Frees the memory allocated for the frame buffer bitmap
				
				RECT rect = {0};
				GetClientRect(handle, &rect);
				
				frame_buffer.width = rect.right - rect.left;
				frame_buffer.height = rect.bottom - rect.top;
				
				bitmap_info.bmiHeader.biWidth = frame_buffer.width;
				bitmap_info.bmiHeader.biHeight = frame_buffer.height;
				frame_bitmap_handle = CreateDIBSection(client_area, &bitmap_info, 0, &frame_buffer.memory, 0, 0);
			}
		}break;
	}
	return DefWindowProc(handle, message, wparam, lparam);
}

static void *LoadFile(char *filename)
{
	HANDLE font_handle = CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
	DWORD file_size = GetFileSize(font_handle, 0);
	void *mem_buffer = VirtualAlloc(0,  file_size, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE);
	
	if(mem_buffer)
	{
		ReadFile(font_handle, mem_buffer, file_size, 0, 0);		
	}
	
	return mem_buffer;
}

int WINAPI DSAMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WNDCLASSA window_class = {0};
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; 
	window_class.lpfnWndProc = Wndproc;
	window_class.hInstance = hInstance;
	window_class.lpszClassName = "Visualizer";
	window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
	window_class.hCursor = LoadCursor(0, IDC_CROSS);
	window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	if(RegisterClass(&window_class) == 0)
	{
		return 0;
	}
	
	HWND window_handle = CreateWindow(window_class.lpszClassName,
									  "DSA_VISUALIZER",
									  WS_OVERLAPPEDWINDOW,
									  1920, 0,
									  CW_USEDEFAULT, CW_USEDEFAULT,
									  0, 0, hInstance, 0);
	if(window_handle)
	{
		ShowWindow(window_handle, SW_SHOWMAXIMIZED); // Make the application window visible on the screen
		
		HDC hdc_dest = GetDC(window_handle);
		RECT rect = {0};
		
		GetClientRect(window_handle, &rect); // client area: part of the window the user-space app can draw

		frame_buffer.width = rect.right - rect.left;
		frame_buffer.height = rect.bottom - rect.top;
		
		bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
		bitmap_info.bmiHeader.biWidth = frame_buffer.width;
		bitmap_info.bmiHeader.biHeight = frame_buffer.height;
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;
		
		client_area = CreateCompatibleDC(0); // client area of the application window (excluding the menu bar, GetWindowDC gets the DC for the window + menu bar)
		frame_bitmap_handle = CreateDIBSection(client_area, &bitmap_info, DIB_PAL_COLORS, &frame_buffer.memory, 0, 0);
		
		char *dll_file_name = "visualizer.dll";
		char *dll_temp_file_name = "visualizer_temp.dll";
		
		CopyFile(dll_file_name, dll_temp_file_name,0);
		
		MSG msg = {0};
		FILETIME current_write_time = {0};
		FILETIME last_write_time = {0};
		HMODULE dll_module = LoadLibrary(dll_temp_file_name);
		dll_entry_point game_logic = (dll_entry_point)GetProcAddress(dll_module, "VisualizerMain");
		
		u8 user_input[COUNT] = {0};
		char *font_raw_bytes = LoadFile("assets/cyber.otf");
		
		// TODO: Parse open type font file
		{
			struct open_type_font font_data = {0};
			font_data.tbl_dir = *(struct table_directory *)font_raw_bytes;
			
			SWITCH_ENDIANNES_16(font_data.tbl_dir.num_tables);
			SWITCH_ENDIANNES_16(font_data.tbl_dir.search_range);
			SWITCH_ENDIANNES_16(font_data.tbl_dir.entry_selector);
			SWITCH_ENDIANNES_16(font_data.tbl_dir.range_shift);
			SWITCH_ENDIANNES_32(font_data.tbl_dir.sfnt_version);
			
			font_raw_bytes += sizeof(struct table_directory);
		}
		
		LARGE_INTEGER perf_count_last;
		QueryPerformanceCounter(&perf_count_last); // <1us resolution. delta time will be in nanoseconds so we need to convert to milliseconds
		
		LARGE_INTEGER perf_freq;
		QueryPerformanceFrequency(&perf_freq); // Times timestamp counter is refreshed per 1s. Set at boot time
		
		struct migrating_data game_state = {0};
		float frame_limit = 1 / 60.f;
		unsigned __int64 hardware_counter = __rdtsc(); // measures clock cycles
		
		// MAIN LOOP
		while(main_loop_state)
		{
			// hot-loading logic
			{
				HANDLE pic = CreateFile(dll_file_name, GENERIC_READ, 
										(FILE_SHARE_READ | FILE_SHARE_WRITE), 
										0, OPEN_EXISTING, 0, 0);
				
				GetFileTime(pic, 0, 0, &last_write_time); // File must not be locked by any process by this call
				CloseHandle(pic);				
				
				if(CompareFileTime(&current_write_time, &last_write_time)) // rets 0 iff timestamps match
				{
					FreeLibrary(dll_module);
					CopyFile(dll_file_name, dll_temp_file_name,0);
					
					dll_module = LoadLibrary(dll_temp_file_name);
					game_logic = (dll_entry_point)GetProcAddress(dll_module, "VisualizerMain");
					
					if(game_logic == 0)
					{
						game_logic = DllStub;
					}
					
					current_write_time = last_write_time;
				}
			}

			while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) 
			{
				//Note: Loop executes, GetMessage(...) gets called on following iteration, blocks until a message comes.
				//	  To prevent this we have to use PeekMessageA, which returns 0 when no messages are in the message queue 
				TranslateMessage(&msg); // passes msg to kernel code for keyboard translation. If we don't process keyboard input this syscall isn't required
				DispatchMessage(&msg); // calls into kernel code (which at some point calls into Wndproc then continues in kernel code until leave instruction
			}
			
			// Logic can be done in Wndproc but here we skip a spiral callstack of kernel->user->kernel->user->etc..
			// Capture Input
			switch(msg.message)
			{

				case WM_KEYDOWN:
				{
					switch(msg.wParam)
					{
						case VK_LEFT: user_input[LEFT] = 1; break;
						case VK_RIGHT: user_input[RIGHT] = 1; break;
						case VK_UP: user_input[UP] = 1; break;
						case VK_DOWN: user_input[DOWN] = 1; break;
					}
				}break;
				case WM_KEYUP:
				{
				}break;
				
				default: DefWindowProc(window_handle, msg.message, msg.wParam, msg.lParam);
			}
			
			game_logic(&frame_buffer, &game_state); // call to Platform Independent Code
			SelectObject(client_area, frame_bitmap_handle); // Place bitmap into device context
			BitBlt(hdc_dest, 0, 0, frame_buffer.width, frame_buffer.height, client_area,0, 0, SRCCOPY); // Bitmap block transfer
			
			// Time Measurements
			{
				LARGE_INTEGER perf_count_current;
				QueryPerformanceCounter(&perf_count_current);
				
				s64 delta_count = perf_count_current.QuadPart - perf_count_last.QuadPart;
				game_state.frame_time = (float)delta_count / perf_freq.QuadPart; 
				perf_count_last.QuadPart = perf_count_current.QuadPart;
				
				while((game_state.frame_time * milliseconds) < (frame_limit * milliseconds))// convert unit sec->millisec
				{
					LARGE_INTEGER perf_count_current;
					QueryPerformanceCounter(&perf_count_current);
					
					delta_count = perf_count_current.QuadPart - perf_count_last.QuadPart;
					game_state.frame_time = (float)delta_count / perf_freq.QuadPart;
				}
				
				unsigned __int64 hardware_counter_stamp = __rdtsc();
				
				game_state.clock_cycles = (hardware_counter_stamp - hardware_counter) / Mhz; // change hz -> mhz
				hardware_counter = hardware_counter_stamp;
			}
		}
	}
	return 0;
}

// Variables referenced by the compiler that are defined in runtime implementations
int _fltused;