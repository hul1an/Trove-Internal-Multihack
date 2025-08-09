#define WIN_32_LEAN_AND_MEAN

#include "functions.h"
#include "gui/gui.h"


void MainThread()
{   
    if (!SetupD3DHook()) {
        Sleep(100);
        return;
    }

    uintptr_t base = (uintptr_t)GetModuleHandleA("Trove.exe");
    
    PatchMovssInstructions(base);
    
    while (true) {

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            show_menu = (!show_menu);
        }

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

        closestDist = FLT_MAX;
        float highestLevel = 0.0f;
		float highestHP = 0.0f;
        closestEntity = { "", 0, 0, 0, 0, 0, 0, 0.0 };
        bool found = false;

        if (filteredEntities.empty()) {
            found = false;
            silentAimActive = false;
        }

        //TODO: create proper multi-filter targeting system, add optiont to filter by health, boss only, etc
        //do cursed skulls have any sort of active indicator within the entity list?
        //if i could figure out how to do boss only + skulls thatd be best
        //add fov system, 180 flicking is a bit much for main account, i'd feel safe with like cs2 semirage fov LOL
        
        if (!filteredEntities.empty()) {
            for (const auto& e : filteredEntities) {
                float dx = e.x - px;
                float dy = e.y - py;
                float dz = e.z - pz;
                float dist = sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < closestDist && dist <= maxRangeSlider) {
                    if ((closestDist - dist) < 20 && !bossPriority) {
						//if enemies are within 20 units of each other, prefer the one with the highest HP
                        if (e.health > highestHP) {
							highestHP = e.health;
                            closestDist = dist;
                            closestEntity = e;
                        }                    
                    }
                    if ((closestDist - dist) < 40 && bossPriority) {
                        //if boss priority is enabled and enemies are within 40 units of each other, prefer the one with the highest HP
                        if (e.health > highestHP) {
                            highestHP = e.health;
                            closestDist = dist;
                            closestEntity = e;
                        }
                    }
                    else if((closestDist - dist) > 20) {
                        closestDist = dist;
                        closestEntity = e;
                    }
                    found = true;
                }
            }
        }
        
        if (found && silentAimCb && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {

            if (closestEntity.level < 0 || closestEntity.level > 100) {
                closestEntity.level = 0;
            }

            auto [fx, fy, fz] = GetForwardVector(cx, cy, cz, closestEntity.x, closestEntity.y, closestEntity.z);
            silentAimValueX = -fx;
            silentAimValueY = -fy - 0.04;
            silentAimValueZ = -fz;
            silentAimActive = true;

        }
        else {
            silentAimActive = false;
            
        }
        
        Sleep(1);
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