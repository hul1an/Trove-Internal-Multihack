#define WIN_32_LEAN_AND_MEAN

#include "functions.h"
#include "gui/gui.h"


int flipflop = 0;

void MainThread()
{   
    if (!SetupD3DHook()) {
        Sleep(100);
        return;
    }

   // AttachConsole();
    //std::cout << "[*] DLL injected and running.\n";
    uintptr_t base = (uintptr_t)GetModuleHandleA("Trove.exe");
    //std::cout << "[*] Base: 0x" << std::hex << base << std::endl;
    
    PatchMovssInstructions(base);
    
    while (true) {

        uintptr_t localPlayer = GetAddress(base, { localPlayerOffset });
 
        uintptr_t playerPosPtr = GetAddress(base, positionOffsets);
        uintptr_t camPtr = GetAddress(base, camOffsets);

        uintptr_t world = GetAddress(base, { 0x109BC98, 0x0 });

        auto allEntities = GetAllEntities(world);
        std::vector<Entity> filteredEntities;
        for (const auto& e : allEntities) {
            if (!ContainsKeyword(e.name, blacklist_keywords)) {
                filteredEntities.push_back(e);
            }
        }
        
        float px = ReadFloat(playerPosPtr);
        float py = ReadFloat(playerPosPtr + 4);
        float pz = ReadFloat(playerPosPtr + 8);
        
        float cx = ReadFloat(camPtr);
        float cy = ReadFloat(camPtr + 4);
        float cz = ReadFloat(camPtr + 8);

        //std::cout << "[PlayerPos] x: " << px << ", y: " << py << ", z: " << pz << std::endl;
        //std::cout << "[CamPos] x: " << cx << ", y: " << cy << ", z: " << cz << std::endl;

        float closestDist = FLT_MAX;
        closestEntity = { "", 0, 0, 0, 0, 0, 0, 0.0 };
        bool found = false;

        if (filteredEntities.empty()) {
            found = false;
            silentAimActive = false;
        }
        
        if (!filteredEntities.empty()) {
            for (const auto& e : filteredEntities) {
                float dx = e.x - px;
                float dy = e.y - py;
                float dz = e.z - pz;
                float dist = sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < closestDist && dist <= maxRangeSlider) {
                    closestDist = dist;
                    closestEntity = e;
                    found = true;
                }
            }
        }
        
        
        if (found && silentAimCb) {
            auto [fx, fy, fz] = GetForwardVector(cx, cy, cz, closestEntity.x, closestEntity.y, closestEntity.z);
            silentAimValueX = -fx;
            silentAimValueY = -fy - 0.04;
            silentAimValueZ = -fz;
            silentAimActive = true;
            

           // flipflop++;
            //if (flipflop >= 10) {
                //std::cout << "[DEBUG] Target: " << closestEntity.name << " Distance: " << closestDist << " scale: " << closestEntity.scale << " level: "<< closestEntity.level << std::endl;
              //  flipflop = 0;
           // }
        }
        else {
            silentAimActive = false;
            //if (flipflop >= 10) {
                //std::cout << "No valid entity found within range " << maxRangeSlider << ".\n";
                //flipflop = 0;
            //}
            
        }
        
        //std::cout << "hello from main loop" << std::endl;
        Sleep(10);
    }
}



BOOL WINAPI DllMain(HMODULE instance, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(instance);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, NULL, 0, NULL);

	}
	return true;
}