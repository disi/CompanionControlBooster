#pragma once
#include <Global.h>

// --- STRUCTS ---
// Structure to hold all actor states
struct ActorStateData
{
    std::uint32_t lifeState;       // LIFE_STATE (0-8)
    std::uint32_t weaponState;     // WEAPON_STATE (0-5)
    std::uint32_t gunState;        // GUN_STATE (0-8)
    std::uint32_t interactingState; // INTERACTING_STATE (0-3)
};

// ========================================================================================
// ACTOR STATE CHECKING FUNCTIONS
// ========================================================================================
// 
// LIFE_STATE enum values (from Actor.h):
//   - kAlive (0)         : Actor is alive and functioning normally
//   - kDying (1)         : Actor is in the process of dying (death animation playing)
//   - kDead (2)          : Actor is fully dead
//   - kUnconscious (3)   : Actor is knocked out/unconscious
//   - kReanimate (4)     : Actor has been reanimated (necromancy)
//   - kRecycle (5)       : Actor is being recycled by the engine
//   - kRestrained (6)    : Actor is restrained/captured
//   - kEssentialDown (7) : Essential actor is downed (protected, can't die)
//   - kBleedout (8)      : Actor is in bleedout state (dying but can be saved)
//
// GUN_STATE enum values (from Actor.h):
//   - kDrawn (0)         : Weapon is drawn and ready
//   - kRelaxed (1)       : Weapon drawn but in relaxed pose
//   - kBlocked (2)       : Weapon blocked/unavailable
//   - kAlert (3)         : Weapon drawn and on alert
//   - kReloading (4)     : Currently reloading weapon
//   - kThrowing (5)      : Throwing grenade/throwable
//   - kSighted (6)       : Aiming down sights (iron sights/scope)
//   - kFire (7)          : Currently firing weapon
//   - kFireSighted (8)   : Firing while aiming down sights
//
// WEAPON_STATE enum values (from Actor.h):
//   - kSheathed (0)      : Weapon is put away
//   - kWantToDraw (1)    : Beginning to draw weapon
//   - kDrawing (2)       : Animation of drawing weapon
//   - kDrawn (3)         : Weapon is fully drawn
//   - kWantToSheathe (4) : Beginning to sheathe weapon
//   - kSheathing (5)     : Animation of sheathing weapon
//
// INTERACTING_STATE enum values (from Actor.h):
//   - kNotInteracting (0)         : Not interacting with any object
//   - kWaitingToInteract (1)      : Queued to interact with object
//   - kInteracting (2)            : Currently interacting with object
//   - kWaitingToStopInteracting (3): Finishing interaction
//
// ========================================================================================
namespace ACTOR_STATE
{
    // Life States
    constexpr std::uint32_t ALIVE           = 0;
    constexpr std::uint32_t DYING           = 1;
    constexpr std::uint32_t DEAD            = 2;
    constexpr std::uint32_t UNCONSCIOUS     = 3;
    constexpr std::uint32_t REANIMATE       = 4;
    constexpr std::uint32_t RECYCLE         = 5;
    constexpr std::uint32_t RESTRAINED      = 6;
    constexpr std::uint32_t ESSENTIAL_DOWN  = 7;
    constexpr std::uint32_t BLEEDOUT        = 8;
    
    // Weapon States
    constexpr std::uint32_t WEAPON_SHEATHED       = 0;
    constexpr std::uint32_t WEAPON_WANT_TO_DRAW   = 1;
    constexpr std::uint32_t WEAPON_DRAWING        = 2;
    constexpr std::uint32_t WEAPON_DRAWN          = 3;
    constexpr std::uint32_t WEAPON_WANT_TO_SHEATHE= 4;
    constexpr std::uint32_t WEAPON_SHEATHING      = 5;
    
    // Gun States
    constexpr std::uint32_t GUN_DRAWN        = 0;
    constexpr std::uint32_t GUN_RELAXED      = 1;
    constexpr std::uint32_t GUN_BLOCKED      = 2;
    constexpr std::uint32_t GUN_ALERT        = 3;
    constexpr std::uint32_t GUN_RELOADING    = 4;
    constexpr std::uint32_t GUN_THROWING     = 5;
    constexpr std::uint32_t GUN_SIGHTED      = 6;
    constexpr std::uint32_t GUN_FIRE         = 7;
    constexpr std::uint32_t GUN_FIRE_SIGHTED = 8;
    
    // Interacting States
    constexpr std::uint32_t NOT_INTERACTING          = 0;
    constexpr std::uint32_t WAITING_TO_INTERACT      = 1;
    constexpr std::uint32_t INTERACTING              = 2;
    constexpr std::uint32_t WAITING_TO_STOP_INTERACT = 3;
    
    // Filter wildcard (any state)
    constexpr std::uint32_t ANY = 0xFF;
}

// Enemy Tier Classification System
enum class ENEMY_TIER : std::uint8_t
{
    LOW = 0,      // Basic enemies (low health, simple behavior)
    MEDIUM = 1,   // Standard enemies (moderate threat)
    HIGH = 2      // Dangerous enemies (bosses, legendaries, high health)
};

// Enemy combat style flags
struct EnemyAnalysis
{
    ENEMY_TIER tier;
    float healthPercentOfMax;  // Relative to highest enemy in cell
    bool isRanged;
    bool isMelee;
    bool hasGrenades;
    bool isUnique;
    bool isAlerted;
    bool isLegendary;
};

// Actor tracking data structure
struct TrackedActorData
{
    RE::Actor* actor;                    // Pointer to the actor
    ENEMY_TIER tier;                     // Threat level (for enemies)
    bool aiUpdated;                      // Whether AI was updated
    bool buffed;                         // Whether buffs were applied
    float distanceToPlayer;              // Distance in units
    RE::NiPoint3 position;               // Current position
    std::uint32_t lifeState;             // Current LIFE_STATE
    std::uint32_t weaponState;           // Current WEAPON_STATE
    std::uint32_t gunState;              // Current GUN_STATE
    std::uint32_t interactingState;      // Current INTERACTING_STATE
    float healthPercent;                 // Current health / max health
    float maxHealth;                     // Maximum health pool
    bool isAlerted;                      // In combat/alert state
    bool isRanged;                       // Uses ranged weapons
    bool isMelee;                        // Uses melee weapons
    bool hasGrenades;                    // Can throw grenades
    bool isUnique;                       // Unique NPC flag
    bool isLegendary;                    // Legendary enemy flag
    bool usesStimpak;                    // true = stimpak, false = repair kit
    std::chrono::steady_clock::time_point lastUpdate;     // Last scan time
    float velocity;                      // Current velocity
    int stuckCounter;                    // Consecutive stuck updates
    bool lost;                           // Whether actor is lost
};

// Companion Movement task
struct CompanionTask {
    RE::Actor* companion;
    float timeRemaining;
    float convexRadius;
};

// Companion Movement task management
namespace MovementSystem
{
    extern std::mutex g_companionTasksMutex;
    extern std::vector<CompanionTask> g_companionTasks;
    // Add a companion task
    void AddCompanionTask(RE::Actor* companion, float duration);
    // Process all Companion tasks (called per-frame)
    void ProcessCompanionTasks(float deltaTime);
    // Remove a specific Companion task
    void RemoveCompanionTask(RE::Actor* companion);
    // Apply movement settings to a companion
    void ApplyStuckMeasures1(RE::Actor* companion);
    // Apply movement settings to a companion
    void ApplyStuckMeasures2(RE::Actor* companion);
    // Remove movement settings from a companion
    void RemoveStuckMeasures(RE::Actor* companion);
}

// Slower loop to update companion data
namespace ActorTracking
{
    // Faster loop to update companion flags
    struct CompanionFlags {
        std::atomic<bool> stuck{false};
        std::atomic<int> stuckCounter{0};
        std::atomic<float> velocity{0.0f};
        std::atomic<bool> lost{false};
        std::atomic<float> lastPosX{0.0f};
        std::atomic<float> lastPosY{0.0f};
        std::atomic<float> lastPosZ{0.0f};
    };
    // Fast per-companion flags (mutex only for map structural changes)
    extern std::mutex g_companionFlagsMutex;
    extern std::unordered_map<RE::Actor*, CompanionFlags> g_companionFlags;
    void EnsureCompanionFlagEntry(RE::Actor* actor);
    void SyncCompanionFlagsWithSnapshot(const std::vector<TrackedActorData>& snapshot);
    bool GetActorStuckStatusFast(RE::Actor* actor);
    void SetActorStuckStatusFast(RE::Actor* actor, bool stuck);
    int GetActorStuckCounterFast(RE::Actor* actor);
    void IncrementActorStuckCounterFast(RE::Actor* actor);
    void SetActorStuckCounterFast(RE::Actor* actor, int counter);
    float GetActorVelocityFast(RE::Actor* actor);
    void SetActorLastPositionFast(RE::Actor* actor, const RE::NiPoint3& pos);
    void GetActorLastPositionFast(RE::Actor* actor, RE::NiPoint3& outPos);
    void SetActorVelocityFast(RE::Actor* actor, float vel);
    bool GetActorLostStatusFast(RE::Actor* actor);
    void SetActorLostStatusFast(RE::Actor* actor, bool lost);
    // normal data tracking (mutex for entire data set)
    extern std::mutex g_actorDataMutex;
    extern std::vector<TrackedActorData> g_enemies;
    extern std::vector<TrackedActorData> g_companions;
    extern std::vector<TrackedActorData> g_neutralNPCs;
    // CACHE: Previous frame data for comparison
    extern std::vector<TrackedActorData> g_enemies_prev;
    extern std::vector<TrackedActorData> g_companions_prev;
    extern std::vector<TrackedActorData> g_neutralNPCs_prev;
    // Helper to cache current state before update
    inline void CacheCurrentState() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        g_enemies_prev = g_enemies;
        g_companions_prev = g_companions;
        g_neutralNPCs_prev = g_neutralNPCs;
    }
    // Replace companion data (thread-safe)
    inline void ReplaceCompanionData(const std::vector<TrackedActorData>& newData) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        g_companions = newData;
    }
    // Replace enemy data (thread-safe)
    inline void ReplaceEnemyData(const std::vector<TrackedActorData>& newData) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        g_enemies = newData;
    }
    // Replace neutral NPC data (thread-safe)
    inline void ReplaceNeutralNPCData(const std::vector<TrackedActorData>& newData) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        g_neutralNPCs = newData;
    }
    // Get all companion data (thread-safe)
    inline std::vector<TrackedActorData> GetCompanionData() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        return g_companions;  // Returns a copy
    }
    // Get a COPY of companion data (thread-safe)
    inline std::optional<TrackedActorData> GetCompanionData(RE::Actor* actor) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        for (const auto& data : g_companions) {
            if (data.actor == actor) {
                return data;  // Returns a copy
            }
        }
        return std::nullopt;
    }
    // Get previous companion data
    inline std::optional<TrackedActorData> GetPreviousCompanionData(RE::Actor* actor) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        for (const auto& data : g_companions_prev) {
            if (data.actor == actor) {
                return data;
            }
        }
        return std::nullopt;
    }
    // Helper to get enemy data (thread-safe)
    inline std::vector<TrackedActorData> GetEnemyData() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        return g_enemies;  // Returns a copy
    }
    // Helper to get enemy data (thread-safe)
    inline std::optional<TrackedActorData> GetEnemyData(RE::Actor* actor) {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        for (const auto& data : g_enemies) {
            if (data.actor == actor) {
                return data;  // Returns a copy
            }
        }
        return std::nullopt;
    }
    // Helper to get neutral NPC data (thread-safe)
    inline std::vector<TrackedActorData> GetNeutralNPCData() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        return g_neutralNPCs;  // Returns a copy
    }
    // Helper get enemies actors
    inline std::vector<RE::Actor*> GetEnemyActors() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        std::vector<RE::Actor*> actors;
        for (const auto& data : g_enemies) {
            if (data.actor) {
                actors.push_back(data.actor);
            }
        }
        return actors;
    }
    // Helper get companion actors
    inline std::vector<RE::Actor*> GetCompanionActors() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        std::vector<RE::Actor*> actors;
        for (const auto& data : g_companions) {
            if (data.actor) {
                actors.push_back(data.actor);
            }
        }
        return actors;
    }
    // Helper get neutral NPC actors
    inline std::vector<RE::Actor*> GetNeutralNPCActors() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        std::vector<RE::Actor*> actors;
        for (const auto& data : g_neutralNPCs) {
            if (data.actor) {
                actors.push_back(data.actor);
            }
        }
        return actors;
    }
    // Helper to clear stale data
    inline void ClearAll() {
        std::lock_guard<std::mutex> lock(g_actorDataMutex);
        g_enemies.clear();
        g_companions.clear();
        g_neutralNPCs.clear();
        g_enemies_prev.clear();
        g_companions_prev.clear();
        g_neutralNPCs_prev.clear();
    }
}

// -- EVENTS ---

// Event handler for companion kill enemy events
class CompanionKillEventSink : public RE::BSTEventSink<RE::TESDeathEvent>
{
public:
    virtual RE::BSEventNotifyControl ProcessEvent(
        const RE::TESDeathEvent& a_event,
        RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) override;
    
    static CompanionKillEventSink* GetSingleton()
    {
        static CompanionKillEventSink singleton;
        return &singleton;
    }
private:
    CompanionKillEventSink() = default;
    ~CompanionKillEventSink() = default;
    CompanionKillEventSink(const CompanionKillEventSink&) = delete;
    CompanionKillEventSink(CompanionKillEventSink&&) = delete;
    CompanionKillEventSink& operator=(const CompanionKillEventSink&) = delete;
    CompanionKillEventSink& operator=(CompanionKillEventSink&&) = delete;
};

// --- FUNCTIONS ---

void ActionCompanions_Internal();
RE::BGSInventoryItem* ActorAddInventoryItem_Internal(RE::Actor *actor, RE::TESForm *itemForm, std::int32_t count);
void ActorRemoveInventoryItem_Internal(RE::Actor* actor, RE::TESForm* itemForm, std::int32_t count);
void ApplyAIAggression_Internal();
void ApplyPerksToCompanions_Internal();
void ApplyKeywordsToCompanions_Internal();
void BuffCompanions_Internal();
bool CheckActorHasItem_Internal(RE::Actor* actor, RE::TESForm* itemForm);
ActorStateData CheckActorStates_Internal(RE::Actor* actor);
bool CheckActorStatesMatch_Internal(RE::Actor* actor, std::uint32_t lifeStateFilter = 0xFF, std::uint32_t weaponStateFilter = 0xFF, std::uint32_t gunStateFilter = 0xFF, std::uint32_t interactingStateFilter = 0xFF);
bool CheckIsCurrentCellSettlement_Internal();
TrackedActorData CreateTrackedData_Internal(RE::Actor* actor, ENEMY_TIER tier = ENEMY_TIER::LOW);
void CompanionsSetMortality_Internal();
EnemyAnalysis EnemyActorAnalyze_Internal(RE::Actor* actor);
std::map<ENEMY_TIER, int> EnemyActorAnalyzeThreatLevel_Internal(std::vector<TrackedActorData> enemyData);
void EquipCompanions_Internal();
void EquipAmmunition_Internal();
void EquipInventoryItem_Internal(RE::Actor* aNPC, RE::BGSInventoryItem* aInvItem);
float GetActorAngleToActor(const RE::Actor* src, const RE::Actor* dst);
float GetActorDistanceToObject_Internal(RE::Actor* actor, RE::TESObjectREFR* object);
float GetActorDistanceToPlayer_Internal(RE::Actor* actor);
std::vector<RE::TESObjectREFR*> GetAllReferencesInCurrentCell_Internal();
std::vector<RE::Actor*> GetAllActors_Internal();
RE::BGSInventoryItem::Stack* GetInventoryItemStackData_Internal(RE::BGSInventoryItem* invItem);
RE::NiPoint3 GetPointXY_Internal(RE::NiPoint3 a_pos, RE::CFilter a_filter, float a_stepRadians, float a_scanDistance, float a_moveDistance);
float GetPointZ_Internal(RE::NiPoint3 a_pos, RE::CFilter a_filter, float a_scanDistanceUp, float a_scanDistanceDown);
std::uint32_t GetSlotMaskFromIndex_Internal(std::int32_t aiSlotIndex);
void HealActorDowned_Internal(RE::Actor *actor);
void HealActorLimbs_Internal(RE::Actor* actor);
void HealActorHealth_Internal(RE::Actor* actor, float healthPercent);
void HealActorPA_Internal(RE::Actor *actor);
void InitializeVariables_Internal();
bool IsActorActiveCompanion_Internal(RE::Actor* actor);
bool IsActorEnemy_Internal(RE::Actor* actor);
bool IsActorExcluded_Internal(RE::Actor* actor);
bool IsActorItemEquipped_Internal(RE::Actor *actor, RE::BGSInventoryItem *invItem);
bool IsActorPlayerOrCompanion_Internal(RE::Actor* actor);
bool IsActorVendor_Internal(RE::Actor* actor);
bool IsArmorItem_Internal(RE::TESForm* itemForm);
bool IsArmorPowerFrame_Internal(RE::TESObjectREFR* armor);
bool IsInventoryMenuOpen_Internal();
bool IsWeaponItem_Internal(RE::TESForm* itemForm);
RE::TESObjectREFR::RemoveItemData LootBuildRemoveItemData_Internal(RE::BGSInventoryItem *aInventoryItem, RE::TESObjectREFR *aContainer, std::int32_t aCount);
std::int32_t LootItems_Internal();
bool LootItemsWeaponLooseNearCorpse_Internal(RE::TESObjectREFR* source, RE::Actor* companion);
bool LootItemsFromReference_Internal(RE::TESObjectREFR* source, RE::Actor* companion);
bool LootItemFilter_Internal(RE::TESForm* a_FormRef);
bool LootItemFilter_Internal(RE::TESForm* aForm);
void SetCompanionChatter_Internal(RE::Actor* comp);
void SetCompanionCombatAI_Internal(RE::Actor* comp, RE::TESCombatStyle* combatStyle);
void Update_Internal();
std::int32_t UpdateGlobalActorArrays_Internal();

// --- HOOKS ---

// This is our simple repeating timer to run periodic updates
class CCB_RepeatingTimer {
public:
    CCB_RepeatingTimer() : running(false) {}
    // Start the timer with interval in seconds
    void Start(int intervalSeconds, std::function<void()> callback) {
        running = true;
        std::thread([=]() mutable {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
                if (running) callback();
            }
        }).detach();
    }
    void Stop() { running = false; }
    bool IsRunning() const { return running.load(); }
private:
    std::atomic<bool> running;
};

// --- PAPYRUS ---

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm);