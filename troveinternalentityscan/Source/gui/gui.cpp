
#include "gui.h"

#pragma comment(lib, "MinHook/libMinHook.x86.lib")

#pragma comment(lib, "d3d11.lib")

//mostly skidded from "CasualGamer" on youtube, thanks for that babe xoxo
//im 2 stupid for this directx hooking nonsense </3

HINSTANCE dll_handle;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;

bool get_present_pointer()
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = GetForegroundWindow();
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE; 
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	IDXGISwapChain* swap_chain;
	ID3D11Device* device;

	const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		feature_levels,
		2,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		nullptr,
		nullptr) == S_OK)
	{
		void** p_vtable = *reinterpret_cast<void***>(swap_chain);
		swap_chain->Release();
		device->Release();
		//context->Release();
		p_present_target = (present)p_vtable[8];
		return true;
	}
	return false;
}

WNDPROC oWndProc;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	//if shit is exploding, start looking from here

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

//menu vars
bool show_menu = true;
bool silentAimCb = false;
bool bossPriority = true;
float version = 0.2f;
float maxRangeSlider = 40.0f;
Entity closestEntity = {};
float closestDist = FLT_MAX;




bool init = false;
HWND window = NULL;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;
static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
	if (!init) {
		if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device)))
		{
			p_device->GetImmediateContext(&p_context);
			DXGI_SWAP_CHAIN_DESC sd;
			p_swap_chain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			//init imgui
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange; //enabling this will stop imgui from managing the cursor instead of the game
			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX11_Init(p_device, p_context);
			init = true;
		}
		else
			return p_present(p_swap_chain, sync_interval, flags);
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::GetStyle().WindowRounding = 0.0f;

	ImGui::NewFrame();
	//draw whatever we want between newframe and endframe
	ImGui::ShowDemoWindow();


	if (show_menu) {
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
		ImGui::Begin("T.I.M v 0.2", &show_menu, window_flags);
		ImGui::SetWindowSize(ImVec2(300, 300), ImGuiCond_Always);
		ImGui::Text("Combat Options:");
		ImGui::Checkbox("Silent Aim", &silentAimCb);
		ImGui::Checkbox("Prioritise High HP", &bossPriority);
		ImGui::SliderFloat("Max Range", &maxRangeSlider, 1, 40);
		ImGui::End();

		
	}

	if (silentAimCb) {
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		int corner = 0;
		if (corner != -1)
		{
			const float PAD = 100.0f;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
			window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
			window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
			window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::SetNextWindowBgAlpha(0.15f); // Transparent background
		if (ImGui::Begin("Silent Aim Targeting", &silentAimCb, window_flags))
		{
			ImGui::Text("Silent Aim Targeting");
			ImGui::Separator();
			ImGui::Text("Current Target: %s", closestEntity.name.c_str());
			ImGui::Text("Level: %i", closestEntity.level);
			ImGui::Text("Health: %.0f", closestEntity.health);
			ImGui::Text("Dist: %.1f", closestDist);
			
		}
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	p_context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return p_present(p_swap_chain, sync_interval, flags);
}

bool SetupD3DHook() {
	if (!get_present_pointer())
	{
		return false;
	}

	MH_STATUS status = MH_Initialize();
	if (status != MH_OK)
	{
		return false;
	}

	if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
		return false;
	}

	if (MH_EnableHook(p_present_target) != MH_OK) {
		return false;
	}
	return true;
}



