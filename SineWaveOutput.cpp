#include <math.h>
#include <Windows.h>
#include <stdint.h>
#include <dsound.h>

#define global static
#define PI 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;

// macro for creating a function signature that will be used for a function pointer
#define CREATE_DIRECT_SOUND(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef CREATE_DIRECT_SOUND(CreateDirectSound);

struct Dimension
{
	int width;
	int height;
};

struct OffscreenBuffer
{
	void* memory;
	BITMAPINFO bitmapInfo;
	int height;
	int width;
	int pitch;
};

struct SoundOutput
{
	int samplesPerSecond;
	int hertz;
	int16 volume;
	uint32 sampleIndex;
	int samplesPerCycle;
	int bytesPerSample;
	int soundBufferSize;
	float sinePosition; // the sine position
	int latencySampleCount;
};

global bool globalRunning;
global OffscreenBuffer buffer;
global LPDIRECTSOUNDBUFFER globalSoundBuffer; // secondary buffer that we write sound into

Dimension getWindowDimension(HWND window)
{
	Dimension windowDimensions;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	windowDimensions.width = clientRect.right - clientRect.left;
	windowDimensions.height = clientRect.bottom - clientRect.top;

	return windowDimensions;
}

void initDirectSound(HWND window, uint32 samplesPerSecond, uint32 bufferSize)
{
	HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
	if (dSoundLibrary)
	{
		CreateDirectSound * createDirectSound = (CreateDirectSound *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (createDirectSound && 
			SUCCEEDED(createDirectSound(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if ( SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)) )
			{				
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; // this is the primary buffer

				// get the primary buffer so that we can set the wave format.
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					HRESULT error = primaryBuffer->SetFormat(&waveFormat);
					if (SUCCEEDED(error))
					{
						//TODO: pop up window
						OutputDebugStringA("Primary buffer format was set.\n");
					}
					else
					{

					}
				}
				else
				{

				}
			}
			else
			{

			}

			DSBUFFERDESC bufferDesc2nd = {};
			bufferDesc2nd.dwSize = sizeof bufferDesc2nd;
			bufferDesc2nd.dwFlags = 0;
			bufferDesc2nd.dwBufferBytes = bufferSize;
			bufferDesc2nd.lpwfxFormat = &waveFormat;
			HRESULT error = directSound->CreateSoundBuffer(&bufferDesc2nd, &globalSoundBuffer, 0);
			if (SUCCEEDED(error))
			{
				//TODO: pop up window
				OutputDebugStringA("Secondary buffer was set.\n");
			}
		}
	}
}



// Allocates the back buffer.
void initBuffer(OffscreenBuffer* buffer, int height, int width)
{
	// free the memory if there is any
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	// set the width and height of the buffer
	buffer->height = height;
	buffer->width = width;
	int bytesPerPixel = 4;

	// fill out the bitmap info
	buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
	buffer->bitmapInfo.bmiHeader.biWidth = buffer->width;
	buffer->bitmapInfo.bmiHeader.biHeight = -buffer->height;
	buffer->bitmapInfo.bmiHeader.biPlanes = 1;
	buffer->bitmapInfo.bmiHeader.biBitCount = 32;
	buffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;

	// set aside memory for the buffer's pixel data
	int bitmapMemorySize = buffer->width * buffer->height * bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	buffer->pitch = buffer->width * bytesPerPixel;
}

void renderGradient(OffscreenBuffer *buffer, int r, int g, int b)
{
	int width = buffer->width;
	int height = buffer->height;

	uint8 *row = (uint8 *)buffer->memory;
	for (int y = 0; y < height; y++)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < width; x++)
		{
			// r g b xx
			// b g r xx
			uint8 blue = b + x;
			uint8 green = g + y;

			uint32 color = blue | (green << 8) | (r << 16);
			*pixel++ = color;
		}

		row += buffer->pitch;
	}

}

void drawBufferToScreen(HDC deviceContext, OffscreenBuffer* buffer, int windowWidth, int windowHeight)
{
	StretchDIBits(
		deviceContext,
		0,
		0,
		windowWidth,
		windowHeight,
		0,
		0,
		buffer->width,
		buffer->height,
		buffer->memory,
		&buffer->bitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK MessageHandler(HWND window,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{

	case WM_CLOSE:
	{
		globalRunning = false;
	}break;

	case WM_DESTROY:
	{
		globalRunning = false;
	}break;

	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		Dimension windowDim = getWindowDimension(window);
		drawBufferToScreen(deviceContext, &buffer, windowDim.width, windowDim.height);
		EndPaint(window, &paint);

	}break;

	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}break;
	}

	return result;
}

void fillSoundBuffer(SoundOutput *soundOutput, DWORD lockByte, DWORD writeBytes)
{
	VOID * region1;
	DWORD region1Size;
	VOID * region2;
	DWORD region2Size;

	if (SUCCEEDED(globalSoundBuffer->Lock(lockByte, writeBytes,
											&region1, &region1Size,
											&region2, &region2Size,
											0)))
	{
		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample; //number of samples in region 1
		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;

		int16* outputSample = (int16*)region1;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			float sinValue = sinf(soundOutput->sinePosition);
			int16 sampleValue = (int16)(sinValue * soundOutput->volume);
			*outputSample = sampleValue;
			++outputSample;
			*outputSample = sampleValue;
			++outputSample;

			soundOutput->sinePosition += 2.0f * PI / (float)soundOutput->samplesPerCycle;
			soundOutput->sampleIndex++;
		}

		outputSample = (int16*) region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			float sinValue = sinf(soundOutput->sinePosition);
			int16 sampleValue = (int16)(sinValue * soundOutput->volume);
			*outputSample = sampleValue;
			++outputSample;
			*outputSample = sampleValue;
			++outputSample;

			soundOutput->sinePosition += 2.0f * PI / (float)soundOutput->samplesPerCycle;
			soundOutput->sampleIndex++;
		}

		globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

int CALLBACK
WinMain(HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     cmdLine,
	int       showCode)
{
	WNDCLASS windowClass = {};

	// makes the entire client rect part of the window redraw whenever
	// moved horizontally or veritcally
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = MessageHandler;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"Gradient Renderer";

	if (RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(
			0,
			windowClass.lpszClassName,
			L"GradientRenderer",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, //window with border and title. also shows up visible by default
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0);

		if (window)
		{
			HDC deviceContext = GetDC(window);
			uint8 xOffset = 0;
			uint8 yOffset = 0;

			initBuffer(&buffer, 1280, 720);
			// sound code
			SoundOutput soundOutput = {};
			soundOutput.samplesPerSecond = 48000;
			soundOutput.hertz = 256;
			soundOutput.volume = 3000;
			soundOutput.samplesPerCycle = soundOutput.samplesPerSecond / soundOutput.hertz;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.soundBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

			initDirectSound(window, soundOutput.samplesPerSecond, soundOutput.soundBufferSize);
			fillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
			globalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			globalRunning = true;
			while (globalRunning)
			{
				MSG message;
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						globalRunning = false;
					}

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				renderGradient(&buffer, 255, xOffset, yOffset);
				xOffset++;
				yOffset++;

				
				DWORD playCursor;
				DWORD writeCursor;

				if (SUCCEEDED(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					DWORD lockByte = (soundOutput.sampleIndex * soundOutput.bytesPerSample) % soundOutput.soundBufferSize;
					DWORD TargetCursor =
						((playCursor +
							(soundOutput.latencySampleCount*soundOutput.bytesPerSample)) %
							soundOutput.soundBufferSize);
					DWORD BytesToWrite;
					// TODO(casey): Change this to using a lower latency offset from the playcursor
					// when we actually start having sound effects.
					if (lockByte > TargetCursor)
					{
						BytesToWrite = (soundOutput.soundBufferSize - lockByte);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - lockByte;
					}

					fillSoundBuffer(&soundOutput, lockByte, BytesToWrite);
				}

				Dimension windowDimension = getWindowDimension(window);
				drawBufferToScreen(deviceContext, &buffer, windowDimension.width, windowDimension.height);
			}


		}
	}

	return 0;
}