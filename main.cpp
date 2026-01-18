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
inline constexpr std::size_t MINOR = 6;
inline constexpr std::size_t PATCH = 0;
inline constexpr auto NAME = "0.6.0"sv;
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
// Module name
const char* MODULE_NAME = "CCBCL.dll";
const char* INI_NAME = "CCBCL.ini";
const char* INI_MCM_NAME = "MCMCCBCL.ini";
// Our update timers
CCB_RepeatingTimer g_updateTimer;
CCB_RepeatingTimer g_movementTimer;
// Global death handler registered flag
std::atomic<bool> g_deathHandlerRegistered = false;
// --- User Settings ---
// Global debug flag
bool DEBUGGING = false;
// Timer delay for systems that struggle to load (in seconds)
float TIMER_DELAY = 15.0f;
// Current game time
float CURRENT_GAME_TIME = 0.0f;
// Global update interval (in seconds)
float UPDATE_INTERVAL = 3.0f;
// Read the ini every x updates (0 = only on game start)
int INI_RELOAD_INTERVAL = 10;
// Actor search radius around the player in game units
float ACTOR_SEARCH_RADIUS = 8000.0f;
// General flags
bool COMBAT_SETTINGS = true;                 // Enable Combat Settings
bool FOLLOW_SETTINGS = true;                 // Enable Follow Settings
bool LOOTING_SETTINGS = true;                // Enable Looting Settings
bool ATTRIBUTES_SETTINGS = true;             // Enable Attributes Settings
bool OTHER_SETTINGS = true;                  // Enable Other Settings
// AI settings
float AI_HEALTH_THRESHOLD = 40.0f;
bool AI_USE_STIMPAK_ENABLED = true;
bool AI_USE_STIMPAK_UNLIMITED = false;
bool AI_AUTO_REVIVE = true;
bool AI_FLEE_COMBAT = true;
float AI_FLEE_DISTANCE = 500.0f;
// follow distance settings
bool AI_DISTANCE_ENABLED = true;
int AI_FOLLOW_DISTANCE_GENERAL = 1; // 0=Near, 1=Medium, 2=Far
int AI_FOLLOW_DISTANCE_INTERIORS= 0; // 0=Near, 1=Medium, 2=Far
float AI_FOLLOW_DISTANCE_NEAR = 500.0f;
float AI_FOLLOW_DISTANCE_MEDIUM = 1000.0f;
float AI_FOLLOW_DISTANCE_FAR = 1500.0f;
bool AI_SPEED_ENABLED = true;
float AI_FOLLOW_SPEED_NEAR = 1.0f;
float AI_FOLLOW_SPEED_MEDIUM = 1.5f;
float AI_FOLLOW_SPEED_FAR = 2.0f;
// Equipment settings
bool AI_EQUIP_ARMOR = false;
bool AI_EQUIP_WEAPON = false;
std::vector<int> AI_EQUIP_SLOTS = { 42, 43, 44, 45 }; // 33 Outfit, 30 Head, 41 [A]Body, 42 [A]Left Arm, 43 [A]Right Arm, 44 [A]Left Leg, 45 [A]Right Leg
bool AI_EQUIP_AMMO_REFILL = true;
int AI_EQUIP_AMMO_AMOUNT = 50;
// Movement settings
bool AI_STUCK_ENABLED = true;
int AI_STUCK_THRESHOLD = 20;
int AI_STUCK_COLLISIONS = 2;
float AI_STUCK_SPEED = 10.0f;
float AI_OVERSHOOT_SPEED = 250.0f;
// Aggression settings
bool AI_AGGRESSION_ENABLED = true;
bool AI_AGGRESSION_ZONEAWARE = true;
bool AI_AGGRESSION_ALL = true;
bool AI_AGGRESSION_SNEAK = false;
float AI_AGGRESSION_RADIUS0 = 1600.0f;
float AI_AGGRESSION_RADIUS1 = 1200.0f;
float AI_AGGRESSION_RADIUS2 = 800.0f;
// Chatter settings
bool CHATTER_ENABLED = false;
float CHATTER_MULTIPLIER = 1.0f;
float CHATTER_MULTIPLIER_SNEAK = 5.0f;
// Combat AI settings
bool COMBATSTYLE_ENABLED = true;
int COMBATSTYLE_TARGET = 0;
float COMBATSTYLE_OFFENSIVE = 0.5f;
float COMBATSTYLE_DEFENSIVE = 0.5f;
float COMBATSTYLE_RANGED_WEAPON = 1.0f;
float COMBATSTYLE_MELEE_WEAPON = 1.0f;
// Ranged
float COMBATSTYLE_RANGED_ADJUSTMENT = 1.0f;
float COMBATSTYLE_RANGED_CROUCHING = 1.0f;
float COMBATSTYLE_RANGED_STRAFE = 0.5f;
float COMBATSTYLE_RANGED_WAITING = 0.5f;
float COMBATSTYLE_RANGED_ACCURACY = 0.5f;
// Close-Quarters
float COMBATSTYLE_CLOSE_FALLBACK = 0.5f;
float COMBATSTYLE_CLOSE_CIRCLE = 0.5f;
float COMBATSTYLE_CLOSE_DISENGAGE = 0.5f;
float COMBATSTYLE_CLOSE_FLANK = 0.5f;
int COMBATSTYLE_CLOSE_THROW_GRENADE = 0;
// Cover
float COMBATSTYLE_COVER_DISTANCE = 0.5f;
// Loot item filter flag
bool LOOT_ENABLED = true;
bool LOOT_COMBAT = false;
float LOOT_RADIUS = 1000.0f;
bool LOOT_JUNK = true;
bool LOOT_AMMO = true;
bool LOOT_AID = true;
bool LOOT_GEAR = true;
int LOOT_MIN_VALUE = 0;
int LOOT_MAX_VALUE = 9999;
bool LOOT_JUNK_BREAKDOWN = true;
bool LOOT_STEAL = false;
bool LOOT_WEIGHT_LIMIT = true;
// XP gain settings
bool XP_ENABLED = true;
float XP_RATIO = 0.5f;
float XP_KILLER_TOLERANCE = 2000.0f;
// Buff settings
bool BUFF_ENABLED = true;
bool BUFF_SET_VALUES = false;
float BUFF_HEALTH = 100.0f;
float BUFF_HEAL_RATE = 1.0f;
float BUFF_COMBAT_HEAL_RATE = 1.0f;
float BUFF_DAMAGE_RESIST = 10.0f;
float BUFF_FIRE_RESIST = 10.0f;
float BUFF_ELECTRICAL_RESIST = 10.0f;
float BUFF_FROST_RESIST = 10.0f;
float BUFF_ENERGY_RESIST = 10.0f;
float BUFF_POISON_RESIST = 10.0f;
float BUFF_RADIATION_RESIST = 10.0f;
float BUFF_AGILITY = 1.0f;
float BUFF_ENDURANCE = 1.0f;
float BUFF_INTELLIGENCE = 1.0f;
float BUFF_LUCK = 1.0f;
float BUFF_PERCEPTION = 1.0f;
float BUFF_SNEAK = 30.0f;
float BUFF_STRENGTH = 1.0f;
float BUFF_LOCKPICK = 30.0f;
float BUFF_CARRYWEIGHT = 150.0f;
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
// ActorValues for follower state/distance/stance
std::uint32_t ACTORVALUE_FOLLOWERSTATE_ID = 0x00000344;   // 00000344 FollowerState
RE::ActorValueInfo* g_actorValueFollowerState = nullptr;
std::uint32_t ACTORVALUE_FOLLOWERDISTANCE_ID = 0x00000345; // 00000345 FollowerDistance
RE::ActorValueInfo* g_actorValueFollowerDistance = nullptr;
std::uint32_t ACTORVALUE_FOLLOWERSTANCE_ID = 0x00000346;  // 00000346 FollowerStance
RE::ActorValueInfo* g_actorValueFollowerStance = nullptr;
// Item forms
std::uint32_t ITEM_STIMPAK_ID = 0x00023736; // 00023736 Stimpak
RE::TESForm* g_itemStimpak = nullptr;
std::uint32_t ITEM_REPAIRKIT_ID = 0x00004F12; // 00004F12 Repair Kit
RE::TESForm* g_itemRepairKit = nullptr;
// Race forms for stimpak users
std::vector<std::uint32_t> RACE_STIMPAK_ID = {0x00013746, 0x000EAFB6}; // 00013746 Human, 000EAFB6 Ghoul
std::vector<RE::TESRace*> g_raceStimpak;
// IDLE animations
std::uint32_t IDLE_STIMPAK_ID = 0x000B1CF9; // 000B1CF9 3rdPUseStimpakOnSelf
RE::TESIdleForm* g_idleStimpak = nullptr;
// Keywords
std::uint32_t KYWD_ARMORTYPEPOWER_ID = 0x0004D8A1;    // 0004D8A1 ArmorTypePower
RE::BGSKeyword* g_kwdArmorTypePower = nullptr;
std::uint32_t KYWD_ISPOWERARMORFRAME_ID = 0x0003430B; // 0003430B isPowerArmorFrame
RE::BGSKeyword* g_kwdIsPowerArmorFrame = nullptr;
std::uint32_t KYWD_ANIMAL_ID = 0x00013798;          // 00013798 Animal
RE::BGSKeyword* g_kwdAnimal = nullptr;
std::uint32_t KYWD_ROBOT_ID = 0x0002CB73;          // 0002CB73 Robot
RE::BGSKeyword* g_kwdRobot = nullptr;
std::uint32_t KYWD_SYNTH_ID = 0x0010C3CE;          // 0010C3CE Synth
RE::BGSKeyword* g_kwdSynth = nullptr;
std::vector<std::uint32_t> KYWD_LOOTEXCLUDE_ID_LIST;
std::vector<RE::BGSKeyword*> g_kwdLootExclude;
// Packages
std::uint32_t PACK_FOLLOWERSCOMPANION_ID = 0x0002A101; // 0002A101 FollowersCompanion
RE::TESPackage* g_packFollowersCompanion = nullptr;
// GetSingletons
RE::ActorValue* g_actorValueSingleton = nullptr;
RE::ProcessLists* g_processListsSingleton = nullptr;
RE::ActorEquipManager* g_actorEquipMgrSingleton = nullptr;
RE::UI* g_uiSingleton = nullptr;
// Always Loot Forms
std::vector<std::uint32_t> ITEM_LOOTALWAYS_ID_LIST;
std::vector<RE::TESForm*> g_itemLootAlways;
// Lootable Form Types
std::vector<RE::ENUM_FORM_ID> LOOTABLE_FORM_TYPES = {
    RE::ENUM_FORM_ID::kCONT, // Containers
    RE::ENUM_FORM_ID::kACHR, // Actor and Dead bodies
    RE::ENUM_FORM_ID::kNPC_, // NPC
    RE::ENUM_FORM_ID::kARMO, // Armor
    RE::ENUM_FORM_ID::kWEAP, // Weapons
    RE::ENUM_FORM_ID::kAMMO, // Ammo
    RE::ENUM_FORM_ID::kMISC, // Junk/Misc
    RE::ENUM_FORM_ID::kALCH, // Chems/Food/Aid
    RE::ENUM_FORM_ID::kBOOK, // Magazines/Books
    RE::ENUM_FORM_ID::kKEYM, // Keys
    RE::ENUM_FORM_ID::kFURN  // Furniture
};
// Globals
std::uint32_t GLOBAL_COM_FOLLOW_ID=0x0002A106;      // 0002A106 FollowersComFollow 1.0
RE::TESGlobal* g_globalComFollow = nullptr;
std::uint32_t GLOBAL_COM_GOHOME_ID=0x0002A108;       // 0002A108 FollowersComGoHome 4.0
RE::TESGlobal* g_globalComGoHome = nullptr;
std::uint32_t GLOBAL_COM_WAIT_ID=0x0002A107;         // 0002A107 FollowersComWait 2.0
RE::TESGlobal* g_globalComWait = nullptr;
std::uint32_t GLOBAL_COM_DISTFAR_ENUM_ID=0x0002A10B;       // 0002A10B FollowersComDistFar 2.0
RE::TESGlobal* g_globalComDistFar = nullptr;
std::uint32_t GLOBAL_COM_DISTMEDIUM_ENUM_ID=0x0002A10A;     // 0002A10A FollowersComDistMedium 1.0
RE::TESGlobal* g_globalComDistMedium = nullptr;
std::uint32_t GLOBAL_COM_DISTNEAR_ENUM_ID=0x0002A109;     // 0002A109 FollowersComDistNear 0.0
RE::TESGlobal* g_globalComDistNear = nullptr;
// Follower stances
std::uint32_t GLOBAL_COM_STANCEAGGRO_ID=0x0002AE53;    // 0002AE53 FollowersComStanceAggro 1.0
RE::TESGlobal* g_globalComStanceAggro = nullptr;
std::uint32_t GLOBAL_COM_STANCECOMBATFALSE_ID=0x0002AE55; // 0002AE55 FollowersComStanceCombatFalse 0.0
RE::TESGlobal* g_globalComStanceCombatFalse = nullptr;
std::uint32_t GLOBAL_COM_STANCECOMBATTRUE_ID=0x0002AE56; // 0002AE56 FollowersComStanceCombatTrue 1.0
RE::TESGlobal* g_globalComStanceCombatTrue = nullptr;
std::uint32_t GLOBAL_COM_STANCEDEFENSIVE_ID=0x0002AE54; // 0002AE54 FollowersComStanceDefensive 0.0
RE::TESGlobal* g_globalComStanceDefensive = nullptr;
// Follower distances
std::uint32_t GLOBAL_COM_DISTFAR_VAL_ID=0x000F0D06;       // 000F0D06 FollowersComDistFar 1500.0
RE::TESGlobal* g_globalComDistFarVal = nullptr;
std::uint32_t GLOBAL_COM_DISTMEDIUM_VAL_ID=0x000F0D05;    // 000F0D05 FollowersComDistMedium 1000.0
RE::TESGlobal* g_globalComDistMediumVal = nullptr;
std::uint32_t GLOBAL_COM_DISTNEAR_VAL_ID=0x000F0D04;      // 000F0D04 FollowersComDistNear 500.0
RE::TESGlobal* g_globalComDistNearVal = nullptr;

// Helper function to extract key from a line in lower case
inline std::string GetKeyFromLine(const std::string& line) {
    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos)
        return "";
    // extract key
    std::string key = line.substr(0, eqPos);
    // remove trailing comments
    size_t semicolonPos = key.find(';');
    if (semicolonPos != std::string::npos)
        key = key.substr(0, semicolonPos);
    // Remove whitespace
    key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
    // Convert to lowercase for case-insensitive comparison
    return ToLower(key);
}
// Helper function to extract value from a line in lower case
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
    return ToLower(value);
}
// Helper to get the directory of the plugin DLL
std::string GetPluginDirectory(HMODULE hModule) {
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
}
// Helper to convert "true"/"1" to bool
bool ParseBool(const std::string& value) {
    std::string v = ToLower(value);
    return (v == "true" || v == "1");
}
// Helper to parse hex string to uint32_t
uint32_t ParseHexFormID(const std::string& hexStr) {
    return static_cast<uint32_t>(std::stoul(hexStr, nullptr, 16));
}
// Load the MCM configuration from INI file
void LoadMCMConfig() {
    // Get the DLL handle for this plugin
    HMODULE hModule = GetModuleHandleA(MODULE_NAME);
    std::filesystem::path pluginDirPath = GetPluginDirectory(hModule);
    auto mcmConfigPath = pluginDirPath.parent_path().parent_path().parent_path() / "MCM" / "Settings" / INI_MCM_NAME;
    // Remove cleaning for compatibility issues with Vortex hardlinks
    //mcmConfigPath = std::filesystem::weakly_canonical(mcmConfigPath);
    if (DEBUGGING)
        REX::INFO("LoadMCMConfig: Loading config from: {}", mcmConfigPath.string());
    if (!std::filesystem::exists(mcmConfigPath)) {
        REX::WARN("LoadMCMConfig: MCM config not found: {}", mcmConfigPath.string());
        return;
    }
    // Create our boolean settings map, can be expanded as needed
    std::unordered_map<std::string, bool*> boolSettings = {
        {"idebugging", &DEBUGGING},
        {"icombat_settings", &COMBAT_SETTINGS},
        {"ifollow_settings", &FOLLOW_SETTINGS},
        {"ilooting_settings", &LOOTING_SETTINGS},
        {"iattributes_settings", &ATTRIBUTES_SETTINGS},
        {"iother_settings", &OTHER_SETTINGS},
        {"iai_use_stimpak_enabled", &AI_USE_STIMPAK_ENABLED},
        {"iai_use_stimpak_unlimited", &AI_USE_STIMPAK_UNLIMITED},
        {"iai_auto_revive", &AI_AUTO_REVIVE},
        {"iai_flee_combat", &AI_FLEE_COMBAT},
        {"iai_equip_armor", &AI_EQUIP_ARMOR},
        {"iai_equip_weapon", &AI_EQUIP_WEAPON},
        {"iai_equip_ammo_refill", &AI_EQUIP_AMMO_REFILL},
        {"iai_aggression_enabled", &AI_AGGRESSION_ENABLED},
        {"iai_aggression_zoneaware", &AI_AGGRESSION_ZONEAWARE},
        {"iai_aggression_sneak", &AI_AGGRESSION_SNEAK},
        {"iai_distance_enabled", &AI_DISTANCE_ENABLED},
        {"iai_speed_enabled", &AI_SPEED_ENABLED},
        {"iai_stuck_enabled", &AI_STUCK_ENABLED},
        {"iloot_enabled", &LOOT_ENABLED},
        {"iloot_combat", &LOOT_COMBAT},
        {"iloot_weight_limit", &LOOT_WEIGHT_LIMIT},
        {"iloot_junk", &LOOT_JUNK},
        {"iloot_ammo", &LOOT_AMMO},
        {"iloot_aid", &LOOT_AID},
        {"iloot_gear", &LOOT_GEAR},
        {"iloot_junk_breakdown", &LOOT_JUNK_BREAKDOWN},
        {"iloot_steal", &LOOT_STEAL},
        {"ixp_enabled", &XP_ENABLED},
        {"ichatter_enabled", &CHATTER_ENABLED},
        {"ibuff_enabled", &BUFF_ENABLED},
        {"ikeyword_enabled", &KEYWORD_ENABLED},
        {"ibuff_set_values", &BUFF_SET_VALUES},
        {"ipa_enabled", &PA_ENABLED},
        {"icombatstyle_enabled", &COMBATSTYLE_ENABLED},
        {"iperk_enabled", &PERK_ENABLED}
    };
    std::unordered_map<std::string, int*> intMap = {
        {"iai_follow_distance_general", &AI_FOLLOW_DISTANCE_GENERAL},
        {"iai_follow_distance_interiors", &AI_FOLLOW_DISTANCE_INTERIORS}
    };
    // Open file read only
    std::ifstream file(mcmConfigPath, std::ios::in);
    if (!file.is_open()) {
        REX::WARN("LoadMCMConfig: Could not open MCM INI file: {}", mcmConfigPath.string());
        return;
    }
    // Read the lines
    std::string line;
    while (std::getline(file, line)) {
        // Skip for empty lines or comments starting with ';' or section headers '['
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == ';' || line[first] == '[')
            continue;
        // Extract Key and Value
        std::string key = GetKeyFromLine(line);
        std::string valStr = GetValueFromLine(line);
        if (key.empty())
            continue;
        if (boolSettings.count(key)) {
            *boolSettings[key] = ParseBool(valStr);
            if (DEBUGGING)
                REX::INFO("MCM Config: {} set to {}", key, *boolSettings[key]);
        } else if (intMap.count(key)) {
            try {
                *intMap[key] = std::stoi(valStr);
                if (DEBUGGING)
                    REX::INFO("MCM Config: {} set to {}", key, *intMap[key]);
            } catch (const std::exception& e) {
                REX::WARN("MCM Config: Failed to parse int for key {}: {}", key, e.what());
            }
        }
    }
}
// Load the plugin configuration from INI file
void LoadConfig() {
    HMODULE hModule = GetModuleHandleA(MODULE_NAME);
    std::filesystem::path configPath = std::filesystem::path(GetPluginDirectory(hModule)) / INI_NAME;
    REX::INFO("LoadConfig: Loading config from: {}", configPath.string());
    // Boolean Map
    std::unordered_map<std::string, bool*> boolMap = {
        {"debugging", &DEBUGGING}, {"combat_settings", &COMBAT_SETTINGS},
        {"follow_settings", &FOLLOW_SETTINGS}, {"looting_settings", &LOOTING_SETTINGS},
        {"attributes_settings", &ATTRIBUTES_SETTINGS}, {"other_settings", &OTHER_SETTINGS},
        {"ai_use_stimpak_enabled", &AI_USE_STIMPAK_ENABLED}, {"ai_distance_enabled", &AI_DISTANCE_ENABLED},
        {"ai_use_stimpak_unlimited", &AI_USE_STIMPAK_UNLIMITED}, {"ai_auto_revive", &AI_AUTO_REVIVE},
        {"ai_flee_combat", &AI_FLEE_COMBAT}, {"ai_equip_armor", &AI_EQUIP_ARMOR},
        {"ai_equip_weapon", &AI_EQUIP_WEAPON}, {"ai_equip_ammo_refill", &AI_EQUIP_AMMO_REFILL},
        {"ai_stuck_enabled", &AI_STUCK_ENABLED}, {"ai_aggression_enable", &AI_AGGRESSION_ENABLED},
        {"ai_aggression_zoneaware", &AI_AGGRESSION_ZONEAWARE}, {"ai_speed_enabled", &AI_SPEED_ENABLED},
        {"ai_aggression_all", &AI_AGGRESSION_ALL}, {"ai_aggression_sneak", &AI_AGGRESSION_SNEAK},
        {"chatter_enabled", &CHATTER_ENABLED}, {"combatstyle_enabled", &COMBATSTYLE_ENABLED},
        {"loot_enabled", &LOOT_ENABLED}, {"loot_combat", &LOOT_COMBAT},
        {"loot_junk", &LOOT_JUNK}, {"loot_ammo", &LOOT_AMMO}, {"loot_aid", &LOOT_AID},
        {"loot_gear", &LOOT_GEAR}, {"loot_junk_breakdown", &LOOT_JUNK_BREAKDOWN},
        {"loot_steal", &LOOT_STEAL}, {"loot_weight_limit", &LOOT_WEIGHT_LIMIT},
        {"xp_enabled", &XP_ENABLED}, {"pa_enabled", &PA_ENABLED},
        {"buff_enabled", &BUFF_ENABLED}, {"buff_set_values", &BUFF_SET_VALUES},
        {"perk_enabled", &PERK_ENABLED}, {"keyword_enabled", &KEYWORD_ENABLED}
    };
    // Float Map
    std::unordered_map<std::string, float*> floatMap = {
        {"timer_delay", &TIMER_DELAY}, {"update_interval", &UPDATE_INTERVAL},
        {"actor_search_radius", &ACTOR_SEARCH_RADIUS}, {"ai_health_threshold", &AI_HEALTH_THRESHOLD},
        {"ai_flee_distance", &AI_FLEE_DISTANCE}, {"ai_stuck_speed", &AI_STUCK_SPEED}, 
        {"ai_overshoot_speed", &AI_OVERSHOOT_SPEED},
        {"ai_follow_distance_near", &AI_FOLLOW_DISTANCE_NEAR}, {"ai_follow_distance_medium", &AI_FOLLOW_DISTANCE_MEDIUM},
        {"ai_follow_distance_far", &AI_FOLLOW_DISTANCE_FAR}, {"ai_follow_speed_near", &AI_FOLLOW_SPEED_NEAR},
        {"ai_follow_speed_medium", &AI_FOLLOW_SPEED_MEDIUM}, {"ai_follow_speed_far", &AI_FOLLOW_SPEED_FAR},
        {"ai_aggression_radius0", &AI_AGGRESSION_RADIUS0},
        {"ai_aggression_radius1", &AI_AGGRESSION_RADIUS1}, {"ai_aggression_radius2", &AI_AGGRESSION_RADIUS2},
        {"chatter_multiplier", &CHATTER_MULTIPLIER}, {"chatter_multiplier_sneak", &CHATTER_MULTIPLIER_SNEAK},
        {"combat_offensive", &COMBATSTYLE_OFFENSIVE}, {"combat_defensive", &COMBATSTYLE_DEFENSIVE},
        {"combat_ranged_weapon", &COMBATSTYLE_RANGED_WEAPON}, {"combat_melee_weapon", &COMBATSTYLE_MELEE_WEAPON},
        {"combat_ranged_adjustment", &COMBATSTYLE_RANGED_ADJUSTMENT}, {"combat_ranged_crouching", &COMBATSTYLE_RANGED_CROUCHING},
        {"combat_ranged_strafe", &COMBATSTYLE_RANGED_STRAFE}, {"combat_ranged_waiting", &COMBATSTYLE_RANGED_WAITING},
        {"combat_ranged_accuracy", &COMBATSTYLE_RANGED_ACCURACY}, {"combat_close_fallback", &COMBATSTYLE_CLOSE_FALLBACK},
        {"combat_close_circle", &COMBATSTYLE_CLOSE_CIRCLE}, {"combat_close_disengage", &COMBATSTYLE_CLOSE_DISENGAGE},
        {"combat_close_flank", &COMBATSTYLE_CLOSE_FLANK}, {"combat_cover_distance", &COMBATSTYLE_COVER_DISTANCE},
        {"loot_radius", &LOOT_RADIUS}, {"xp_ratio", &XP_RATIO}, {"xp_killer_tolerance", &XP_KILLER_TOLERANCE},
        {"pa_repair_amount", &PA_REPAIR_AMOUNT}, {"buff_heal_rate", &BUFF_HEAL_RATE}, {"buff_health", &BUFF_HEALTH},
        {"buff_combat_heal_rate", &BUFF_COMBAT_HEAL_RATE}, {"buff_damage_resist", &BUFF_DAMAGE_RESIST},
        {"buff_fire_resist", &BUFF_FIRE_RESIST}, {"buff_electrical_resist", &BUFF_ELECTRICAL_RESIST},
        {"buff_frost_resist", &BUFF_FROST_RESIST}, {"buff_energy_resist", &BUFF_ENERGY_RESIST},
        {"buff_poison_resist", &BUFF_POISON_RESIST}, {"buff_radiation_resist", &BUFF_RADIATION_RESIST},
        {"buff_agility", &BUFF_AGILITY}, {"buff_endurance", &BUFF_ENDURANCE},
        {"buff_intelligence", &BUFF_INTELLIGENCE}, {"buff_lockpick", &BUFF_LOCKPICK},
        {"buff_luck", &BUFF_LUCK}, {"buff_perception", &BUFF_PERCEPTION},
        {"buff_strength", &BUFF_STRENGTH}, {"buff_sneak", &BUFF_SNEAK},
        {"buff_carryweight", &BUFF_CARRYWEIGHT}, {"threat_weapon_bonus", &THREAT_WEAPON_BONUS},
        {"threat_legendary_bonus", &THREAT_LEGENDARY_BONUS}, {"threat_unique_bonus", &THREAT_UNIQUE_BONUS},
        {"threat_health_bonus", &THREAT_HEALTH_BONUS}, {"threat_alert_bonus", &THREAT_ALERT_BONUS}
    };
    // Integer Map
    std::unordered_map<std::string, int*> intMap = {
        {"ini_reload_interval", &INI_RELOAD_INTERVAL}, {"ai_equip_ammo_amount", &AI_EQUIP_AMMO_AMOUNT},
        {"ai_stuck_threshold", &AI_STUCK_THRESHOLD}, {"ai_stuck_collisions", &AI_STUCK_COLLISIONS},
        {"ai_follow_distance_general", &AI_FOLLOW_DISTANCE_GENERAL}, {"ai_follow_distance_interiors", &AI_FOLLOW_DISTANCE_INTERIORS},
        {"combat_target", &COMBATSTYLE_TARGET}, {"combat_close_throw_grenade", &COMBATSTYLE_CLOSE_THROW_GRENADE},
        {"loot_min_value", &LOOT_MIN_VALUE}, {"loot_max_value", &LOOT_MAX_VALUE}
    };
    // Hex Map for single uint32_t values
    std::unordered_map<std::string, uint32_t*> hexMap = {
        {"current_companion_faction_id", &CURRENT_COMPANION_FACTION_ID},
        {"actorvalue_hc_downed_id", &ACTORVALUE_HC_DOWNED_ID},
        {"actorvalue_followerstate_id", &ACTORVALUE_FOLLOWERSTATE_ID},
        {"actorvalue_followerdistance_id", &ACTORVALUE_FOLLOWERDISTANCE_ID},
        {"actorvalue_followerstance_id", &ACTORVALUE_FOLLOWERSTANCE_ID},
        {"item_stimpak_id", &ITEM_STIMPAK_ID},
        {"item_repairkit_id", &ITEM_REPAIRKIT_ID},
        {"idle_stimpak_id", &IDLE_STIMPAK_ID},
        {"kywd_armortypepower_id", &KYWD_ARMORTYPEPOWER_ID},
        {"kywd_ispowerarmorframe_id", &KYWD_ISPOWERARMORFRAME_ID},
        {"kywd_animal_id", &KYWD_ANIMAL_ID},
        {"kywd_robot_id", &KYWD_ROBOT_ID},
        {"kywd_synth_id", &KYWD_SYNTH_ID},
        {"pack_followerscompanion_id", &PACK_FOLLOWERSCOMPANION_ID},
        // Follower Global Variables (States)
        {"global_com_follow_id", &GLOBAL_COM_FOLLOW_ID},
        {"global_com_gohome_id", &GLOBAL_COM_GOHOME_ID},
        {"global_com_wait_id", &GLOBAL_COM_WAIT_ID},
        {"global_com_distfar_id", &GLOBAL_COM_DISTFAR_ENUM_ID},
        {"global_com_distmedium_id", &GLOBAL_COM_DISTMEDIUM_ENUM_ID},
        {"global_com_distnear_id", &GLOBAL_COM_DISTNEAR_ENUM_ID},
        // Follower stances
        {"global_com_stanceaggro_id", &GLOBAL_COM_STANCEAGGRO_ID},
        {"global_com_stancecombatfalse_id", &GLOBAL_COM_STANCECOMBATFALSE_ID},
        {"global_com_stancecombattrue_id", &GLOBAL_COM_STANCECOMBATTRUE_ID},
        {"global_com_stancedefensive_id", &GLOBAL_COM_STANCEDEFENSIVE_ID},
        // Follower distances (The static floats)
        {"global_com_distfar_val_id", &GLOBAL_COM_DISTFAR_VAL_ID},
        {"global_com_distmedium_val_id", &GLOBAL_COM_DISTMEDIUM_VAL_ID},
        {"global_com_distnear_val_id", &GLOBAL_COM_DISTNEAR_VAL_ID}
    };
    // Vector Map for lists of uint32_t
    std::unordered_map<std::string, std::vector<uint32_t>*> vectorMap = {
        {"perk_to_apply", &PERK_ID_LIST},
        {"keyword_to_apply", &KEYWORD_ID_LIST},
        {"exclude_actor_id_list", &EXCLUDE_ACTOR_ID_LIST},
        {"race_stimpak_id", &RACE_STIMPAK_ID},
        {"kywd_lootexclude_id", &KYWD_LOOTEXCLUDE_ID_LIST},
        {"item_lootalways_id", &ITEM_LOOTALWAYS_ID_LIST}
    };
    // Keep track of which vectors have been cleared
    std::set<std::string> clearedVectors;
    // First try to open the stream directly read only
    std::ifstream file(configPath, std::ios::in);
    // Check if the file opened successfully
    if (!file.is_open()) {
        REX::WARN("LoadConfig: Could not open INI file: {}. Creating default.", configPath.string());
        // Create the file with defaultIni contents if it doesn't exist
        std::ofstream out(configPath);
        if (out.is_open()) {
            out << defaultIni;
            out.flush();
            out.close();
            REX::INFO("LoadConfig: Default INI created at: {}", configPath.string());
        } else {
            REX::WARN("LoadConfig: Failed to create default INI at: {}", configPath.string());
            return;
        }
        // Clear the failed stream state
        file.clear();
        // Try to open again for reading and give up if it fails again
        file.open(configPath, std::ios::in);
        if (!file.is_open()) {
            REX::WARN("LoadConfig: Still could not open INI file after creating default: {}", configPath.string());
            return;
        }
    }
    // Read the lines
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Skip for empty lines or comments starting with ';' or section headers '['
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == ';' || line[first] == '[') continue;
            // Extract Key and Value
            std::string key = GetKeyFromLine(line); // Already ToLower inside helper
            std::string val = GetValueFromLine(line);
            if (key.empty()) continue;
            // Process based on type
            if (boolMap.count(key)) { 
                *boolMap[key] = (ToLower(val) == "true" || val == "1"); 
            }
            else if (floatMap.count(key)) { 
                try { *floatMap[key] = std::stof(val); } catch (...) {} 
            }
            else if (intMap.count(key)) { 
                try { *intMap[key] = std::stoi(val); } catch (...) {} 
            }
            else if (hexMap.count(key)) { 
                try { *hexMap[key] = std::stoul(val, nullptr, 16); } catch (...) {} 
            }
            else if (vectorMap.count(key)) {
                if (clearedVectors.find(key) == clearedVectors.end()) {
                    vectorMap[key]->clear();
                    clearedVectors.insert(key);
                }
                try { vectorMap[key]->push_back(std::stoul(val, nullptr, 16)); } catch (...) {}
            }
            // Special Case: Comma-separated Integers
            else if (key == "ai_equip_slots") {
                if (clearedVectors.find(key) == clearedVectors.end()) {
                    AI_EQUIP_SLOTS.clear();
                    clearedVectors.insert(key);
                }
                std::stringstream ss(val);
                std::string s;
                while (std::getline(ss, s, ',')) { 
                    try { AI_EQUIP_SLOTS.push_back(std::stoi(s)); } catch (...) {} 
                }
            }
            // Special Case: Form Type Strings (Using native game function)
            else if (key == "form_type_loot") {
                if (clearedVectors.find(key) == clearedVectors.end()) {
                    LOOTABLE_FORM_TYPES.clear();
                    clearedVectors.insert(key);
                }
                try {
                    RE::ENUM_FORM_ID formType = RE::TESForm::GetFormTypeFromString(val.c_str());
                    LOOTABLE_FORM_TYPES.push_back(formType);
                } catch (...) {}
            }
        }
    }
    file.close();
    REX::INFO("--- CCBCL.ini Configuration Values ---");
    // Log Booleans
    for (auto const& [key, ptr] : boolMap) {
        REX::INFO(" [BOOL]  {:.<30} {}", key, *ptr);
    }
    // Log Floats
    for (auto const& [key, ptr] : floatMap) {
        REX::INFO(" [FLOAT] {:.<30} {:.2f}", key, *ptr);
    }
    // Log Ints
    for (auto const& [key, ptr] : intMap) {
        REX::INFO(" [INT]   {:.<30} {}", key, *ptr);
    }
    // Log Single Hex IDs
    for (auto const& [key, ptr] : hexMap) {
        REX::INFO(" [HEX]   {:.<30} 0x{:08X}", key, *ptr);
    }
    // Log Vectors (Counts and Values)
    for (auto const& [key, vecPtr] : vectorMap) {
        std::stringstream ss;
        for (uint32_t id : *vecPtr) { ss << "0x" << std::uppercase << std::hex << id << " "; }
        REX::INFO(" [VEC]   {} = Count: {} | IDs: {}", key, vecPtr->size(), ss.str());
    }
    // Log Special Cases
    REX::INFO(" [VEC]   {:.<30} Count: {}", "ai_equip_slots", AI_EQUIP_SLOTS.size());
    REX::INFO(" [VEC]   {:.<30} Count: {}", "form_type_loot", LOOTABLE_FORM_TYPES.size());
    REX::INFO("---------------------------------------");
    REX::INFO("LoadConfig: Complete.");
}

// Helper function to initialize timers and event sinks
void InitializeTimersAndEvents() {
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
    RE::BSTEventSource<RE::TESDeathEvent>* eventSourceDeath = RE::TESDeathEvent::GetEventSource();
    if (eventSourceDeath) {
        eventSourceDeath->RegisterSink(CompanionKillEventSink::GetSingleton());
        REX::INFO("Successfully registered death event sink for companion kills.");
    } else {
        REX::WARN("Failed to get death event source.");
    }
}

// Delayed timer start helper
void ScheduleDelayedInitialization(float delaySeconds) {
    // Static timer persists across function calls
    static CCB_RepeatingTimer initDelayTimer;
    static bool hasBeenScheduled = false;
    // Prevent multiple schedules
    if (hasBeenScheduled) {
        REX::WARN("Delayed initialization already scheduled, ignoring duplicate call.");
        return;
    }
    hasBeenScheduled = true;
    // Start a timer that will fire once after the delay
    initDelayTimer.Start(delaySeconds, []() {
        static bool hasInitialized = false;
        // Only initialize once
        if (!hasInitialized) {
            hasInitialized = true;
            REX::INFO("Delayed initialization executing after wait period...");
            InitializeTimersAndEvents();
            // Stop this timer since we only needed it once
            // (Note: accessing the static timer from the parent scope)
            static CCB_RepeatingTimer* timerPtr = nullptr;
            if (timerPtr) {
                timerPtr->Stop();
            }
        }
    });
    REX::INFO("Scheduled delayed initialization to start in {} seconds.", delaySeconds);
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
        // Register and start our periodic update task and events
        ScheduleDelayedInitialization(TIMER_DELAY);
        break;
    case F4SE::MessagingInterface::kNewGame:
        REX::INFO("Received kMessage_NewGame. A new game has been loaded.");
        // Schedule timer initialization with a delay to avoid race conditions
        // during new game world initialization
        ScheduleDelayedInitialization(TIMER_DELAY);
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
