#pragma once

#define WIN_32_LEAN_AND_MEAN

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cfloat>
#include <array>
#include <thread>
#include <algorithm>


//functions.h
// 
//===========================
//
//  OFFSETS AND STRUCTS
//
//===========================
extern const uintptr_t localPlayerOffset;

// Offsets for positions, camera, and silent aim
extern const std::vector<uintptr_t> positionOffsets;
extern const std::vector<uintptr_t> camOffsets;
extern std::array<uintptr_t, 3> silentAimOffsets;

struct Entity {
    std::string name;
    float x, y, z;
    float scale;
    int level;
    int death;
    double health;
};



//=================================
//
//  SETTINGS & MISC VARIABLES
//
//=================================

extern DWORD old_protect0;
extern DWORD old_protect1;
extern DWORD old_protect2;

extern DWORD return_addr0;
extern DWORD return_addr1;
extern DWORD return_addr2;

extern unsigned char originalBytes0[16];
extern unsigned char originalBytes1[16];
extern unsigned char originalBytes2[16];

extern float silentAimValueX;
extern float silentAimValueY;
extern float silentAimValueZ;

extern bool silentAimActive;


extern const std::vector<std::string> blacklist_keywords;

//=================================
//
//  FUNCTION DECLARATIONS
//
//=================================

void cave0();
void cave1();
void cave2();

MODULEINFO GetModuleInfo(const char* szModule);

DWORD FindPattern(const char* module, const char* pattern, const char* mask);

void PatchMovssInstructions(uintptr_t baseAddress);

void RepairMovssInstruction(uintptr_t baseAddress);

std::tuple<float, float, float> GetForwardVector(float sx, float sy, float sz, float dx, float dy, float dz);

void AttachConsole();

uintptr_t GetAddress(uintptr_t base, const std::vector<uintptr_t>& offsets);

float ReadFloat(uintptr_t address);

bool WriteFloat(uintptr_t address, float value);

bool ReadValue(uintptr_t address, const std::string& type, void* out);

std::string ReadString(uintptr_t address, size_t maxLen);

bool ContainsKeyword(const std::string& str, const std::vector<std::string>& keywords);

void WriteToMemory(uintptr_t addressToWrite, char* valueToWrite, int byteNum);

std::vector<Entity> GetAllEntities(uintptr_t world);



