#pragma once
#include <PCH.h>

#include <Plugin.h>

// Helper function to convert string to lowercase
inline std::string ToLower(const std::string &str)
{
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// --- 1.10.163 ---
// Global logger pointer
extern std::shared_ptr<spdlog::logger> gLog;

// --- General F4SE Definitions ---
// Global data handler
extern RE::TESDataHandler *g_dataHandle;
// Declare the F4SEMessagingInterface and F4SEScaleformInterface
extern const F4SE::MessagingInterface *g_messagingInterface;
// Declare the F4SEPapyrusInterface
extern const F4SE::PapyrusInterface *g_papyrusInterface;
// Declare the F4SETaskInterface
extern const F4SE::TaskInterface *g_taskInterface;

// --- Global Variables ---
// Expose config load function
void LoadConfig();
// Timers
extern CCB_RepeatingTimer g_updateTimer;
extern CCB_RepeatingTimer g_movementTimer;
// --- User Settings ---
// Default ini file
extern const char *defaultIni;
// Global debug flag
extern bool DEBUGGING;
// Current game time
extern float CURRENT_GAME_TIME;
// Global update interval (in seconds)
extern float UPDATE_INTERVAL;
// Read the ini every x updates (0 = only on game start)
extern int INI_RELOAD_INTERVAL;
// Actor search radius around the player in game units
extern float ACTOR_SEARCH_RADIUS;
// AI behavior settings
extern float AI_HEALTH_THRESHOLD;
extern bool AI_USE_STIMPAK;
extern bool AI_USE_STIMPAK_UNLIMITED;
extern bool AI_AUTO_REVIVE;
extern bool AI_FLEE_COMBAT;
extern float AI_FLEE_DISTANCE;
extern bool AI_EQUIP_ITEMS;
extern bool AI_EQUIP_GEAR;
extern bool AI_EQUIP_AMMO_REFILL;
extern int AI_EQUIP_AMMO_AMOUNT;
// Movement settings
extern bool AI_STUCK_CHECK;
extern int AI_STUCK_THRESHOLD;
extern int AI_STUCK_COLLISIONS;
extern float AI_STUCK_SPEED;
extern float AI_STUCK_DISTANCE;
// Aggression settings
extern bool AI_AGGRESSION_ENABLED;
extern bool AI_AGGRESSION_ALL;
extern bool AI_AGGRESSION_SNEAK;
extern float AI_AGGRESSION_RADIUS0;
extern float AI_AGGRESSION_RADIUS1;
extern float AI_AGGRESSION_RADIUS2;
// Chatter settings
extern bool CHATTER_ENABLED;
extern float CHATTER_MULTIPLIER;
extern float CHATTER_MULTIPLIER_SNEAK;
// Combat AI settings
extern bool COMBAT_ENABLED;
extern int COMBAT_TARGET;
extern float COMBAT_OFFENSIVE;
extern float COMBAT_DEFENSIVE;
extern float COMBAT_RANGED;
extern float COMBAT_MELEE;
// Ranged
extern float COMBAT_RANGED_ADJUSTMENT;
extern float COMBAT_RANGED_CROUCHING;
extern float COMBAT_RANGED_STRAFE;
extern float COMBAT_RANGED_WAITING;
extern float COMBAT_RANGED_ACCURACY;
// Close-Quarters
extern float COMBAT_CLOSE_FALLBACK;
extern float COMBAT_CLOSE_CIRCLE;
extern float COMBAT_CLOSE_DISENGAGE;
extern float COMBAT_CLOSE_FLANK;
extern int COMBAT_CLOSE_THROW_GRENADE;
// Cover
extern float COMBAT_COVER_DISTANCE;
// Loot item filter flag
extern bool LOOT_ENABLED;
extern bool LOOT_COMBAT;
extern float LOOT_RADIUS;
extern bool LOOT_JUNK;
extern bool LOOT_AMMO;
extern bool LOOT_AID;
extern int LOOT_MIN_VALUE;
extern int LOOT_MAX_VALUE;
extern bool LOOT_STEAL;
extern bool LOOT_WEIGHT_LIMIT;
// XP gain settings
extern bool XP_ENABLED;
extern float XP_RATIO;
extern float XP_KILLER_TOLERANCE;
// Buff settings
extern bool BUFF_ENABLED;
extern float BUFF_HEAL_RATE;
extern float BUFF_COMBAT_HEAL_RATE;
extern float BUFF_DAMAGE_RESIST;
extern float BUFF_FIRE_RESIST;
extern float BUFF_ELECTRICAL_RESIST;
extern float BUFF_FROST_RESIST;
extern float BUFF_ENERGY_RESIST;
extern float BUFF_POISON_RESIST;
extern float BUFF_RADIATION_RESIST;
// Buff attribute settings
extern float BUFF_AGILITY;
extern float BUFF_ENDURANCE;
extern float BUFF_INTELLIGENCE;
extern float BUFF_LOCKPICK;
extern float BUFF_LUCK;
extern float BUFF_PERCEPTION;
extern float BUFF_SNEAK;
extern float BUFF_STRENGTH;
extern float BUFF_CARRYWEIGHT;
// Power Armor Settings
extern bool PA_ENABLED;
extern float PA_REPAIR_AMOUNT;
// Perk settings
extern bool PERK_ENABLED;
extern std::vector<std::uint32_t> PERK_ID_LIST;
extern std::vector<RE::BGSPerk *> g_perkList;
// --- Keyword settings ---
extern bool KEYWORD_ENABLED;
extern std::vector<std::uint32_t> KEYWORD_ID_LIST;
extern std::vector<RE::BGSKeyword *> g_keywordList;
// --- Actor Exclusion List ---
// List of Actor FormIDs to exclude from mod effects
extern std::vector<std::uint32_t> EXCLUDE_ACTOR_ID_LIST;
// Threat score weights
extern float THREAT_WEAPON_BONUS;
extern float THREAT_LEGENDARY_BONUS;
extern float THREAT_UNIQUE_BONUS;
extern float THREAT_HEALTH_BONUS;
extern float THREAT_ALERT_BONUS;
// --- DATA ---
// Current companion faction ID
extern std::uint32_t CURRENT_COMPANION_FACTION_ID;
extern RE::TESFaction *g_companionFaction;
// ActorValue for HC downed state
extern std::uint32_t ACTORVALUE_HC_DOWNED_ID;
extern RE::ActorValueInfo *g_actorValueHCDowned;
// Item forms
extern std::uint32_t ITEM_STIMPAK_ID;
extern RE::TESForm *g_itemStimpak;
// Repair Kit
extern std::uint32_t ITEM_REPAIRKIT_ID;
extern RE::TESForm *g_itemRepairKit;
// Race forms for stimpak users
extern std::vector<std::uint32_t> RACE_STIMPAK_ID;
extern std::vector<RE::TESRace *> g_raceStimpak;
// Synth 3 component
extern std::uint32_t RACE_SYNTH3C_ID;
extern RE::TESObjectMISC *g_raceSynth3C;
// IDLE animations
extern std::uint32_t IDLE_STIMPAK_ID;
extern RE::TESIdleForm *g_idleStimpak;
// Keywords
extern std::uint32_t KYWD_ARMORTYPEPOWER_ID;
extern RE::BGSKeyword* g_kwdArmorTypePower;
extern std::uint32_t KYWD_ISPOWERARMORFRAME_ID;
extern RE::BGSKeyword* g_kwdIsPowerArmorFrame;
// Packages
extern std::uint32_t PACK_FOLLOWERSCOMPANION_ID;
extern RE::TESPackage* g_packFollowersCompanion;

// REX Logging Compatibility
#undef ERROR
namespace REX
{
    template <class... Args> void INFO(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->info(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void WARN(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->warn(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void ERROR(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->error(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void CRITICAL(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->critical(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void DEBUG(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->debug(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void TRACE(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->trace(a_fmt, std::forward<Args>(a_args)...);
    }
} // namespace REX

// Lookup a form by its ID and the file it belongs to
template <typename T>
T* GetFormByFileAndID_Internal(std::uint32_t formID, const char* fileName = nullptr) {
    // No file name provided, return form by global ID
    if (!fileName || strlen(fileName) == 0) {
        return RE::TESForm::GetFormByID<T>(formID);
    }
    // Resolve the form ID within the specified file
    if (!g_dataHandle) return nullptr;
    auto resolvedFormID = g_dataHandle->LookupFormID(formID, fileName);
    return RE::TESForm::GetFormByID<T>(resolvedFormID);
}