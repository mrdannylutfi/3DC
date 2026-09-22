#include <stdio.h>

// ==========================================
// ENUMS & TYPES
// ==========================================

typedef enum {
    SKIRMISH,    // Min players: 2, Max players: 10
    DEATHMATCH,  // Min players: 2, Max players: 10
    ONEONONE     // Min players: 2, Max players: 2
} GameType;

typedef enum {
    MAP1,
    MAP2,
    MAP3
} MapType;

typedef enum {
    USA, // United States
    CAN, // Canada
    MEX, // Mexico
    EU,  // Europe
    AFR, // Africa
    ASI, // Asia
    AUS  // Australia
} RegionType;

typedef enum {
    NVA, // Nvidia
    AMD, // AMD Catalyst
    INT  // Intel
} GpuBrand;

// ==========================================
// STRUCTURES
// ==========================================

typedef struct {
    int minPlayers; // Min players to form a game
    int maxPlayers; // Max players allowed
    int maxBots;    // Max NPC AI bots
} PlayerConfig;

typedef struct {
    int serverIndex;
    int isDedicated;          // 0 = dedicated, 1 = not dedicated
    int isPasswordProtected;  // 1 = password protected
    int isLinux;              // 2 = linux
    char* currentVersion;
} ServerTypeInfo;

typedef struct {
    int buddyCount;
    char** buddyList; // Array of server names (buddy servers)
} BuddyServer;

typedef struct {
    RegionType region;
    int maxPing; // Maximum ping for connecting clients (0 = no max)
} ServerConfig;

typedef struct {
    int minCpu;         // Min CPU requirement
    GpuBrand minGpu;   // Min GPU requirement vendor
} PcRequirements;

// ==========================================
// FUNCTION DECLARATION
// ==========================================

/**
 * Obtains server information and registers this LAN server to the ServerList.
 * Accessible UI / Actions: Cancel, Server Screen Update.
 */
void addLANServer(
    GameType gType,
    MapType mType,
    PlayerConfig players,
    ServerTypeInfo serverType,
    BuddyServer buddyServer,
    ServerConfig serverConfig,
    PcRequirements pcRequirements
) {
    // Function logic to append server details to ServerList goes here
    printf("LAN Server successfully registered.\n");
}
