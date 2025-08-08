#include "functions.h"

const uintptr_t localPlayerOffset = 0x1099448;

// Offsets for positions, camera, and silent aim
const std::vector<uintptr_t> positionOffsets = { localPlayerOffset, 0x8, 0x28, 0xC4, 0x4, 0x80 };
const std::vector<uintptr_t> camOffsets = { localPlayerOffset, 0x20, 0x70 };
std::array<uintptr_t, 3> silentAimOffsets = {
    0x4873C3, // F3 0F 10 82 DC 00 00 00
    0x4873F9, // F3 0F 10 82 E0 00 00 00
    0x48742F  // F3 0F 10 82 E4 00 00 00
};

//=================================
//
//  SETTINGS & MISC VARIABLES
//
//=================================

DWORD old_protect0;
DWORD old_protect1;
DWORD old_protect2;

DWORD return_addr0;
DWORD return_addr1;
DWORD return_addr2;

unsigned char originalBytes0[16];
unsigned char originalBytes1[16];
unsigned char originalBytes2[16];

float silentAimValueX = 0;
float silentAimValueY = 0;
float silentAimValueZ = 0;

bool silentAimActive = false;


// Blacklist keywords
extern const std::vector<std::string> blacklist_keywords = {
    "pet", "portal", "abilities", "placeable", "cornerstone",
    "services", "client", "karma", "outpost", "minion", "custom",
    "hub", "merchant", "tutorial", "garden_upkeep_interactive"
};

//=================================
//
//  UTILITY & HELPER FUNCTIONS
//
//=================================

__declspec(naked) void cave0() {
    __asm {
        push eax
        push ebx

        //Execute original instruction
        movss xmm0, [edx + 0x000000DC]

        // Check if silent aim is active
        mov eax, offset silentAimActive
        cmp byte ptr[eax], 1
        jne skip_override

        // Override with silentaim value
        mov eax, offset silentAimValueX
        movss xmm0, [eax]

        skip_override:
        pop ebx
            pop eax
            jmp return_addr0
    }
}
__declspec(naked) void cave1() {
    __asm {
        push eax
        push ebx

        // Execute original instruction
        movss xmm0, [edx + 0x000000E0]

        //Check if silent aim is active
        mov eax, offset silentAimActive
        cmp byte ptr[eax], 1
        jne skip_override

        // Override with silentaim value
        mov eax, offset silentAimValueY
        movss xmm0, [eax]

        skip_override:
        pop ebx
            pop eax
            jmp return_addr1
    }
}
__declspec(naked) void cave2() {
    __asm {
        push eax
        push ebx

        // Execute original instruction
        movss xmm0, [edx + 0x000000E4]

        // Check if silent aim is active
        mov eax, offset silentAimActive
        cmp byte ptr[eax], 1
        jne skip_override

        // Override with silentaim value
        mov eax, offset silentAimValueZ
        movss xmm0, [eax]

        skip_override:
        pop ebx
            pop eax
            jmp return_addr2
    }
}

MODULEINFO GetModuleInfo(const char* szModule) { //makes sure you dont read out of bounds mem
    MODULEINFO modInfo = { 0 };
    HMODULE hModule = GetModuleHandle(szModule);
    if (hModule == 0)
        return modInfo; //if there aint nothing return null value

    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
    return modInfo;
}

DWORD FindPattern(const char* module, const char* pattern, const char* mask) { //gave up on using uintptr_t, im running 32 bit anyway
    MODULEINFO mInfo = GetModuleInfo(module);

    DWORD base = (DWORD)mInfo.lpBaseOfDll;
    DWORD size = (DWORD)mInfo.SizeOfImage;

    DWORD patternLength = (DWORD)strlen(mask);

    for (DWORD i = 0; i < size - patternLength; i++) { //teeny tiny efficiency boost by way of making sure there is enough bytes for our pattern to fit wherever we scan
        bool found = true;
        for (DWORD j = 0; j < patternLength; j++) {
            found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
        }

        if (found)
            return base + i;
    }

    return 0XDEADBEEF;
}

void PatchMovssInstructions(uintptr_t baseAddress) { //i genuinely might be retarded, i could have made this func so much more compact but whatever

    uintptr_t instrAddr0 = FindPattern("Trove.exe", "\xF3\x0F\x10\x82\xDC\x00\x00\x00", "xxxxx???");
    uintptr_t instrAddr1 = FindPattern("Trove.exe", "\xF3\x0F\x10\x82\xE0\x00\x00\x00", "xxxxx???");
    uintptr_t instrAddr2 = FindPattern("Trove.exe", "\xF3\x0F\x10\x82\xE4\x00\x00\x00", "xxxxx???");

    return_addr0 = instrAddr0 + 8;
    return_addr1 = instrAddr1 + 8;
    return_addr2 = instrAddr2 + 8;

    //fx
    VirtualProtect((void*)(unsigned char*)instrAddr0, 8, PAGE_EXECUTE_READWRITE, &old_protect0);
    for (int i = 0; i < 8; ++i) {
        originalBytes0[i] = *((unsigned char*)instrAddr0 + i);
    }
    *(unsigned char*)instrAddr0 = 0xE9; // 0xE9 = jmp
    *(DWORD*)((unsigned char*)instrAddr0 + 1) = (DWORD)&cave0 - ((DWORD)(unsigned char*)instrAddr0 + 5);
    *((unsigned char*)instrAddr0 + 5) = 0x90;
    *((unsigned char*)instrAddr0 + 6) = 0x90;
    *((unsigned char*)instrAddr0 + 7) = 0x90; //triple nop to prevent disaster
    //fy
    VirtualProtect((void*)(unsigned char*)instrAddr1, 8, PAGE_EXECUTE_READWRITE, &old_protect1);
    for (int i = 0; i < 8; ++i) {
        originalBytes1[i] = *((unsigned char*)instrAddr1 + i);
    }
    *(unsigned char*)instrAddr1 = 0xE9; // 0xE9 = jmp
    *(DWORD*)((unsigned char*)instrAddr1 + 1) = (DWORD)&cave1 - ((DWORD)(unsigned char*)instrAddr1 + 5);
    *((unsigned char*)instrAddr1 + 5) = 0x90;
    *((unsigned char*)instrAddr1 + 6) = 0x90;
    *((unsigned char*)instrAddr1 + 7) = 0x90;
    //fz
    VirtualProtect((void*)(unsigned char*)instrAddr2, 8, PAGE_EXECUTE_READWRITE, &old_protect2);
    for (int i = 0; i < 8; ++i) {
        originalBytes2[i] = *((unsigned char*)instrAddr2 + i);
    }
    *(unsigned char*)instrAddr2 = 0xE9; // 0xE9 = jmp
    *(DWORD*)((unsigned char*)instrAddr2 + 1) = (DWORD)&cave2 - ((DWORD)(unsigned char*)instrAddr2 + 5);
    *((unsigned char*)instrAddr2 + 5) = 0x90;
    *((unsigned char*)instrAddr2 + 6) = 0x90;
    *((unsigned char*)instrAddr2 + 7) = 0x90;
}

void RepairMovssInstruction(uintptr_t baseAddress) { //unused ngl
    uintptr_t instrAddr0 = baseAddress + silentAimOffsets[0];
    uintptr_t instrAddr1 = baseAddress + silentAimOffsets[1];
    uintptr_t instrAddr2 = baseAddress + silentAimOffsets[2];
    //fx
    for (int i = 0; i < 6; ++i) {
        *((unsigned char*)instrAddr0 + i) = originalBytes0[i];
    }
    VirtualProtect((void*)(unsigned char*)instrAddr0, 8, old_protect0, NULL);
    //fy
    for (int i = 0; i < 6; ++i) {
        *((unsigned char*)instrAddr1 + i) = originalBytes1[i];
    }
    VirtualProtect((void*)(unsigned char*)instrAddr1, 8, old_protect1, NULL);
    //fz
    for (int i = 0; i < 6; ++i) {
        *((unsigned char*)instrAddr2 + i) = originalBytes2[i];
    }
    VirtualProtect((void*)(unsigned char*)instrAddr2, 8, old_protect2, NULL);
}

std::tuple<float, float, float> GetForwardVector(float sx, float sy, float sz, float dx, float dy, float dz) {
    float vx = dx - sx;
    float vy = dy - sy;
    float vz = dz - sz;
    float length = sqrt(vx * vx + vy * vy + vz * vz);
    if (length == 0) return { 0.0f, 0.0f, 0.0f };
    return { vx / length, vy / length, vz / length };
}

void AttachConsole()
{
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

    std::cout.clear();
    std::cin.clear();
    std::cerr.clear();

    std::cout << "[+] Console attached!\n";
}

uintptr_t GetAddress(uintptr_t base, const std::vector<uintptr_t>& offsets) {
    uintptr_t address = base;

    for (size_t i = 0; i + 1 < offsets.size(); ++i) {
        if (address == 0) return 0;
        if (IsBadReadPtr(reinterpret_cast<void*>(address + offsets[i]), sizeof(uintptr_t))) {
            return 0;
        }
        address = *reinterpret_cast<uintptr_t*>(address + offsets[i]);
    }

    return offsets.empty() ? address : (address + offsets.back());
}

float ReadFloat(uintptr_t address) {
    float value = 0.0f;
    if (address == 0) return 0;
    if (IsBadReadPtr(reinterpret_cast<void*>(address), sizeof(uintptr_t))) {
        return 0;
    }
    value = *reinterpret_cast<float*>(address);
    return value;
}

bool WriteFloat(uintptr_t address, float value) {
    *reinterpret_cast<float*>(address) = value;
    return true;
}

bool ReadValue(uintptr_t address, const std::string& type, void* out) {
    // Validate address first
    if (address == 0) {
        //std::cout << "[DEBUG] ReadValue: address is null" << std::endl;
        return false;
    }

    size_t typeSize = 4; // default
    if (type == "double") typeSize = 8;

    if (IsBadReadPtr(reinterpret_cast<void*>(address), typeSize)) {
        //std::cout << "[DEBUG] ReadValue: bad memory at 0x" << std::hex << address << std::endl;
        return false;
    }

    try {
        if (type == "float") {
            *static_cast<float*>(out) = *reinterpret_cast<float*>(address);
            return true;
        }
        else if (type == "uint") {
            *static_cast<uint32_t*>(out) = *reinterpret_cast<uint32_t*>(address);
            return true;
        }
        else if (type == "double") {
            *static_cast<double*>(out) = *reinterpret_cast<double*>(address);
            return true;
        }
        else if (type == "int") {
            *static_cast<int32_t*>(out) = *reinterpret_cast<int32_t*>(address);
            return true;
        }
    }
    catch (...) {
        //std::cout << "[DEBUG] ReadValue: exception at 0x" << std::hex << address << std::endl;
        return false;
    }

    std::cout << "[DEBUG] ReadValue: unknown type " << type << std::endl;
    return false;
}

std::string ReadString(uintptr_t address, size_t maxLen) {
    if (address == 0) {
        //std::cout << "[DEBUG] ReadString: address is null" << std::endl;
        return "";
    }

    if (IsBadReadPtr(reinterpret_cast<void*>(address), 1)) {
        //std::cout << "[DEBUG] ReadString: bad memory at 0x" << std::hex << address << std::endl;
        return "";
    }

    try {
        const char* str = reinterpret_cast<const char*>(address);
        size_t len = 0;

        // Safe string length calculation
        while (len < maxLen) {
            if (IsBadReadPtr(reinterpret_cast<void*>(address + len), 1)) {
                break;
            }
            if (str[len] == '\0') {
                break;
            }
            len++;
        }

        return std::string(str, len);
    }
    catch (...) {
        std::cout << "[DEBUG] ReadString: exception at 0x" << std::hex << address << std::endl;
        return "";
    }
}

bool ContainsKeyword(const std::string& str, const std::vector<std::string>& keywords) {
    for (const auto& kw : keywords) {
        if (str.find(kw) != std::string::npos) return true;
    }
    return false;
}

void WriteToMemory(uintptr_t addressToWrite, char* valueToWrite, int byteNum) { //using uintptr_t instead of DWORD incase i wanna reuse for 64 bit
    unsigned long oldProtection;

    VirtualProtect((LPVOID)(addressToWrite), byteNum, PAGE_EXECUTE_READWRITE, &oldProtection); //&oldprotect send the old protection into our unsigned long incase we want to restore
    memcpy((LPVOID)addressToWrite, valueToWrite, byteNum);

    VirtualProtect((LPVOID)(addressToWrite), byteNum, oldProtection, NULL);
}// honestly dont think ill use this function, ill jst do it normally its like 4 lines bro

std::vector<Entity> GetAllEntities(uintptr_t world)
{
    std::vector<Entity> entities;
    if (!world) return entities;

    uintptr_t nodeInfo = GetAddress(world, { 0x7C });
    if (!nodeInfo) return entities;

    uint32_t baseAddr = 0;
    ReadValue(nodeInfo, "uint", &baseAddr);
    if (!baseAddr) return entities;

    uint32_t size = 0, step = 0;
    uintptr_t sizeAddr = GetAddress(nodeInfo, { 0x8 });
    ReadValue(sizeAddr, "uint", &size);
    uintptr_t stepAddr = GetAddress(nodeInfo, { 0x4 });
    ReadValue(stepAddr, "uint", &step);

    std::vector<uintptr_t> nodes;
    for (uint32_t i = 0; i < size; ++i) {
        uintptr_t addr = baseAddr + i * step;
        while (addr) {
            uint32_t next = 0;
            if (!ReadValue(addr, "uint", &next)) break;
            if (next != 1) nodes.push_back(addr);
            addr = next & ~1u;
        }
    }

    for (auto node : nodes) {
        uintptr_t ent = GetAddress(node, { 0x10, 0xC4, 0x4, 0x0 });
        if (!ent) continue;

        uintptr_t nameAddr = GetAddress(ent, { 0x58, 0x64, 0x0 });
        auto name = ReadString(nameAddr, 96);
        if (name.empty()) continue;



        uintptr_t posAddr = GetAddress(ent, { 0x58, 0xC4, 0x4, 0x80 });
        float x = 0, y = 0, z = 0;
        ReadValue(posAddr, "float", &x);
        ReadValue(posAddr + 4, "float", &y);
        ReadValue(posAddr + 8, "float", &z);

        float scale = 0;
        ReadValue(posAddr + 0x74, "float", &scale);

        int level = 0;
        uintptr_t lvlAddr = GetAddress(ent, { 0x58, 0xC4, 0x54, 0x120 });
        ReadValue(lvlAddr, "int", &level);

        int death = 0;
        uintptr_t deathAddr = GetAddress(ent, { 0x58, 0x0 });
        ReadValue(deathAddr, "int", &death);

        double health = 0;
        uintptr_t healthAddr = GetAddress(ent, { 0x58, 0xC4, 0x84, 0x80 });
        ReadValue(healthAddr, "double", &health);

        entities.push_back({ name, x, y, z, scale, level, death, health });
    }

    //std::cout << "\n[Info] Base: 0x" << std::hex << baseAddr << " Size: " << std::dec << size << " Step: " << step << std::endl;
    //std::cout << "[Info] Total entities found: " << entities.size() << std::endl;

    return entities;
}