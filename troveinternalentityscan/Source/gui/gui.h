#pragma once

//gui.h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <iostream>

#include "../../minhook/MinHook.h"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"
#include "../../imgui/imgui_internal.h" //this is for drawing shapes

#include "../functions.h"



bool get_present_pointer();

LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags);

bool SetupD3DHook();

extern bool show_menu;
extern bool silentAimCb;
extern float version;
extern float maxRangeSlider;
extern Entity closestEntity;
extern float closestDist;



//externs
extern HINSTANCE dll_handle;
extern ID3D11Device* p_device;
extern ID3D11DeviceContext* p_context;
extern ID3D11RenderTargetView* mainRenderTargetView;
extern HWND window;
