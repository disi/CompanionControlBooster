#include <Global.h>

// Global logger pointer
std::shared_ptr<spdlog::logger> gLog;

// --- Explicit F4SE_API Definition ---
// This macro is essential for exporting functions from the DLL.
// If the F4SE headers aren't providing it correctly for your setup,
// we define it directly.
#define F4SE_API __declspec(dllexport)

// This is used by commonLibF4
namespace Version {
inline constexpr std::size_t MAJOR = 0;
inline constexpr std::size_t MINOR = 1;
inline constexpr std::size_t PATCH = 2;
inline constexpr auto NAME = "0.1.2"sv;
inline constexpr auto AUTHORNAME = "disi"sv;
inline constexpr auto PROJECT = "CCBCL"sv;
} // namespace Version

// Declare the F4SEMessagingInterface and F4SEScaleformInterface
const F4SE::MessagingInterface* g_messagingInterface = nullptr;
// Papyrus interface
const F4SE::PapyrusInterface* g_papyrusInterface = nullptr;
// Task interface for menus and threads
const F4SE::TaskInterface* g_taskInterface = nullptr;
// Plugin handle
F4SE::PluginHandle g_pluginHandle = 0;
// Datahandler
RE::TESDataHandler* g_dataHandle = 0;

// --- Global Variables ---
// Our update timers
CCB_RepeatingTimer g_updateTimer;
CCB_RepeatingTimer g_movementTimer;
// Global death handler registered flag
std::atomic<bool> g_deathHandlerRegistered = false;
// --- User Settings ---
// Global debug flag
bool DEBUGGING = false;
// Current game time
float CURRENT_GAME_TIME = 0.0f;
// Global update interval (in seconds)
float UPDATE_INTERVAL = 3.0f;
// Read the ini every x updates (0 = only on game start)
int INI_RELOAD_INTERVAL = 10;
// Actor search radius around the player in game units
float ACTOR_SEARCH_RADIUS = 4000.0f;
// AI settings
float AI_HEALTH_THRESHOLD = 40.0f;
bool AI_USE_STIMPAK = true;
bool AI_USE_STIMPAK_UNLIMITED = false;
bool AI_AUTO_REVIVE = true;
bool AI_FLEE_COMBAT = true;
float AI_FLEE_DISTANCE = 500.0f;
bool AI_EQUIP_ITEMS = true;
bool AI_EQUIP_GEAR = false;
bool AI_EQUIP_AMMO_REFILL = true;
int AI_EQUIP_AMMO_AMOUNT = 50;
// Movement settings
bool AI_STUCK_CHECK = true;
int AI_STUCK_THRESHOLD = 20;
int AI_STUCK_COLLISIONS = 2;
float AI_STUCK_SPEED = 10.0f;
float AI_STUCK_DISTANCE = 1500.0f;
// Aggression settings
bool AI_AGGRESSION_ENABLED = true;
bool AI_AGGRESSION_ALL = true;
bool AI_AGGRESSION_SNEAK = false;
float AI_AGGRESSION_RADIUS0 = 3000.0f;
float AI_AGGRESSION_RADIUS1 = 2000.0f;
float AI_AGGRESSION_RADIUS2 = 1000.0f;
// Chatter settings
bool CHATTER_ENABLED = false;
float CHATTER_MULTIPLIER = 1.0f;
float CHATTER_MULTIPLIER_SNEAK = 5.0f;
// Combat AI settings
bool COMBAT_ENABLED = true;
int COMBAT_TARGET = 0;
float COMBAT_OFFENSIVE = 0.5f;
float COMBAT_DEFENSIVE = 0.5f;
float COMBAT_RANGED = 1.0f;
float COMBAT_MELEE = 0.2f;
// Ranged
float COMBAT_RANGED_ADJUSTMENT = 1.0f;
float COMBAT_RANGED_CROUCHING = 1.0f;
float COMBAT_RANGED_STRAFE = 0.5f;
float COMBAT_RANGED_WAITING = 0.5f;
float COMBAT_RANGED_ACCURACY = 0.5f;
// Close-Quarters
float COMBAT_CLOSE_FALLBACK = 0.5f;
float COMBAT_CLOSE_CIRCLE = 0.5f;
float COMBAT_CLOSE_DISENGAGE = 0.5f;
float COMBAT_CLOSE_FLANK = 0.5f;
int COMBAT_CLOSE_THROW_GRENADE = 0;
// Cover
float COMBAT_COVER_DISTANCE = 0.5f;
// Loot item filter flag
bool LOOT_ENABLED = true;
bool LOOT_COMBAT = false;
float LOOT_RADIUS = 1000.0f;
bool LOOT_JUNK = true;
bool LOOT_AMMO = true;
bool LOOT_AID = true;
int LOOT_MIN_VALUE = 0;
int LOOT_MAX_VALUE = 800;
bool LOOT_STEAL = false;
bool LOOT_WEIGHT_LIMIT = true;
// XP gain settings
bool XP_ENABLED = true;
float XP_RATIO = 0.5f;
float XP_KILLER_TOLERANCE = 3000.0f;
// Buff settings
bool BUFF_ENABLED = true;
float BUFF_HEAL_RATE = 5.0f;
float BUFF_COMBAT_HEAL_RATE = 5.0f;
float BUFF_DAMAGE_RESIST = 200.0f;
float BUFF_FIRE_RESIST = 50.0f;
float BUFF_ELECTRICAL_RESIST = 50.0f;
float BUFF_FROST_RESIST = 50.0f;
float BUFF_ENERGY_RESIST = 50.0f;
float BUFF_POISON_RESIST = 50.0f;
float BUFF_RADIATION_RESIST = 50.0f;
float BUFF_AGILITY = 5.0f;
float BUFF_ENDURANCE = 5.0f;
float BUFF_INTELLIGENCE = 5.0f;
float BUFF_LUCK = 5.0f;
float BUFF_PERCEPTION = 5.0f;
float BUFF_SNEAK = 5.0f;
float BUFF_STRENGTH = 5.0f;
float BUFF_LOCKPICK = 5.0f;
float BUFF_CARRYWEIGHT = 1000.0f;
// --- Power Armor Settings ---
bool PA_ENABLED = true;
float PA_REPAIR_AMOUNT = 0.01f;
// Perk settings
bool PERK_ENABLED = true;
std::vector<std::uint32_t> PERK_ID_LIST;
std::vector<RE::BGSPerk*> g_perkList;
// --- Keyword settings ---
bool KEYWORD_ENABLED = true;
std::vector<std::uint32_t> KEYWORD_ID_LIST;
std::vector<RE::BGSKeyword*> g_keywordList;
// -- Actor exclusion list --
std::vector<std::uint32_t> EXCLUDE_ACTOR_ID_LIST;
// Threat score weights
float THREAT_WEAPON_BONUS = 1.0f;
float THREAT_LEGENDARY_BONUS = 1.0f;
float THREAT_UNIQUE_BONUS = 1.0f;
float THREAT_HEALTH_BONUS = 1.0f;
float THREAT_ALERT_BONUS = 1.0f;
// --- DATA ---
// Current companion faction ID
std::uint32_t CURRENT_COMPANION_FACTION_ID = 0x00023C01;
RE::TESFaction* g_companionFaction = nullptr;
// ActorValue for HC downed state
std::uint32_t ACTORVALUE_HC_DOWNED_ID = 0x00249F6D; // 00249F6D HC_IsCompanionInNeedOfHealing
RE::ActorValueInfo* g_actorValueHCDowned = nullptr;
// Item forms
std::uint32_t ITEM_STIMPAK_ID = 0x00023736; // 00023736 Stimpak
RE::TESForm* g_itemStimpak = nullptr;
std::uint32_t ITEM_REPAIRKIT_ID = 0x00004F12; // 00004F12 Repair Kit
RE::TESForm* g_itemRepairKit = nullptr;
// Race forms for stimpak users
std::vector<std::uint32_t> RACE_STIMPAK_ID = {0x00013746, 0x000EAFB6}; // 00013746 Human, 000EAFB6 Ghoul
std::vector<RE::TESRace*> g_raceStimpak;
// Synth 3 component
std::uint32_t RACE_SYNTH3C_ID = 0x000CFF74; // 000CFF74 Synth3 component
RE::TESObjectMISC* g_raceSynth3C = nullptr;
// IDLE animations
std::uint32_t IDLE_STIMPAK_ID = 0x000B1CF9; // 000B1CF9 3rdPUseStimpakOnSelf
RE::TESIdleForm* g_idleStimpak = nullptr;
// Keywords
std::uint32_t KYWD_ARMORTYPEPOWER_ID = 0x0004D8A1;    // 0004D8A1 ArmorTypePower
RE::BGSKeyword* g_kwdArmorTypePower = nullptr;
std::uint32_t KYWD_ISPOWERARMORFRAME_ID = 0x0015503F; // 0015503F isPowerArmorFrame
RE::BGSKeyword* g_kwdIsPowerArmorFrame = nullptr;
// Packages
std::uint32_t PACK_FOLLOWERSCOMPANION_ID = 0x0002A101; // 0002A101 FollowersCompanion
RE::TESPackage* g_packFollowersCompanion = nullptr;

// Helper function to extract value from a line
inline std::string GetValueFromLine(const std::string& line) {
    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos)
        return "";
    std::string value = line.substr(eqPos + 1);
    size_t semicolonPos = value.find(';');
    // remove comments after the value
    if (semicolonPos != std::string::npos)
        value = value.substr(0, semicolonPos);
    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
    return value;
}
// Helper to get the directory of the plugin DLL
std::string GetPluginDirectory(HMODULE hModule) {
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
}
// Helper to parse hex string to uint32_t
uint32_t ParseHexFormID(const std::string& hexStr) {
    return static_cast<uint32_t>(std::stoul(hexStr, nullptr, 16));
}
void LoadConfig() {
    // Clear previous perk list if any
    PERK_ID_LIST.clear();
    // Clear previous actor exclusion list if any
    EXCLUDE_ACTOR_ID_LIST.clear();
    // Clear previous race stimpak list if any
    RACE_STIMPAK_ID.clear();
    // Get the DLL handle for this plugin
    HMODULE hModule = GetModuleHandleA("CCBCL.dll");
    std::string configPath = GetPluginDirectory(hModule) + "CCBCL.ini";
    REX::INFO("LoadConfig: Loading config from: {}", configPath);
    // First try to open the stream directly
    std::ifstream file(configPath);
    // Check if the file opened successfully
    if (!file.is_open()) {
        REX::WARN("LoadConfig: Could not open INI file: {}. Creating default.", configPath);
        // Create the file with defaultIni contents
        std::ofstream out(configPath);
        if (out.is_open()) {
            out << defaultIni;
            out.close();
            REX::INFO("LoadConfig: Default INI created at: {}", configPath);
        } else {
            REX::WARN("LoadConfig: Failed to create default INI at: {}", configPath);
            return;
        }
        // Try to open again for reading
        file.open(configPath);
        if (!file.is_open()) {
            REX::WARN("LoadConfig: Still could not open INI file after creating default: {}", configPath);
            return;
        }
    }
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';')
            continue;
        // Lower case for case-insensitive comparison
        std::string lowerLine = ToLower(line);
        // --- Debugging flag ---
        if (lowerLine.find("debugging") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                DEBUGGING = true;
            } else {
                DEBUGGING = false;
            }
            continue;
        }

        // --- Update Interval ---
        if (lowerLine.find("update_interval") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float interval = std::stof(value);
                if (interval > 0.0f) {
                    UPDATE_INTERVAL = interval;
                } else {
                    REX::WARN("LoadConfig: Invalid Update Interval value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Update Interval value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ini_reload_interval") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                int interval = std::stoi(value);
                if (interval >= 0) {
                    INI_RELOAD_INTERVAL = interval;
                } else {
                    REX::WARN("LoadConfig: Invalid INI Reload Interval value: {}. Must be non-negative.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing INI Reload Interval value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("actor_search_radius") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float radius = std::stof(value);
                if (radius > 0.0f) {
                    ACTOR_SEARCH_RADIUS = radius;
                } else {
                    REX::WARN("LoadConfig: Invalid Actor Search Radius value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Actor Search Radius value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- AI Behavior Settings ---
        if (lowerLine.find("ai_health_threshold") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float threshold = std::stof(value);
                if (threshold >= 0.0f && threshold <= 100.0f) {
                    AI_HEALTH_THRESHOLD = threshold;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Health Threshold value: {}. Must be between 0 and 100.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Health Threshold value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_use_stimpak") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_USE_STIMPAK = true;
            } else {
                AI_USE_STIMPAK = false;
            }
            continue;
        }
        if (lowerLine.find("ai_use_stimpak_unlimited") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_USE_STIMPAK_UNLIMITED = true;
            } else {
                AI_USE_STIMPAK_UNLIMITED = false;
            }
            continue;
        }
        if (lowerLine.find("ai_auto_revive") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_AUTO_REVIVE = true;
            } else {
                AI_AUTO_REVIVE = false;
            }
            continue;
        }
        if (lowerLine.find("ai_flee_combat") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_FLEE_COMBAT = true;
            } else {
                AI_FLEE_COMBAT = false;
            }
            continue;
        }
        if (lowerLine.find("ai_flee_distance") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float distance = std::stof(value);
                if (distance > 0.0f) {
                    AI_FLEE_DISTANCE = distance;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Flee Distance value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Flee Distance value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_equip_items") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_EQUIP_ITEMS = true;
            } else {
                AI_EQUIP_ITEMS = false;
            }
            continue;
        }
        if (lowerLine.find("ai_equip_gear") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_EQUIP_GEAR = true;
            } else {
                AI_EQUIP_GEAR = false;
            }
            continue;
        }
        if (lowerLine.find("ai_equip_ammo_refill") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_EQUIP_AMMO_REFILL = true;
            } else {
                AI_EQUIP_AMMO_REFILL = false;
            }
            continue;
        }
        if (lowerLine.find("ai_equip_ammo_amount") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                int amount = std::stoi(value);
                if (amount > 0) {
                    AI_EQUIP_AMMO_AMOUNT = amount;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Equip Ammo Amount value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Equip Ammo Amount value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // Movement settings
        if (lowerLine.find("ai_stuck_check") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_STUCK_CHECK = true;
            } else {
                AI_STUCK_CHECK = false;
            }
            continue;
        }
        if (lowerLine.find("ai_stuck_threshold") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                int threshold = std::stoi(value);
                if (threshold >= 0) {
                    AI_STUCK_THRESHOLD = threshold;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Stuck Threshold value: {}. Must be non-negative.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Stuck Threshold value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_stuck_collisions") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                int collisions = std::stoi(value);
                if (collisions >= 0) {
                    AI_STUCK_COLLISIONS = collisions;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Stuck Collisions value: {}. Must be non-negative.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Stuck Collisions value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_stuck_speed") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float speed = std::stof(value);
                if (speed >= 0.0f) {
                    AI_STUCK_SPEED = speed;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Stuck Speed value: {}. Must be non-negative.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Stuck Speed value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_stuck_distance") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float distance = std::stof(value);
                if (distance > 0.0f) {
                    AI_STUCK_DISTANCE = distance;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Stuck Distance value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Stuck Distance value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // Aggression settings
        if (lowerLine.find("ai_aggression_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_AGGRESSION_ENABLED = true;
            } else {
                AI_AGGRESSION_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("ai_aggression_all") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_AGGRESSION_ALL = true;
            } else {
                AI_AGGRESSION_ALL = false;
            }
            continue;
        }
        if (lowerLine.find("ai_aggression_sneak") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                AI_AGGRESSION_SNEAK = true;
            } else {
                AI_AGGRESSION_SNEAK = false;
            }
            continue;
        }
        if (lowerLine.find("ai_aggression_radius0") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float radius = std::stof(value);
                if (radius > 0.0f) {
                    AI_AGGRESSION_RADIUS0 = radius;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Aggression Radius0 value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Aggression Radius0 value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_aggression_radius1") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float radius = std::stof(value);
                if (radius > 0.0f) {
                    AI_AGGRESSION_RADIUS1 = radius;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Aggression Radius1 value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Aggression Radius1 value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("ai_aggression_radius2") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float radius = std::stof(value);
                if (radius > 0.0f) {
                    AI_AGGRESSION_RADIUS2 = radius;
                } else {
                    REX::WARN("LoadConfig: Invalid AI Aggression Radius2 value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing AI Aggression Radius2 value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // Chatter settings
        if (lowerLine.find("chatter_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                CHATTER_ENABLED = true;
            } else {
                CHATTER_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("chatter_multiplier") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float multiplier = std::stof(value);
                if (multiplier > 0.0f) {
                    CHATTER_MULTIPLIER = multiplier;
                } else {
                    REX::WARN("LoadConfig: Invalid Chatter Multiplier value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Chatter Multiplier value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("chatter_multiplier_sneak") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float multiplier = std::stof(value);
                if (multiplier > 0.0f) {
                    CHATTER_MULTIPLIER_SNEAK = multiplier;
                } else {
                    REX::WARN("LoadConfig: Invalid Chatter Multiplier Sneak value: {}. Must be positive.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Chatter Multiplier Sneak value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Combat AI Settings ---
        if (lowerLine.find("combat_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                COMBAT_ENABLED = true;
            } else {
                COMBAT_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("combat_target") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                int target = std::stoi(value);
                if (target >= 0 && target <= 2) {
                    COMBAT_TARGET = target;
                } else {
                    REX::WARN("LoadConfig: Invalid Combat Target value: {}. Must be 0 (Nearest), 1 (Lowest Threat), or 2 (Highest Threat).", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Target value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_offensive") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float offensive = std::stof(value);
                if (offensive >= 0.0f && offensive <= 1.0f) {
                    COMBAT_OFFENSIVE = offensive;
                } else {
                    REX::WARN("LoadConfig: Invalid Combat Offensive value: {}. Must be between 0.0 and 1.0.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Offensive value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_defensive") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float defensive = std::stof(value);
                if (defensive >= 0.0f && defensive <= 1.0f) {
                    COMBAT_DEFENSIVE = defensive;
                } else {
                    REX::WARN("LoadConfig: Invalid Combat Defensive value: {}. Must be between 0.0 and 1.0.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Defensive value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_ranged") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float ranged = std::stof(value);
                if (ranged >= 0.0f && ranged <= 1.0f) {
                    COMBAT_RANGED = ranged;
                } else {
                    REX::WARN("LoadConfig: Invalid Combat Ranged value: {}. Must be between 0.0 and 1.0.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_melee") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                float melee = std::stof(value);
                if (melee >= 0.0f && melee <= 1.0f) {
                    COMBAT_MELEE = melee;
                } else {
                    REX::WARN("LoadConfig: Invalid Combat Melee value: {}. Must be between 0.0 and 1.0.", value);
                }
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Melee value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // --- Ranged Combat Modifiers ---
        if (lowerLine.find("combat_ranged_adjustment") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_RANGED_ADJUSTMENT = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged Adjustment value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_ranged_crouching") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_RANGED_CROUCHING = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged Crouching value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_ranged_strafe") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_RANGED_STRAFE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged Strafe value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_ranged_waiting") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_RANGED_WAITING = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged Waiting value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_ranged_accuracy") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_RANGED_ACCURACY = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Ranged Accuracy value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // --- Close-Quarters Combat Modifiers ---
        if (lowerLine.find("combat_close_fallback") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_CLOSE_FALLBACK = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Close Fallback value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_close_circle") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_CLOSE_CIRCLE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Close Circle value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_close_disengage") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_CLOSE_DISENGAGE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Close Disengage value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_close_flank") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_CLOSE_FLANK = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Close Flank value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("combat_close_throw_grenade") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_CLOSE_THROW_GRENADE = std::stoi(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Close Throw Grenade value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // --- Cover Combat Modifiers ---
        if (lowerLine.find("combat_cover_distance") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                COMBAT_COVER_DISTANCE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Combat Cover Distance value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Loot Settings ---
        if (lowerLine.find("loot_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_ENABLED = true;
            } else {
                LOOT_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("loot_combat") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_COMBAT = true;
            } else {
                LOOT_COMBAT = false;
            }
            continue;
        }
        if (lowerLine.find("loot_radius") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                LOOT_RADIUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Loot Radius: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("loot_junk") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_JUNK = true;
            } else {
                LOOT_JUNK = false;
            }
            continue;
        }
        if (lowerLine.find("loot_ammo") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_AMMO = true;
            } else {
                LOOT_AMMO = false;
            }
            continue;
        }
        if (lowerLine.find("loot_aid") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_AID = true;
            } else {
                LOOT_AID = false;
            }
            continue;
        }
        if (lowerLine.find("loot_min_value") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                LOOT_MIN_VALUE = std::stoi(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Loot Min Value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("loot_max_value") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                LOOT_MAX_VALUE = std::stoi(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Loot Max Value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("loot_steal") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_STEAL = true;
            } else {
                LOOT_STEAL = false;
            }
            continue;
        }
        if (lowerLine.find("loot_weight_limit") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                LOOT_WEIGHT_LIMIT = true;
            } else {
                LOOT_WEIGHT_LIMIT = false;
            }
            continue;
        }

        // --- XP Gain Settings ---
        if (lowerLine.find("xp_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                XP_ENABLED = true;
            } else {
                XP_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("xp_ratio") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                XP_RATIO = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing XP Ratio: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("xp_killer_tolerance") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                XP_KILLER_TOLERANCE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing XP Killer Tolerance: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Buff Settings ---
        if (lowerLine.find("buff_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                BUFF_ENABLED = true;
            } else {
                BUFF_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("buff_heal_rate") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_HEAL_RATE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Heal Rate: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_combat_heal_rate") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_COMBAT_HEAL_RATE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Combat Heal Rate: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_damage_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_DAMAGE_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Damage Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_fire_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_FIRE_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Fire Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_electrical_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_ELECTRICAL_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Electrical Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_frost_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_FROST_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Frost Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_energy_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_ENERGY_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Energy Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_poison_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_POISON_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Poison Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_radiation_resist") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_RADIATION_RESIST = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Radiation Resist: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_agility") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_AGILITY = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Agility: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_endurance") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_ENDURANCE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Endurance: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_intelligence") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_INTELLIGENCE = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Intelligence: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_lockpick") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_LOCKPICK = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Lockpick: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_luck") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_LUCK = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Luck: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_perception") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_PERCEPTION = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Perception: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_sneak") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_SNEAK = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Sneak: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_strength") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_STRENGTH = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Strength: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("buff_carryweight") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                BUFF_CARRYWEIGHT = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Buff Carry Weight: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Power Armor Settings ---
        if (lowerLine.find("pa_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                PA_ENABLED = true;
            } else {
                PA_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("pa_repair_amount") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                PA_REPAIR_AMOUNT = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Power Armor Repair Amount: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Perk Settings ---
        if (lowerLine.find("perk_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                PERK_ENABLED = true;
            } else {
                PERK_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("perk_to_apply") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                std::uint32_t perkId = ParseHexFormID(value);
                PERK_ID_LIST.push_back(perkId);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Perk ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // -- Keyword Settings ---
        if (lowerLine.find("keyword_enabled") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "true" || value == "1") {
                KEYWORD_ENABLED = true;
            } else {
                KEYWORD_ENABLED = false;
            }
            continue;
        }
        if (lowerLine.find("keyword_to_apply") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                std::uint32_t keywordId = ParseHexFormID(value);
                KEYWORD_ID_LIST.push_back(keywordId);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Keyword ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Form IDs of actors to exlude from the mod ---
        if (lowerLine.find("exclude_actor_id_list") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                std::uint32_t actorId = ParseHexFormID(value);
                EXCLUDE_ACTOR_ID_LIST.push_back(actorId);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Actor ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // --- Threat Score Weights ---
        if (lowerLine.find("threat_weapon_bonus") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                THREAT_WEAPON_BONUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Threat Weapon Bonus value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("threat_legendary_bonus") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                THREAT_LEGENDARY_BONUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Threat Legendary Bonus value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("threat_unique_bonus") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                THREAT_UNIQUE_BONUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Threat Unique Bonus value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("threat_health_bonus") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                THREAT_HEALTH_BONUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Threat Health Bonus value: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("threat_alert_bonus") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                THREAT_ALERT_BONUS = std::stof(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Threat Alert Bonus value: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // DATA
        if (lowerLine.find("companion_faction_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                CURRENT_COMPANION_FACTION_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Companion Faction ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("actorvalue_hc_downed_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                ACTORVALUE_HC_DOWNED_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing ActorValue HC Downed ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("item_stimpak") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                ITEM_STIMPAK_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Stimpak Item ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("item_repairkit") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                ITEM_REPAIRKIT_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Repair Kit Item ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // Races for stimpak usage
        if (lowerLine.find("race_stimpak_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                std::uint32_t raceId = ParseHexFormID(value);
                RACE_STIMPAK_ID.push_back(raceId);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Stimpak Race ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // Synt3 component id
        if (lowerLine.find("race_synth3c_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                RACE_SYNTH3C_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Synt3 Component ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // Animation IDs
        if (lowerLine.find("anim_stimpak") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                IDLE_STIMPAK_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing Stimpak Animation ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        // Keywords
        if (lowerLine.find("kywd_armortypepower_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                KYWD_ARMORTYPEPOWER_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing PowerArmorType Keyword ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
        if (lowerLine.find("kywd_ispowerarmorframe_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                KYWD_ISPOWERARMORFRAME_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing IsPowerArmorFrame Keyword ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }

        // Packages
        if (lowerLine.find("pack_followerscompanion_id") == 0) {
            std::string value = GetValueFromLine(line);
            try {
                PACK_FOLLOWERSCOMPANION_ID = ParseHexFormID(value);
            } catch (const std::exception& e) {
                REX::WARN("LoadConfig: Error parsing FollowersCompanion Package ID: {}. Exception: {}", value, e.what());
            }
            continue;
        }
    }
    file.close();
    REX::INFO("LoadConfig: Completed loading config.");
    REX::INFO(" - Debugging: {}", DEBUGGING);
    REX::INFO(" - Update Interval: {} seconds", UPDATE_INTERVAL);
    REX::INFO(" - Reload ini every {} updates.", INI_RELOAD_INTERVAL);
    REX::INFO(" - Actor Search Radius: {}", ACTOR_SEARCH_RADIUS);
    REX::INFO(" - AI Behavior: Threshold={}, UsesStimpak={}, UseStimpakUnlimited={}, AutoRevive={}, FleeCombat={},  FleeDistance={}, EquipItems={}, EquipGear={}, EquipAmmoRefill={}, EquipAmmoAmount={}, StuckCheck={}, StuckThreshold={}, StuckCollisions={}, StuckSpeedThreshold={}, StuckDistance={}", AI_HEALTH_THRESHOLD, AI_USE_STIMPAK, AI_USE_STIMPAK_UNLIMITED, AI_AUTO_REVIVE, AI_FLEE_COMBAT, AI_FLEE_DISTANCE, AI_EQUIP_ITEMS, AI_EQUIP_GEAR, AI_EQUIP_AMMO_REFILL, AI_EQUIP_AMMO_AMOUNT, AI_STUCK_CHECK, AI_STUCK_THRESHOLD, AI_STUCK_COLLISIONS, AI_STUCK_SPEED,
              AI_STUCK_DISTANCE);
    REX::INFO(" - AI Aggression Settings: Enabled={}, All={}, AggressionSneak={}, AggressionRadius0={}, AggressionRadius1={}, AggressionRadius2={}", AI_AGGRESSION_ENABLED, AI_AGGRESSION_ALL, AI_AGGRESSION_SNEAK, AI_AGGRESSION_RADIUS0, AI_AGGRESSION_RADIUS1, AI_AGGRESSION_RADIUS2);
    REX::INFO(" - Chatter Settings: Enabled={}, Chatter Multiplier={}, Sneak Multiplier={}", CHATTER_ENABLED, CHATTER_MULTIPLIER, CHATTER_MULTIPLIER_SNEAK);
    REX::INFO(" - Combat AI: Enabled={}, Target={}, Offensive={}, Defensive={}, Ranged={}, Melee={}", COMBAT_ENABLED, COMBAT_TARGET, COMBAT_OFFENSIVE, COMBAT_DEFENSIVE, COMBAT_RANGED, COMBAT_MELEE);
    REX::INFO("   - Ranged Modifiers: Adjustment={}, Crouching={}, Strafe={}, Waiting={}, Accuracy={}", COMBAT_RANGED_ADJUSTMENT, COMBAT_RANGED_CROUCHING, COMBAT_RANGED_STRAFE, COMBAT_RANGED_WAITING, COMBAT_RANGED_ACCURACY);
    REX::INFO("   - Close-Quarters Modifiers: Fallback={}, Circle={}, Disengage={}, Flank={}, ThrowGrenade={}", COMBAT_CLOSE_FALLBACK, COMBAT_CLOSE_CIRCLE, COMBAT_CLOSE_DISENGAGE, COMBAT_CLOSE_FLANK, COMBAT_CLOSE_THROW_GRENADE);
    REX::INFO("   - Cover Modifiers: Distance={}", COMBAT_COVER_DISTANCE);
    REX::INFO(" - Loot Settings: Enabled={}, Combat={}, Radius={}, Junk={}, Ammo={}, Aid={}, MinValue={}, MaxValue={}, Steal={}, WeightLimit={}", LOOT_ENABLED, LOOT_COMBAT, LOOT_RADIUS, LOOT_JUNK, LOOT_AMMO, LOOT_AID, LOOT_MIN_VALUE, LOOT_MAX_VALUE, LOOT_STEAL, LOOT_WEIGHT_LIMIT);
    REX::INFO(" - XP Gain Settings: Enabled={}, Ratio={}, KillerTolerance={}", XP_ENABLED, XP_RATIO, XP_KILLER_TOLERANCE);
    REX::INFO(" - Buff Settings: Enabled={}, HealRate={}, CombatHealRate={}, DmgResist={}, FireResist={}, ElectricalResist={}, FrostResist={}, EnergyResist={}, PoisonResist={}, RadiationResist={}", BUFF_ENABLED, BUFF_HEAL_RATE, BUFF_COMBAT_HEAL_RATE, BUFF_DAMAGE_RESIST, BUFF_FIRE_RESIST, BUFF_ELECTRICAL_RESIST, BUFF_FROST_RESIST, BUFF_ENERGY_RESIST, BUFF_POISON_RESIST, BUFF_RADIATION_RESIST);
    REX::INFO(" - Buff Attributes: Agility={}, Endurance={}, Intelligence={}, Lockpick={}, Luck={}, Perception={}, Sneak={}, Strength={}, CarryWeight={}", BUFF_AGILITY, BUFF_ENDURANCE, BUFF_INTELLIGENCE, BUFF_LOCKPICK, BUFF_LUCK, BUFF_PERCEPTION, BUFF_SNEAK, BUFF_STRENGTH, BUFF_CARRYWEIGHT);
    REX::INFO(" - Power Armor Settings: Enabled={}, RepairAmount={}", PA_ENABLED, PA_REPAIR_AMOUNT);
    REX::INFO(" - Perk Settings: Enabled={}, PerkCount={}", PERK_ENABLED, PERK_ID_LIST.size());
    REX::INFO(" - Keyword Settings: Enabled={}, KeywordCount={}", KEYWORD_ENABLED, KEYWORD_ID_LIST.size());
    REX::INFO(" - Excluded Actors Count: {}", EXCLUDE_ACTOR_ID_LIST.size());
    REX::INFO(" - Threat Weights: Weapon={}, Legendary={}, Unique={}, Health={}, Alert={}", THREAT_WEAPON_BONUS, THREAT_LEGENDARY_BONUS, THREAT_UNIQUE_BONUS, THREAT_HEALTH_BONUS, THREAT_ALERT_BONUS);
    REX::INFO(" - Companion Faction ID: 0x{:08X}", CURRENT_COMPANION_FACTION_ID);
    REX::INFO(" - ActorValue HC Downed ID: 0x{:08X}", ACTORVALUE_HC_DOWNED_ID);
    REX::INFO(" - Stimpak Item ID: 0x{:08X}", ITEM_STIMPAK_ID);
    REX::INFO(" - Repair Kit Item ID: 0x{:08X}", ITEM_REPAIRKIT_ID);
    REX::INFO(" - Stimpak Race IDs Count: {}", RACE_STIMPAK_ID.size());
    REX::INFO(" - Synt3 Component ID: 0x{:08X}", RACE_SYNTH3C_ID);
    REX::INFO(" - Animation IDs: Stimpak=0x{:08X}", IDLE_STIMPAK_ID);
    REX::INFO(" - Keyword IDs: ArmorTypePower=0x{:08X}, IsPowerArmorFrame=0x{:08X}", KYWD_ARMORTYPEPOWER_ID, KYWD_ISPOWERARMORFRAME_ID);
    REX::INFO(" - Package IDs: FollowersCompanion=0x{:08X}", PACK_FOLLOWERSCOMPANION_ID);
}

// Message handler definition
void F4SEMessageHandler(F4SE::MessagingInterface::Message* a_message) {
    RE::BSTEventSource<RE::TESDeathEvent>* eventSourceDeath;
    switch (a_message->type) {
    case F4SE::MessagingInterface::kPostLoad:
        REX::INFO("Received kMessage_PostLoad. Game data is now loaded!");
        break;
    case F4SE::MessagingInterface::kPostPostLoad:
        REX::INFO("Received kMessage_PostPostLoad. Game data finished loading.");
        break;
    case F4SE::MessagingInterface::kGameDataReady:
        REX::INFO("Received kMessage_GameDataReady. Game data is ready.");
        // Get the global data handle and interfaces
        g_dataHandle = RE::TESDataHandler::GetSingleton();
        if (g_dataHandle) {
            REX::INFO("TESDataHandler singleton acquired successfully.");
        } else {
            REX::WARN("Failed to acquire TESDataHandler singleton.");
        }
        break;
    case F4SE::MessagingInterface::kPostLoadGame:
        REX::INFO("Received kMessage_PostLoadGame. A save game has been loaded.");
        // Register and start our periodic update task
        if (g_updateTimer.IsRunning()) {
            g_updateTimer.Stop();
        }
        if (!g_updateTimer.IsRunning()) {
            g_updateTimer.Start(UPDATE_INTERVAL, []() { Update_Internal(); });
            REX::INFO("Update timer started. Every {} seconds.", UPDATE_INTERVAL);
        }
        // Start movement timer (runs every 0.1 seconds = 10 times per second)
        if (g_movementTimer.IsRunning()) {
            g_movementTimer.Stop();
        }
        if (!g_movementTimer.IsRunning()) {
            g_movementTimer.Start(0.1f, []() { MovementSystem::ProcessCompanionTasks(0.1f); });
            REX::INFO("Companion system timer started (10Hz update rate).");
        }
        // Register death event sink for companion kill XP tracking
        eventSourceDeath = RE::TESDeathEvent::GetEventSource();
        if (eventSourceDeath) {
            eventSourceDeath->RegisterSink(CompanionKillEventSink::GetSingleton());
            REX::INFO("Successfully registered death event sink for companion kills.");
        } else {
            REX::WARN("Failed to get death event source.");
        }
        break;
    case F4SE::MessagingInterface::kNewGame:
        REX::INFO("Received kMessage_NewGame. A new game has been loaded.");
        // Register and start our periodic update task
        if (g_updateTimer.IsRunning()) {
            g_updateTimer.Stop();
        }
        if (!g_updateTimer.IsRunning()) {
            g_updateTimer.Start(UPDATE_INTERVAL, []() { Update_Internal(); });
            REX::INFO("Update timer started. Every {} seconds.", UPDATE_INTERVAL);
        }
        // Start movement timer (runs every 0.1 seconds = 10 times per second)
        if (g_movementTimer.IsRunning()) {
            g_movementTimer.Stop();
        }
        if (!g_movementTimer.IsRunning()) {
            g_movementTimer.Start(0.1f, []() { MovementSystem::ProcessCompanionTasks(0.1f); });
            REX::INFO("Companion system timer started (10Hz update rate).");
        }
        // Register death event sink for companion kill XP tracking
        eventSourceDeath = RE::TESDeathEvent::GetEventSource();
        if (eventSourceDeath) {
            eventSourceDeath->RegisterSink(CompanionKillEventSink::GetSingleton());
            REX::INFO("Successfully registered death event sink for companion kills.");
        } else {
            REX::WARN("Failed to get death event source.");
        }
        break;
    }
}

// --- F4SE Entry Points - MUST have C linkage for F4SE to find them ---
extern "C" { // This block ensures C-style (unmangled) names for the linker

F4SE_API bool F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    // Set the plugin information
    // This is crucial to load the plugin
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = Version::PROJECT.data();
    info->version = Version::MAJOR;

    // Set up the logger
    // F4SE::log::log_directory().value(); == Documents/My Games/F4SE/
    std::filesystem::path logPath = F4SE::log::log_directory().value();
    logPath = logPath.parent_path() / "Fallout4" / "F4SE" / std::format("{}.log", Version::PROJECT);
    // Create the file
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
    auto aLog = std::make_shared<spdlog::logger>("aLog"s, sink);
    // Configure the logger
    aLog->set_level(spdlog::level::info);
    aLog->flush_on(spdlog::level::info);
    // Set pattern
    aLog->set_pattern("[%T] [%^%l%$] %v"s);
    // Register to make it global accessable
    spdlog::register_logger(aLog);
    spdlog::set_default_logger(aLog);
    // Assign to global pointer
    gLog = spdlog::get("aLog");
    // First log
    REX::INFO("{}: Plugin Query started.", Version::PROJECT);

    // Minimum version 1.10.163
    const auto ver = f4se->RuntimeVersion();
    if (ver < F4SE::RUNTIME_1_10_162) {
        gLog->critical("unsupported runtime v{}", ver.string());
        return false;
    }

    return true;
}

// This function is called after F4SE has loaded all plugins and the game is
// about to start.
F4SE_API bool F4SEPlugin_Load(const F4SE::LoadInterface* f4se) {
    // Initialize the plugin with logger false to prevent F4SE to use its own
    // logger
    F4SE::Init(f4se, false);

    // Log information
    REX::INFO("{}: Plugin loaded!", Version::PROJECT);
    REX::INFO("F4SE version: {}", F4SE::GetF4SEVersion().string());
    REX::INFO("Game runtime version: {}", f4se->RuntimeVersion().string());

    // Load config
    LoadConfig();

    // Get the global plugin handle and interfaces
    g_pluginHandle = f4se->GetPluginHandle();
    g_taskInterface = F4SE::GetTaskInterface();
    g_papyrusInterface = F4SE::GetPapyrusInterface();
    g_messagingInterface = F4SE::GetMessagingInterface();

    // Register Papyrus functions
    if (g_papyrusInterface) {
        g_papyrusInterface->Register(RegisterPapyrusFunctions);
        REX::INFO("Papyrus functions registration callback successfully registered.");
    } else {
        REX::WARN("Failed to register Papyrus functions. This is critical for "
                  "native functions.");
    }

    // Set the messagehandler to listen to events
    if (g_messagingInterface && g_messagingInterface->RegisterListener(F4SEMessageHandler, "F4SE")) {
        REX::INFO("Registered F4SE message handler.");
    } else {
        REX::WARN("Failed to register F4SE message handler.");
        return false;
    }

    return true;
}

F4SE_API void F4SEPlugin_Release() {
    // This is a new function for cleanup. It is called when the plugin is
    // unloaded.
    REX::INFO("%s: Plugin released.", Version::PROJECT);
    gLog->flush();
    spdlog::drop_all();
}
}
