
#include "gui.h"

#pragma comment(lib, "MinHook/libMinHook.x86.lib")

#pragma comment(lib, "d3d11.lib")

HINSTANCE dll_handle;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;

bool get_present_pointer()
{
	DXGI_SWAP_CHAIN_DESC sd; //struct
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2; // 2 = triple buffering
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //display format, this one works for most gpus but selecting one thats valid is fine
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this is the only valid buffer useage for my usecase
	sd.OutputWindow = GetForegroundWindow(); //game window, getforegroundwindow should work fine
	sd.SampleDesc.Count = 1; //multisampling
	sd.Windowed = TRUE; //honestly idfk, it works
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; //discard lets the display driver choose the most efficient way 2 do whatever

	IDXGISwapChain* swap_chain;
	ID3D11Device* device;

	const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(
		NULL, //null = pointer to default video adapter
		D3D_DRIVER_TYPE_HARDWARE, //type hardware = let gpu handle selecting the type (could also use null)
		NULL, //software
		0, //dont need any
		feature_levels, //the array declared just before this, could also just be null
		2, //there are 2 levels so 2, if levels is null this is 0
		D3D11_SDK_VERSION, //documentation says to do this
		&sd, //sd = swapchain description
		&swap_chain, //pointers to store the created swapchain + device
		&device,
		nullptr,
		nullptr) == S_OK) //returns s_ok if its all good
	{
		void** p_vtable = *reinterpret_cast<void***>(swap_chain); //swap chain var is a pointer to the swap chain
		//vtable pointer is at the very beginning of the swap chain
		//to get the vtable pointer we dereference once,
		swap_chain->Release();
		device->Release();
		//context->Release();
		p_present_target = (present)p_vtable[8]; //by accessing an entry to the array we by extension dereference a 2nd time, this location is the pointer to the function
		//the present method is the 9th method in the function so by accessing the 8th thing we get present
		//note: we defined the typedef of the function we're grabbing up top
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
float version = 0.2f;
float maxRangeSlider = 40.0f;
Entity closestEntity = {};
float closestDist = FLT_MAX;




bool init = false;
HWND window = NULL;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;
static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) { //this is the detour version of the function that minhook will run
	if (!init) {
		if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device)))
		{
			p_device->GetImmediateContext(&p_context);

			// Get HWND to the current window of target game
			DXGI_SWAP_CHAIN_DESC sd;
			p_swap_chain->GetDesc(&sd);
			window = sd.OutputWindow;

			// location in mem where imgui is rendered to
			ID3D11Texture2D* pBackBuffer;
			p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			//create a render target pointing to the back buffer
			p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			// this release doesnt destroy the backbuffer, it releases pBackBuffer
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc); //handles all events inside window, mouse clicks and keystrokes

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



