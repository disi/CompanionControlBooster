#include <Global.h>
#include <PCH.h>

// --- VARIABLES ---
// Actor Tracking variables
namespace ActorTracking {
std::mutex g_actorDataMutex;
std::vector<TrackedActorData> g_enemies;
std::vector<TrackedActorData> g_companions;
std::vector<TrackedActorData> g_neutralNPCs;
std::vector<TrackedActorData> g_enemies_prev;
std::vector<TrackedActorData> g_companions_prev;
std::vector<TrackedActorData> g_neutralNPCs_prev;
} // namespace ActorTracking
// companion flags storage
std::mutex ActorTracking::g_companionFlagsMutex;
std::unordered_map<RE::Actor*, std::unique_ptr<ActorTracking::CompanionFlags>> ActorTracking::g_companionFlags;
// Reload interval counter
int g_iniReloadCounter = 0;
// Global for max enemy health in cell initialized to 1.0 to avoid division by zero
std::atomic<float> g_enemyMaxHealthInCell = 1.0f;
// Global cell flags
bool g_isInSettlement = false;
bool g_isInEncounterZone = false;
bool g_playerInCombat = false;
bool g_playerIsSneaking = false;
// Main thread work pending flag
std::atomic<bool> g_isMainThreadWorkPending = false;
// Throttle XP awards to at most once per second
static std::mutex s_lastXpMutex;
static std::chrono::steady_clock::time_point s_lastXpTime = std::chrono::steady_clock::now() - std::chrono::seconds(2);
// Looted Items tracking
std::unordered_set<RE::TESFormID> g_lootedItems;

// --- EVENTS ---

// Event handler for companion kill enemy events
RE::BSEventNotifyControl CompanionKillEventSink::ProcessEvent(const RE::TESDeathEvent& a_event, RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) {
    if (!XP_ENABLED)
        return RE::BSEventNotifyControl::kContinue;
    // Validate event data
    if (!a_event.actorDying || !a_event.actorKiller) {
        return RE::BSEventNotifyControl::kContinue;
    }
    // Only process death events where the actor is dead
    if (!a_event.dead)
        return RE::BSEventNotifyControl::kContinue;
    // Check throttle timer
    std::lock_guard<std::mutex> lk(s_lastXpMutex);
    auto now = std::chrono::steady_clock::now();
    if (now - s_lastXpTime < std::chrono::seconds(1)) {
        if (DEBUGGING) {
            REX::INFO("CompanionKillEventSink: XP award skipped — throttled (last award {:.3f}s ago).", std::chrono::duration<float>(now - s_lastXpTime).count());
        }
        return RE::BSEventNotifyControl::kContinue;
    }
    // Set new last XP time
    s_lastXpTime = now;
    // Get victim and killer actors
    auto* victim = a_event.actorDying->As<RE::Actor>();
    auto* killer = a_event.actorKiller->As<RE::Actor>();
    auto* player = RE::PlayerCharacter::GetSingleton();
    // Check if the killer is the player or a companion (companion kills also show player as killer)
    if (!victim || !killer || !player || killer != player) {
        return RE::BSEventNotifyControl::kContinue;
    }
    // Check if victim was hostile
    if (!victim->GetHostileToActor(player)) {
        return RE::BSEventNotifyControl::kContinue;
    }
    // Make a local copy under mutex protection
    // Copy companion data
    auto companionDataCopy = ActorTracking::GetCompanionData();
    // Find the companion who is closest to the killer (i.e., likely the firing companion)
    auto victimPos = victim->GetPosition();
    RE::Actor* closestCompanion = nullptr;
    float closestDistance = FLT_MAX;
    for (auto& companionData : companionDataCopy) {
        auto* companion = companionData.actor;
        if (!companion)
            continue;
        // Check if companion is alive (essential actors need to pass false for isDead check)
        if (!companionData.lifeState == ACTOR_STATE::ALIVE)
            continue;
        // Check if companion is in combat
        if (!companionData.isAlerted)
            continue;
        // Calculate distance between COMPANION and VICTIM position
        float distance = companionData.position.GetDistance(victimPos);
        if (distance < closestDistance) {
            closestDistance = distance;
            closestCompanion = companion;
        }
    }
    // Check if we found a firing companion
    if (closestCompanion && closestDistance < XP_KILLER_TOLERANCE) {
        // Companion kill - award XP!
        if (DEBUGGING) {
            REX::INFO("-------------------- Companion Kill Detected --------------------");
            REX::INFO("CompanionKillEventSink: Companion {} kill detected!", closestCompanion->GetDisplayFullName());
        }
        // Calculate and award XP...
        auto* victimNPC = victim->GetNPC();
        if (victimNPC && victimNPC->actorData.level > 0) {
            auto difficultyLevel = player->GetDifficultyLevel();
            float awardedXP = victimNPC->actorData.level;
            auto experienceReward = RE::GamePlayFormulas::GetExperienceReward(RE::GamePlayFormulas::EXPERIENCE_ACTIVITY::kKillNPC, difficultyLevel, awardedXP) * XP_RATIO;
            auto clampedXP = std::clamp(experienceReward, 1.0f, 1000000.0f);
            player->RewardExperience(clampedXP, true, victim, nullptr);
            if (DEBUGGING) {
                REX::INFO("CompanionKillEventSink: Awarded {:.0f} XP for level {} enemy", awardedXP, victimNPC->actorData.level);
                REX::INFO("-----------------------------------------------------------------");
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

// --- FUNCTIONS ---

// Main Update function
void Update_Internal() {
    // Quick check to ensure we are in a game session
    g_taskInterface->AddTask([]() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->parentCell) {
            // Not in a game session, skip update and cancel timer
            if (DEBUGGING)
                REX::INFO("Update_Internal: Not in a game session, skipping update and stopping timer.");
            g_updateTimer.Stop();
            return;
        }
        // Check if player is in a scene
        if (IsActorInScene_Internal(player)) {
            if (DEBUGGING)
                REX::WARN("Update_Internal: Player is currently in a scene, cannot get actors");
            return;
        }
    });
    // Reload INI settings if the interval is set
    if (INI_RELOAD_INTERVAL > 0) {
        g_iniReloadCounter++;
        if (g_iniReloadCounter >= INI_RELOAD_INTERVAL) {
            // Load MCM config
            LoadMCMConfig();
            // Reset counter
            g_iniReloadCounter = 0;
            if (DEBUGGING)
                REX::INFO("Update_Internal: Reloaded INI configuration.");
        }
    }
    // Initialize the global variables in case the game data wasn't ready yet
    InitializeVariables_Internal();
    // Check if the player paused the game
    if (IsMenuOpen_Internal()) {
        if (DEBUGGING)
            REX::INFO("Update_Internal: Game is paused, skipping update.");
        return;
    }
    // Continue with update
    if (DEBUGGING)
        REX::INFO("========================================================================");
    // Check the current cell
    g_isInSettlement = CheckIsCurrentCellSettlement_Internal();
    g_isInEncounterZone = CheckIsCurrentCellEncounterZone_Internal();
    if (DEBUGGING) {
        REX::INFO("Update_Internal: Info - Current cell is {}a settlement.", g_isInSettlement ? "" : "not ");
        REX::INFO("Update_Internal: Info - Current cell is {}an encounter zone.", g_isInEncounterZone ? "" : "not ");
    }
    if (DEBUGGING)
        REX::INFO("-----------------------------------------------------------------------");
    if (DEBUGGING)
        REX::INFO("Update_Internal: -------- Starting background work. --------");
    // Make sure the global pointers are initialized
    if (!g_companionFaction) {
        // Try to get the TESFaction this is only run once per session
        g_companionFaction = GetFormByFileAndID_Internal<RE::TESFaction>(CURRENT_COMPANION_FACTION_ID);
    }
    // Update global actor arrays and calculate threat levels
    int actorCount = UpdateGlobalActorArrays_Internal();
    if (DEBUGGING)
        REX::INFO("Update_Internal: Actors - Found a total of {} actors in the current cell.", actorCount);
    if (DEBUGGING)
        REX::INFO("Update_Internal: -------- Background work completed --------");
    // Log the counts of tracked actors and current threat tier distribution
    auto companionData = ActorTracking::GetCompanionData();
    if (companionData.size() == 0) {
        if (DEBUGGING)
            REX::INFO("Update_Internal: No companions detected, skipping main thread functions.");
        if (DEBUGGING)
            REX::INFO("========================================================================");
        return;
    }
    if (DEBUGGING) {
        auto neutralData = ActorTracking::GetNeutralNPCData();
        auto enemyData = ActorTracking::GetEnemyData();
        REX::INFO("Update_Internal: Actors - Current actor tracking summary:");
        REX::INFO("  - Companions: {}", companionData.size());
        REX::INFO("  - Neutral NPCs: {}", neutralData.size());
        REX::INFO("  - Enemies: {}", enemyData.size());
        if (enemyData.size() > 0) {
            auto enemyTierCounts = EnemyActorAnalyzeThreatLevel_Internal(enemyData);
            REX::INFO("    - Low Tier: {}", enemyTierCounts[ENEMY_TIER::LOW]);
            REX::INFO("    - Medium Tier: {}", enemyTierCounts[ENEMY_TIER::MEDIUM]);
            REX::INFO("    - High Tier: {}", enemyTierCounts[ENEMY_TIER::HIGH]);
        }
    }
    if (DEBUGGING)
        REX::INFO("-----------------------------------------------------------------------");
    // Only modify game data on the main thread
    if (g_taskInterface && !g_isMainThreadWorkPending) {
        g_isMainThreadWorkPending = true;
        g_taskInterface->AddTask([companionData]() {
            // Threadsafe work
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player)
                return;
            g_playerInCombat = player->IsInCombat();
            g_playerIsSneaking = player->IsSneaking();
            if (DEBUGGING)
                REX::INFO("Update_Internal: -------- Running functions on the main thread. --------");
            // apply if AI Aggression is enabled and we are in an encounter zone or the player is in combat or sneaking with AI_AGGRESSION_SNEAK enabled
            if (AI_AGGRESSION_ENABLED && ((g_isInEncounterZone && !g_isInSettlement) || g_playerInCombat || (AI_AGGRESSION_SNEAK && g_playerIsSneaking))) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Aggression - Updating companion aggression states...");
                ApplyAIAggression_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            // Remove AI Aggression if disabled
            } else {
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Aggression - AI aggression modification removed if present...");
                RemoveAIAggression_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            // Buff Companions
            if (BUFF_ENABLED) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Buff - Buffing companions...");
                if (BUFF_SET_VALUES) {
                    BuffCompanionsSetValues_Internal(companionData);
                } else {
                    BuffCompanionsMinValues_Internal(companionData);
                }
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            if (PERK_ENABLED) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Perk - Applying perks to companions...");
                ApplyPerksToCompanions_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            if (KEYWORD_ENABLED) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Keyword - Applying keywords to companions...");
                ApplyKeywordsToCompanions_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            // Loot items by companions if enabled and not in settlement and the player is not in a menu (like container or inventory)
            if (LOOT_ENABLED && !g_isInSettlement) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Loot - Looting items by companions...");
                auto itemcount = LootItems_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Loot - Looted a total of {} objects by companions.", itemcount);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            // Equip best items for companions
            if (AI_EQUIP_ARMOR || AI_EQUIP_WEAPON || AI_EQUIP_AMMO_REFILL) {
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (AI_EQUIP_ARMOR || AI_EQUIP_WEAPON) {
                    if (DEBUGGING)
                        REX::INFO("Update_Internal: Equip - Equipping best armor and weapons for companions...");
                    EquipCompanions_Internal(companionData);
                }
                if (AI_EQUIP_AMMO_REFILL) {
                    if (DEBUGGING)
                        REX::INFO("Update_Internal: Equip - Equipping ammunition for companions...");
                    EquipAmmunition_Internal(companionData);
                }
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            if ((g_isInEncounterZone && !g_isInSettlement) || g_playerInCombat) {
                // Action Companions based on their states
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
                if (DEBUGGING)
                    REX::INFO("Update_Internal: Action - Actioning companions...");
                ActionCompanions_Internal(companionData);
                if (DEBUGGING)
                    REX::INFO("-----------------------------------------------------------------------");
            }
            if (DEBUGGING)
                REX::INFO("Update_Internal: -------- Finished main thread work. --------");
            if (DEBUGGING)
                REX::INFO("========================================================================");
        });
        g_isMainThreadWorkPending = false;
    } else {
        if (DEBUGGING)
            REX::INFO("Update_Internal: Main thread work still pending, skipping main thread functions this update.");
    }
}

// Action Companions based on their states
void ActionCompanions_Internal(std::vector<TrackedActorData> companionData) {
    if (DEBUGGING)
        REX::INFO("ActionCompanions_Internal: Function called.");
    // Go over our companions
    for (auto& companion : companionData) {
        auto* comp = companion.actor;
        if (!comp)
            continue;
        if (IsActorInScene_Internal(comp))
            continue; // Skip if in a scene
        auto* compInv = comp->inventoryList;
        if (!compInv)
            continue;
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player)
            continue;
        // Action flags
        bool usedStimpak = false;
        bool fleeCombat = false;
        RE::TESIdleForm* idleToPlay = nullptr;
        // Pre-Check if the companion is out of action
        if (CheckActorStatesMatch_Internal(comp, ACTOR_STATE::DEAD, ACTOR_STATE::ANY, ACTOR_STATE::ANY, ACTOR_STATE::ANY) 
            || CheckActorStatesMatch_Internal(comp, ACTOR_STATE::BLEEDOUT, ACTOR_STATE::ANY, ACTOR_STATE::ANY, ACTOR_STATE::ANY) 
            || CheckActorStatesMatch_Internal(comp, ACTOR_STATE::ESSENTIAL_DOWN, ACTOR_STATE::ANY, ACTOR_STATE::ANY, ACTOR_STATE::ANY)) {
            if (AI_AUTO_REVIVE) {
                // Attempt to revive the companion
                if (companion.usesStimpak) {
                    auto* invStimpak = ActorAddInventoryItem_Internal(comp, g_itemStimpak, 1);
                    if (DEBUGGING)
                        REX::INFO("ActionCompanions_Internal: Revive - Attempting to auto-revive human companion {} using a stimpak...", comp->GetDisplayFullName());
                    if (!invStimpak) {
                        if (DEBUGGING)
                            REX::WARN("ActionCompanions_Internal: Revive - Failed to add Stimpak to companion {}'s inventory for auto-revive.", comp->GetDisplayFullName());
                        continue;
                    }
                    // Heal and revive
                    HealActorHealth_Internal(comp, 100.0f);
                    HealActorLimbs_Internal(comp);
                    // Important to clear the HC downed flag
                    HealActorDowned_Internal(comp);
                    // Make it use the stimpak to get back up
                    EquipInventoryItem_Internal(comp, invStimpak);
                    // No more processing in this update needed, continue to next companion
                    continue;
                } else {
                    auto* invRepairKit = ActorAddInventoryItem_Internal(comp, g_itemRepairKit, 1);
                    if (DEBUGGING)
                        REX::INFO("ActionCompanions_Internal: Revive - Attempting to auto-revive non-human companion {} using a repair kit...", comp->GetDisplayFullName());
                    if (!invRepairKit) {
                        if (DEBUGGING)
                            REX::WARN("ActionCompanions_Internal: Revive - Failed to add Repair Kit to companion {}'s inventory for auto-revive.", comp->GetDisplayFullName());
                        continue;
                    }
                    // Heal and revive
                    HealActorHealth_Internal(comp, 100.0f);
                    HealActorLimbs_Internal(comp);
                    // Important to clear the HC downed flag
                    HealActorDowned_Internal(comp);
                    // Make it use the repair kit to get back up
                    EquipInventoryItem_Internal(comp, invRepairKit);
                    // No more processing in this update needed, continue to next companion
                    continue;
                }
                if (DEBUGGING)
                    REX::INFO("ActionCompanions_Internal: Revive - Companion {} was revived automatically.", comp->GetDisplayFullName());
            } else {
                if (DEBUGGING)
                    REX::INFO("ActionCompanions_Internal: Revive - Actor {} is out of action. Skipping...", comp->GetDisplayFullName());
                continue;
            }
        }
        // Logging
        RE::TESForm* pkgForm = nullptr;
        if (comp->currentProcess) {
            auto* runningPackage = comp->currentProcess->GetPackageThatIsRunning();
            pkgForm = runningPackage ? runningPackage : nullptr;
        }
        if (DEBUGGING) {
            REX::INFO("ActionCompanions_Internal: Logging - Processing companion {} with race {}...", comp->GetDisplayFullName(), comp->race ? comp->race->GetFullName() : "Unknown");
            REX::INFO("ActionCompanions_Internal: Logging - Actor={} runningPkgID=0x{:08X} packageTypeName={}", comp->GetDisplayFullName(), pkgForm ? pkgForm->GetFormID() : 0, pkgForm && pkgForm->GetObjectTypeName());
            if (comp->currentProcess) {
                REX::INFO("ActionCompanions_Internal: Logging - followTarget==player? {} ; escortingPlayer={}, inCombat={}", (comp->currentProcess->followTarget == player->GetActorHandle()) ? "yes" : "no", comp->currentProcess->escortingPlayer ? "true" : "false", companion.isAlerted ? "true" : "false");
            }
            REX::INFO("ActionCompanions_Internal: Logging - The companions velocity is {:.2f} and is currently stuck: {}", ActorTracking::GetActorVelocityFast(comp), ActorTracking::GetActorStuckStatusFast(comp) ? "yes" : "no");
            REX::INFO("ActionCompanions_Internal: Logging - The companion is stuck for {} updates.", ActorTracking::GetActorStuckCounterFast(comp));
        }
        // Check if interacting
        if (CheckActorStatesMatch_Internal(comp, ACTOR_STATE::ALIVE, ACTOR_STATE::ANY, ACTOR_STATE::ANY, ACTOR_STATE::INTERACTING)) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Interacting - Actor {} is interacting. Skipping...", comp->GetDisplayFullName());
            continue;
        }
        // Stimpak: The companion is in combat or alerted and low on health
        if (companion.isAlerted && companion.healthPercent * 100.0f <= AI_HEALTH_THRESHOLD && AI_USE_STIMPAK_ENABLED) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Stimpak - Companion {} is alerted and low on health ({:.1f}%), checking for Stimpak or repair kit use...", comp->GetDisplayFullName(), companion.healthPercent * 100.0f);
            if (AI_USE_STIMPAK_UNLIMITED || (CheckActorHasItem_Internal(comp, g_itemStimpak) && companion.usesStimpak) || (CheckActorHasItem_Internal(comp, g_itemRepairKit) && !companion.usesStimpak)) {
                // Remove a Stimpak from the inventory if not set to unlimited
                if (!AI_USE_STIMPAK_UNLIMITED && companion.usesStimpak) {
                    ActorRemoveInventoryItem_Internal(comp, g_itemStimpak, 1);
                } else if (!AI_USE_STIMPAK_UNLIMITED && !companion.usesStimpak) {
                    ActorRemoveInventoryItem_Internal(comp, g_itemRepairKit, 1);
                }
                // Unlimited Stimpak use
                HealActorHealth_Internal(comp, 100.0f);
                HealActorLimbs_Internal(comp);
                usedStimpak = true;
                idleToPlay = g_idleStimpak;
                if (DEBUGGING)
                    REX::INFO("ActionCompanions_Internal: Stimpak - Companion {} used stimpak or repair kit! Health was at {:.1f}%", comp->GetDisplayFullName(), companion.healthPercent * 100.0f);
            } else {
                if (AI_FLEE_COMBAT) {
                    fleeCombat = true;
                    if (DEBUGGING)
                        REX::INFO("ActionCompanions_Internal: Stimpak - Companion {} wants to use a Stimpak or repair kit but has none, will flee combat!", comp->GetDisplayFullName());
                } else {
                    if (DEBUGGING)
                        REX::INFO("ActionCompanions_Internal: Stimpak - Companion {} wanted to use a Stimpak or repair kit but has none! Companion is not allowed to flee.", comp->GetDisplayFullName());
                }
            }
        }
        // Power Armor healing
        if (PA_ENABLED) {
            if (RE::PowerArmor::ActorInPowerArmor(*comp)) {
                if (DEBUGGING)
                    REX::INFO("ActionCompanions_Internal: Power Armor - Healing companion {} in Power Armor...", comp->GetDisplayFullName());
                HealActorPA_Internal(comp);
            }
        }
        // Chatter multiplier adjustment
        if (CHATTER_ENABLED) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Chatter - Setting chatter multiplier for companion {}...", comp->GetDisplayFullName());
            SetCompanionChatter_Internal(comp);
        }
        // Set Combat AI only if enabled
        if (COMBAT_ENABLED) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Combat AI - Setting combat AI for companion {}...", comp->GetDisplayFullName());
            auto* compCombatStyle = comp->GetCombatStyle();
            if (compCombatStyle) {
                SetCompanionCombatAI_Internal(comp, compCombatStyle);
            }
        }
        // Handle combat target setting
        if (companion.isAlerted && COMBAT_ENABLED) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Combat Target - Setting target for companion {}...", comp->GetDisplayFullName());
            // Set target for the companion if in combat
            if (companion.isAlerted) {
                auto enemyDataCopy = ActorTracking::GetEnemyData();
                RE::Actor* enemyToTarget = nullptr;
                switch (COMBAT_TARGET) {
                case 0: { // closest target
                    float closestDistance = FLT_MAX;
                    for (auto& enemyData : enemyDataCopy) {
                        // Go over enemyData.distance to find the shortest one
                        if (enemyData.distanceToPlayer < closestDistance) {
                            closestDistance = enemyData.distanceToPlayer;
                            enemyToTarget = enemyData.actor;
                        }
                    }
                    break;
                }
                case 1: { // Lowest threat target
                    float lowestThreat = FLT_MAX;
                    for (auto& enemyData : enemyDataCopy) {
                        // Go over enemyData.threatLevel to find a LOW tier one
                        if (enemyData.tier == ENEMY_TIER::LOW) {
                            enemyToTarget = enemyData.actor;
                        } else if (enemyData.tier == ENEMY_TIER::MEDIUM && lowestThreat > 1.0f) {
                            lowestThreat = 1.0f;
                            enemyToTarget = enemyData.actor;
                        } else if (enemyData.tier == ENEMY_TIER::HIGH && lowestThreat > 2.0f) {
                            lowestThreat = 2.0f;
                            enemyToTarget = enemyData.actor;
                        }
                    }
                    break;
                }
                case 2: { // Highest threat target
                    float highestThreat = -1.0f;
                    for (auto& enemyData : enemyDataCopy) {
                        // Go over enemyData.threatLevel to find a HIGH tier one
                        if (enemyData.tier == ENEMY_TIER::HIGH) {
                            enemyToTarget = enemyData.actor;
                            break; // highest possible, break immediately
                        } else if (enemyData.tier == ENEMY_TIER::MEDIUM && highestThreat < 2.0f) {
                            highestThreat = 2.0f;
                            enemyToTarget = enemyData.actor;
                        } else if (enemyData.tier == ENEMY_TIER::LOW && highestThreat < 1.0f) {
                            highestThreat = 1.0f;
                            enemyToTarget = enemyData.actor;
                        }
                    }
                    break;
                }
                }
                // Set the target
                comp->currentCombatTarget = enemyToTarget ? enemyToTarget->As<RE::Actor>() : nullptr;
                comp->UpdateCombat();
            }
        }
        // Finally act on based on flags if not in power armor
        // Use Stimpak idle if used
        if (usedStimpak && idleToPlay && !RE::PowerArmor::ActorInPowerArmor(*comp)) {
            // Play Stimpak idle
            if (IsActorRaceHumanoid_Internal(comp)) {
                if (comp && comp->currentProcess) {
                    comp->currentProcess->PlayIdle(*comp, idleToPlay, nullptr);
                    continue;
                }
            }
        }
        // Flee combat if needed
        if (fleeCombat) {
            // Flee combat to safe location
            if (comp && comp->currentProcess) {
                // Calculate a flee location AI_FLEE_DISTANCE units away from current position
                float minDist = AI_FLEE_DISTANCE * 0.5f;
                float maxDist = AI_FLEE_DISTANCE * 1.5f;
                float fleeFromDist = minDist + static_cast<float>(std::rand()) / RAND_MAX * (maxDist - minDist);
                float fleeToDist = fleeFromDist + static_cast<float>(std::rand()) / RAND_MAX * (maxDist - minDist);
                // InitiateFlee(TESObjectREFR* a_fleeRef, bool a_runonce, bool a_knows, bool a_combatMode,
                // TESObjectCELL* a_cell, TESObjectREFR* a_ref, float a_fleeFromDist, float a_fleeToDist)
                auto* fleeTarget = comp->currentCombatTarget.get().get();
                if (!fleeTarget) {
                    fleeTarget = player;
                }
                comp->InitiateFlee(fleeTarget, false, false, true, nullptr, nullptr, fleeFromDist, fleeToDist);
                continue;
            }
        }
        if (AI_STUCK_CHECK) {
            // Add the companion to the movement task list for the next UPDATE_INTERVAL seconds
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Stuck Check - Adding companion {} to movement task list for stuck checking.", comp->GetDisplayFullName());
            MovementSystem::AddCompanionTask(comp, UPDATE_INTERVAL);
        } else {
            // Remove from movement task list
            MovementSystem::RemoveCompanionTask(comp);
        }
        // Handle lost behaviour or stuck for more than 1 update (teleport to player)
        if ((companion.lost || companion.distanceToPlayer > AI_STUCK_DISTANCE) && AI_STUCK_CHECK) {
            if (DEBUGGING)
                REX::INFO("ActionCompanions_Internal: Lost - Companion {} is lost - teleporting to player!", comp->GetDisplayFullName());
            if (player) {
                auto playerPos = player->GetPosition();
                auto compPos = comp->GetPosition();
                // Calculate the angle from player to companion's current position
                float dx = compPos.x - playerPos.x;
                float dy = compPos.y - playerPos.y;
                float angle = std::atan2(dy, dx); // angle from player to companion
                // Use 75% of the current distance
                float currentDistance = std::sqrt(dx * dx + dy * dy);
                float radius = currentDistance * 0.75f;
                // Teleport companion to new position
                RE::NiPoint3 newPos;
                newPos.x = player->GetPosition().x + radius * std::cos(angle);
                newPos.y = player->GetPosition().y + radius * std::sin(angle);
                // Find a good Z position
                newPos.z = player->GetPosition().z; // start with the player's Z
                // Get collision filter of the companion
                //auto filter = comp->GetCollisionFilter(); // Crashes in 1.10.984 and 1.11.191
                // Create default collision filter instead of crashing GetCollisionFilter()
                RE::CFilter filter;
                filter.filter = 0x0000002C; // Default character controller collision layer
                // Get the xy position to a close object (position, filter, radiant steps, scan distance, move up distance)
                RE::NiPoint3 closePos = GetPointXY_Internal(newPos, filter, 100.0f, 500.0f, 60.0f);
                // Get ground Z at new position + 1.0f
                newPos.z = GetPointZ_Internal(newPos, filter, 100.0f, 500.0f) + 1.0f; // Scan 100 units up and 500 units down and add 1.0f to spawn Slightly above ground to pick up new navmesh
                // Nudge any running idles or stances to stop them
                if (comp->currentProcess)
                    comp->currentProcess->StopCurrentIdle(comp, true, true);
                // Set new position
                comp->SetPosition(newPos, true);
            }
            // Reset flags after teleporting
            ActorTracking::SetActorLostStatusFast(comp, false);
            ActorTracking::SetActorStuckCounterFast(comp, 0);
            continue;
        }
    }
}

// Help Add item from actor's inventory
RE::BGSInventoryItem* ActorAddInventoryItem_Internal(RE::Actor* actor, RE::TESForm* itemForm, std::int32_t count) {
    if (!actor || !itemForm || count <= 0)
        return nullptr;
    auto* invList = actor->inventoryList;
    if (!invList) {
        if (DEBUGGING)
            REX::WARN("ActorAddInventoryItem_Internal: Actor inventory list is null");
        return nullptr;
    }
    auto* itemObject = itemForm->As<RE::TESBoundObject>();
    actor->AddObjectToContainer(itemObject, nullptr, 1, nullptr, RE::ITEM_REMOVE_REASON::kStoreContainer);
    // Find and return the added item
    for (auto& item : invList->data) {
        // The item.object field is a RE::TESBoundObject*. We must compare it to our
        // successfully cast itemObject to find the correct entry.
        if (item.object == itemObject) {
            // You are adding 'count' items. This item should have a count >= 'count'.
            if (item.GetCount() >= count) {
                // Return a pointer to the found item in the inventory data structure
                return &item;
            }
        }
    }
    return nullptr;
}

// Help remove item from actor's inventory
void ActorRemoveInventoryItem_Internal(RE::Actor* actor, RE::TESForm* itemForm, std::int32_t count) {
    if (!actor || !itemForm)
        return;
    auto* invList = actor->inventoryList;
    if (!invList) {
        if (DEBUGGING)
            REX::WARN("RemoveActorInventoryItem_Internal: Actor inventory list is null");
        return;
    }
    // Create RemoveItemData
    RE::TESObjectREFR::RemoveItemData data(itemForm, count);
    data.reason = RE::ITEM_REMOVE_REASON::kNone;
    // Remove item
    auto result = actor->RemoveItem(data);
}

// Helper function to safely test if aiData is accessible (SEH-protected)
bool AIAccessTest_Internal(RE::TESNPC* npc) {
    if (!npc)
        return false;
    
#ifdef _MSC_VER
    __try {
        // Test read to verify memory is accessible
        [[maybe_unused]] volatile auto testRead = npc->aiData.useAggroRadius;
        return true; // Access succeeded
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false; // Access violation occurred
    }
#else
    // Non-MSVC: assume it's safe (no SEH available)
    return true;
#endif
}

// Helper function to apply the aggression settings to the companions current package
void ApplyAIAggression_Internal(std::vector<TrackedActorData> companionData) {
    // Go over our companions
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        if (companion.isAlerted)
            continue; // do not apply when already in combat
        auto* npc = actor->GetNPC();
        if (!npc)
            continue;
        // Test if aiData is safely accessible
        if (!AIAccessTest_Internal(npc)) {
            continue;
        }
        // Log current aiData settings
        if (npc) {
            // commented this section as it is not sure the packages can be detected properly
            // Disable when the standard follower package is not running and AI_AGGRESSION_ALL is false
            //if (actor->currentProcess && actor->currentProcess->GetPackageThatIsRunning() && g_packFollowersCompanion && actor->currentProcess->GetPackageThatIsRunning()->GetFormID() != g_packFollowersCompanion->GetFormID() && !AI_AGGRESSION_ALL) {
            //    npc->aiData.useAggroRadius = static_cast<std::uint32_t>(0);
            //    npc->aiData.aggroRadius[0] = static_cast<std::uint16_t>(0);
            //    npc->aiData.aggroRadius[1] = static_cast<std::uint16_t>(0);
            //    npc->aiData.aggroRadius[2] = static_cast<std::uint16_t>(0);
            //    continue;
            //}
            // Changing settings at runtime if they do not match the INI settings
            if (npc->aiData.useAggroRadius != static_cast<std::uint32_t>(AI_AGGRESSION_ENABLED)
                || npc->aiData.aggroRadius[0] != static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS0)
                || npc->aiData.aggroRadius[1] != static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS1)
                || npc->aiData.aggroRadius[2] != static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS2)) {
                npc->aiData.useAggroRadius = static_cast<std::uint32_t>(AI_AGGRESSION_ENABLED);
                npc->aiData.aggroRadius[0] = static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS0);
                npc->aiData.aggroRadius[1] = static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS1);
                npc->aiData.aggroRadius[2] = static_cast<std::uint16_t>(AI_AGGRESSION_RADIUS2);
            }
        }
    }
}

// Helper function to apply perks to companion actors
void ApplyPerksToCompanions_Internal(std::vector<TrackedActorData> companionData) {
    // Go over our companions
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        // Apply each perk from the global list
        for (auto perk : g_perkList) {
            if (perk && actor->GetPerkRank(perk) <= 0) {
                actor->AddPerk(perk);
                if (DEBUGGING)
                    REX::INFO("ApplyPerksToCompanions: Adding perk {} for companion {}", perk->GetFormEditorID(), actor->GetDisplayFullName());
            }
        }
    }
}

void ApplyKeywordsToCompanions_Internal(std::vector<TrackedActorData> companionData) {
    // Go over our companions
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        // Apply each keyword from the global list
        for (auto keyword : g_keywordList) {
            if (keyword && !actor->HasKeyword(keyword)) {
                actor->AddKeyword(keyword);
                if (DEBUGGING)
                    REX::INFO("ApplyKeywordsToCompanions: Adding keyword {} for companion {}", keyword->GetFormEditorID(), actor->GetDisplayFullName());
            }
        }
    }
}

// Buff companion actors checking minimum values
void BuffCompanionsMinValues_Internal(std::vector<TrackedActorData> companionData) {
    // Go over our companions
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        // Heal Rate buff
        auto* healRateAV = g_actorValueSingleton->healRateMult;
        if (healRateAV) {
            float currentHealRate = actor->GetActorValue(*healRateAV);
            if (currentHealRate < BUFF_HEAL_RATE) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_HEAL_RATE - currentHealRate;
                // Add heal rate buff based on BUFF_HEAL_RATE
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *healRateAV, missingAmount);
                currentHealRate = actor->GetActorValue(*healRateAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Heal rate of {} is {:.2f}", actor->GetDisplayFullName(), currentHealRate);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Heal Rate ActorValue not found!");
        }
        // Combat Heal Rate buff
        auto* combatHealRateAV = g_actorValueSingleton->combatHealthRegenMult;
        if (combatHealRateAV) {
            float currentCombatHealRate = actor->GetActorValue(*combatHealRateAV);
            if (currentCombatHealRate < BUFF_COMBAT_HEAL_RATE) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_COMBAT_HEAL_RATE - currentCombatHealRate;
                // Add combat heal rate buff based on BUFF_COMBAT_HEAL_RATE
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *combatHealRateAV, missingAmount);
                currentCombatHealRate = actor->GetActorValue(*combatHealRateAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Combat heal rate of {} is {:.2f}", actor->GetDisplayFullName(), currentCombatHealRate);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Combat Heal Rate ActorValue not found!");
        }
        // Damage Resist buff
        auto* dmgResistAV = g_actorValueSingleton->damageResistance;
        if (dmgResistAV) {
            float currentDmgResist = actor->GetActorValue(*dmgResistAV);
            if (currentDmgResist < BUFF_DAMAGE_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_DAMAGE_RESIST - currentDmgResist;
                // Add damage resistance buff based on BUFF_DAMAGE_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *dmgResistAV, missingAmount);
                currentDmgResist = actor->GetActorValue(*dmgResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Damage resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentDmgResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Damage Resistance ActorValue not found!");
        }
        // Fire Resist buff
        auto* fireResistAV = g_actorValueSingleton->fireResistance;
        if (fireResistAV) {
            float currentFireResist = actor->GetActorValue(*fireResistAV);
            if (currentFireResist < BUFF_FIRE_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_FIRE_RESIST - currentFireResist;
                // Add fire resistance buff based on BUFF_FIRE_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *fireResistAV, missingAmount);
                currentFireResist = actor->GetActorValue(*fireResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Fire resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentFireResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Fire Resistance ActorValue not found!");
        }
        // Electrical Resist buff
        auto* electricalResistAV = g_actorValueSingleton->electricalResistance;
        if (electricalResistAV) {
            float currentElectricalResist = actor->GetActorValue(*electricalResistAV);
            if (currentElectricalResist < BUFF_ELECTRICAL_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_ELECTRICAL_RESIST - currentElectricalResist;
                // Add electrical resistance buff based on BUFF_ELECTRICAL_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *electricalResistAV, missingAmount);
                currentElectricalResist = actor->GetActorValue(*electricalResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Electrical resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentElectricalResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Electrical Resistance ActorValue not found!");
        }
        // Frost Resist buff
        auto* frostResistAV = g_actorValueSingleton->frostResistance;
        if (frostResistAV) {
            float currentFrostResist = actor->GetActorValue(*frostResistAV);
            if (currentFrostResist < BUFF_FROST_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_FROST_RESIST - currentFrostResist;
                // Add frost resistance buff based on BUFF_FROST_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *frostResistAV, missingAmount);
                currentFrostResist = actor->GetActorValue(*frostResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Frost resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentFrostResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Frost Resistance ActorValue not found!");
        }
        // Energy Resist buff
        auto* energyResistAV = g_actorValueSingleton->energyResistance;
        if (energyResistAV) {
            float currentEnergyResist = actor->GetActorValue(*energyResistAV);
            if (currentEnergyResist < BUFF_ENERGY_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_ENERGY_RESIST - currentEnergyResist;
                // Add energy resistance buff based on BUFF_ENERGY_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *energyResistAV, missingAmount);
                currentEnergyResist = actor->GetActorValue(*energyResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Energy resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentEnergyResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Energy Resistance ActorValue not found!");
        }
        // Poison Resist buff
        auto* poisonResistAV = g_actorValueSingleton->poisonResistance;
        if (poisonResistAV) {
            float currentPoisonResist = actor->GetActorValue(*poisonResistAV);
            if (currentPoisonResist < BUFF_POISON_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_POISON_RESIST - currentPoisonResist;
                // Add poison resistance buff based on BUFF_POISON_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *poisonResistAV, missingAmount);
                currentPoisonResist = actor->GetActorValue(*poisonResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Poison resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentPoisonResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Poison Resistance ActorValue not found!");
        }
        // Radiation Exposure Resist buff
        auto* radiationResistAV = g_actorValueSingleton->radExposureResistance;
        if (radiationResistAV) {
            float currentRadiationResist = actor->GetActorValue(*radiationResistAV);
            if (currentRadiationResist < BUFF_RADIATION_RESIST) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_RADIATION_RESIST - currentRadiationResist;
                // Add radiation exposure resistance buff based on BUFF_RADIATION_RESIST
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *radiationResistAV, missingAmount);
                currentRadiationResist = actor->GetActorValue(*radiationResistAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Radiation exposure resistance of {} is {:.2f}", actor->GetDisplayFullName(), currentRadiationResist);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Radiation Exposure Resistance ActorValue not found!");
        }
        // Agility buff
        auto* agilityAV = g_actorValueSingleton->agility;
        if (agilityAV) {
            float currentAgility = actor->GetActorValue(*agilityAV);
            if (currentAgility < BUFF_AGILITY) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_AGILITY - currentAgility;
                // Add agility buff based on BUFF_AGILITY
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *agilityAV, missingAmount);
                currentAgility = actor->GetActorValue(*agilityAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Agility of {} is {:.2f}", actor->GetDisplayFullName(), currentAgility);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Agility ActorValue not found!");
        }
        // Endurance buff
        auto* enduranceAV = g_actorValueSingleton->endurance;
        if (enduranceAV) {
            float currentEndurance = actor->GetActorValue(*enduranceAV);
            if (currentEndurance < BUFF_ENDURANCE) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_ENDURANCE - currentEndurance;
                // Add endurance buff based on BUFF_ENDURANCE
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *enduranceAV, missingAmount);
                currentEndurance = actor->GetActorValue(*enduranceAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Endurance of {} is {:.2f}", actor->GetDisplayFullName(), currentEndurance);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Endurance ActorValue not found!");
        }
        // Intelligence buff
        auto* intelligenceAV = g_actorValueSingleton->intelligence;
        if (intelligenceAV) {
            float currentIntelligence = actor->GetActorValue(*intelligenceAV);
            if (currentIntelligence < BUFF_INTELLIGENCE) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_INTELLIGENCE - currentIntelligence;
                // Add intelligence buff based on BUFF_INTELLIGENCE
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *intelligenceAV, missingAmount);
                currentIntelligence = actor->GetActorValue(*intelligenceAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Intelligence of {} is {:.2f}", actor->GetDisplayFullName(), currentIntelligence);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Intelligence ActorValue not found!");
        }
        // Lockpick buff
        auto* lockpickAV = g_actorValueSingleton->lockpicking;
        if (lockpickAV) {
            float currentLockpick = actor->GetActorValue(*lockpickAV);
            if (currentLockpick < BUFF_LOCKPICK) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_LOCKPICK - currentLockpick;
                // Add lockpick buff based on BUFF_LOCKPICK
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *lockpickAV, missingAmount);
                currentLockpick = actor->GetActorValue(*lockpickAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Lockpick of {} is {:.2f}", actor->GetDisplayFullName(), currentLockpick);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Lockpick ActorValue not found!");
        }
        // Luck buff
        auto* luckAV = g_actorValueSingleton->luck;
        if (luckAV) {
            float currentLuck = actor->GetActorValue(*luckAV);
            if (currentLuck < BUFF_LUCK) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_LUCK - currentLuck;
                // Add luck buff based on BUFF_LUCK
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *luckAV, missingAmount);
                currentLuck = actor->GetActorValue(*luckAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Luck of {} is {:.2f}", actor->GetDisplayFullName(), currentLuck);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Luck ActorValue not found!");
        }
        // Perception buff
        auto* perceptionAV = g_actorValueSingleton->perception;
        if (perceptionAV) {
            float currentPerception = actor->GetActorValue(*perceptionAV);
            if (currentPerception < BUFF_PERCEPTION) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_PERCEPTION - currentPerception;
                // Add perception buff based on BUFF_PERCEPTION
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *perceptionAV, missingAmount);
                currentPerception = actor->GetActorValue(*perceptionAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Perception of {} is {:.2f}", actor->GetDisplayFullName(), currentPerception);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Perception ActorValue not found!");
        }
        // Sneak buff
        auto* sneakAV = g_actorValueSingleton->sneak;
        if (sneakAV) {
            float currentSneak = actor->GetActorValue(*sneakAV);
            if (currentSneak < BUFF_SNEAK) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_SNEAK - currentSneak;
                // Add sneak buff based on BUFF_SNEAK
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *sneakAV, missingAmount);
                currentSneak = actor->GetActorValue(*sneakAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Sneak of {} is {:.2f}", actor->GetDisplayFullName(), currentSneak);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Sneak ActorValue not found!");
        }
        // Strength buff
        auto* strengthAV = g_actorValueSingleton->strength;
        if (strengthAV) {
            float currentStrength = actor->GetActorValue(*strengthAV);
            if (currentStrength < BUFF_STRENGTH) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_STRENGTH - currentStrength;
                // Add strength buff based on BUFF_STRENGTH
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *strengthAV, missingAmount);
                currentStrength = actor->GetActorValue(*strengthAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Strength of {} is {:.2f}", actor->GetDisplayFullName(), currentStrength);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Strength ActorValue not found!");
        }
        // Carry weight buff
        auto* carryWeightAV = g_actorValueSingleton->carryWeight;
        if (carryWeightAV) {
            float currentCarryWeight = actor->GetActorValue(*carryWeightAV);
            if (currentCarryWeight < BUFF_CARRYWEIGHT) {
                // Calculate exactly how much we need to add to hit the floor
                float missingAmount = BUFF_CARRYWEIGHT - currentCarryWeight;
                // Add carry weight buff based on BUFF_CARRYWEIGHT
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *carryWeightAV, missingAmount);
                currentCarryWeight = actor->GetActorValue(*carryWeightAV);
                if (DEBUGGING)
                    REX::INFO("BuffCompanions: Carry Weight of {} is {:.2f}", actor->GetDisplayFullName(), currentCarryWeight);
            }
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Carry Weight ActorValue not found!");
        }
    }
}

// Buff companion actors checking minimum values
void BuffCompanionsSetValues_Internal(std::vector<TrackedActorData> companionData) {
    // Go over our companions
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        // Heal Rate buff
        auto* healRateAV = g_actorValueSingleton->healRateMult;
        if (healRateAV) {
            float currentHealRate = actor->GetActorValue(*healRateAV);
            float delta = BUFF_HEAL_RATE - currentHealRate;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *healRateAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Heal rate of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_HEAL_RATE);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Heal Rate ActorValue not found!");
        }
        // Combat Heal Rate buff
        auto* combatHealRateAV = g_actorValueSingleton->combatHealthRegenMult;
        if (combatHealRateAV) {
            float currentCombatHealRate = actor->GetActorValue(*combatHealRateAV);
            float delta = BUFF_COMBAT_HEAL_RATE - currentCombatHealRate;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *combatHealRateAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Combat heal rate of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_COMBAT_HEAL_RATE);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Combat Heal Rate ActorValue not found!");
        }
        // Damage Resist buff
        auto* dmgResistAV = g_actorValueSingleton->damageResistance;
        if (dmgResistAV) {
            float currentDmgResist = actor->GetActorValue(*dmgResistAV);
            float delta = BUFF_DAMAGE_RESIST - currentDmgResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *dmgResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Damage resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_DAMAGE_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Damage Resistance ActorValue not found!");
        }
        // Fire Resist buff
        auto* fireResistAV = g_actorValueSingleton->fireResistance;
        if (fireResistAV) {
            float currentFireResist = actor->GetActorValue(*fireResistAV);
            float delta = BUFF_FIRE_RESIST - currentFireResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *fireResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Fire resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_FIRE_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Fire Resistance ActorValue not found!");
        }
        // Electrical Resist buff
        auto* electricalResistAV = g_actorValueSingleton->electricalResistance;
        if (electricalResistAV) {
            float currentElectricalResist = actor->GetActorValue(*electricalResistAV);
            float delta = BUFF_ELECTRICAL_RESIST - currentElectricalResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *electricalResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Electrical resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_ELECTRICAL_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Electrical Resistance ActorValue not found!");
        }
        // Frost Resist buff
        auto* frostResistAV = g_actorValueSingleton->frostResistance;
        if (frostResistAV) {
            float currentFrostResist = actor->GetActorValue(*frostResistAV);
            float delta = BUFF_FROST_RESIST - currentFrostResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *frostResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Frost resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_FROST_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Frost Resistance ActorValue not found!");
        }
        // Energy Resist buff
        auto* energyResistAV = g_actorValueSingleton->energyResistance;
        if (energyResistAV) {
            float currentEnergyResist = actor->GetActorValue(*energyResistAV);
            float delta = BUFF_ENERGY_RESIST - currentEnergyResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *energyResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Energy resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_ENERGY_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Energy Resistance ActorValue not found!");
        }
        // Poison Resist buff
        auto* poisonResistAV = g_actorValueSingleton->poisonResistance;
        if (poisonResistAV) {
            float currentPoisonResist = actor->GetActorValue(*poisonResistAV);
            float delta = BUFF_POISON_RESIST - currentPoisonResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *poisonResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Poison resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_POISON_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Poison Resistance ActorValue not found!");
        }
        // Radiation Exposure Resist buff
        auto* radiationResistAV = g_actorValueSingleton->radExposureResistance;
        if (radiationResistAV) {
            float currentRadiationResist = actor->GetActorValue(*radiationResistAV);
            float delta = BUFF_RADIATION_RESIST - currentRadiationResist;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *radiationResistAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Radiation exposure resistance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_RADIATION_RESIST);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Radiation Exposure Resistance ActorValue not found!");
        }
        // Agility buff
        auto* agilityAV = g_actorValueSingleton->agility;
        if (agilityAV) {
            float currentAgility = actor->GetActorValue(*agilityAV);
            float delta = BUFF_AGILITY - currentAgility;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *agilityAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Agility of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_AGILITY);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Agility ActorValue not found!");
        }
        // Endurance buff
        auto* enduranceAV = g_actorValueSingleton->endurance;
        if (enduranceAV) {
            float currentEndurance = actor->GetActorValue(*enduranceAV);
            float delta = BUFF_ENDURANCE - currentEndurance;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *enduranceAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Endurance of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_ENDURANCE);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Endurance ActorValue not found!");
        }
        // Intelligence buff
        auto* intelligenceAV = g_actorValueSingleton->intelligence;
        if (intelligenceAV) {
            float currentIntelligence = actor->GetActorValue(*intelligenceAV);
            float delta = BUFF_INTELLIGENCE - currentIntelligence;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *intelligenceAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Intelligence of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_INTELLIGENCE);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Intelligence ActorValue not found!");
        }
        // Lockpick buff
        auto* lockpickAV = g_actorValueSingleton->lockpicking;
        if (lockpickAV) {
            float currentLockpick = actor->GetActorValue(*lockpickAV);
            float delta = BUFF_LOCKPICK - currentLockpick;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *lockpickAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Lockpick of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_LOCKPICK);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Lockpick ActorValue not found!");
        }
        // Luck buff
        auto* luckAV = g_actorValueSingleton->luck;
        if (luckAV) {
            float currentLuck = actor->GetActorValue(*luckAV);
            float delta = BUFF_LUCK - currentLuck;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *luckAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Luck of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_LUCK);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Luck ActorValue not found!");
        }
        // Perception buff
        auto* perceptionAV = g_actorValueSingleton->perception;
        if (perceptionAV) {
            float currentPerception = actor->GetActorValue(*perceptionAV);
            float delta = BUFF_PERCEPTION - currentPerception;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *perceptionAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Perception of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_PERCEPTION);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Perception ActorValue not found!");
        }
        // Sneak buff
        auto* sneakAV = g_actorValueSingleton->sneak;
        if (sneakAV) {
            float currentSneak = actor->GetActorValue(*sneakAV);
            float delta = BUFF_SNEAK - currentSneak;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *sneakAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Sneak of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_SNEAK);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Sneak ActorValue not found!");
        }
        // Strength buff
        auto* strengthAV = g_actorValueSingleton->strength;
        if (strengthAV) {
            float currentStrength = actor->GetActorValue(*strengthAV);
            float delta = BUFF_STRENGTH - currentStrength;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *strengthAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Strength of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_STRENGTH);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Strength ActorValue not found!");
        }
        // Carry weight buff
        auto* carryWeightAV = g_actorValueSingleton->carryWeight;
        if (carryWeightAV) {
            float currentCarryWeight = actor->GetActorValue(*carryWeightAV);
            float delta = BUFF_CARRYWEIGHT - currentCarryWeight;
            if (delta != 0.0f) {
                actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *carryWeightAV, delta);
            }
            if (DEBUGGING)
                REX::INFO("BuffCompanions: Carry Weight of {} is {:.2f}", actor->GetDisplayFullName(), BUFF_CARRYWEIGHT);
        } else {
            if (DEBUGGING)
                REX::WARN("BuffCompanions: Carry Weight ActorValue not found!");
        }
    }
}

// Helper function to get distance between Actor and Player
bool CheckActorHasItem_Internal(RE::Actor* actor, RE::TESForm* itemForm) {
    if (!actor || !itemForm)
        return false;
    auto* invList = actor->inventoryList;
    if (!invList)
        return false;
    // Check if the item exists in the inventory
    for (const auto& item : invList->data) {
        if (item.object == itemForm && item.GetCount() > 0) {
            return true;
        }
    }
    return false;
}

// Helper function to check Actor states
ActorStateData CheckActorStates(RE::Actor* actor) {
    ActorStateData states{};
    if (!actor) {
        if (DEBUGGING)
            REX::WARN("CheckActorStates: Actor pointer is null");
        return states; // Returns all zeros
    }
    // Extract all state values
    states.lifeState = static_cast<std::uint32_t>(actor->lifeState);
    states.weaponState = static_cast<std::uint32_t>(actor->weaponState);
    states.gunState = static_cast<std::uint32_t>(actor->gunState);
    states.interactingState = static_cast<std::uint32_t>(actor->interactingState);
    return states;
}

// Helper to check if actor matches specific state criteria
bool CheckActorStatesMatch_Internal(RE::Actor* actor, std::uint32_t lifeStateFilter, std::uint32_t weaponStateFilter, std::uint32_t gunStateFilter, std::uint32_t interactingStateFilter) {
    if (!actor)
        return false;
    auto states = CheckActorStates(actor);
    // Check each state (0xFF means "don't filter this state")
    if (lifeStateFilter != 0xFF && states.lifeState != lifeStateFilter)
        return false;
    if (weaponStateFilter != 0xFF && states.weaponState != weaponStateFilter)
        return false;
    if (gunStateFilter != 0xFF && states.gunState != gunStateFilter)
        return false;
    if (interactingStateFilter != 0xFF && states.interactingState != interactingStateFilter)
        return false;
    return true;
}

// Helper to check if the current cell is a settlement
bool CheckIsCurrentCellEncounterZone_Internal() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell)
        return false;
    auto* cell = player->parentCell;
    if (!cell)
        return false;
    // Get location from cell (can be by extradata or through encounter zone)
    auto* extraData = cell->extraList.get();
    RE::BGSEncounterZone* encounterZone = cell->GetEncounterZone();
    RE::BGSLocation* location = nullptr;
    if (!encounterZone && extraData) {
        auto* extraLocation = extraData->GetByType<RE::ExtraLocation>();
        if (extraLocation) {
            encounterZone = extraLocation->location->As<RE::BGSEncounterZone>();
            location = extraLocation->location;
        }
    } else if (encounterZone) {
        location = encounterZone->data.location;
    } else {
        return true; // No encounter zone means open world, thus encounter zone
    }
    // If no location found, should be open world and assume encounter zone
    if (!location) {
        return true;
    }
    // We should reach here for most cells
    // Initialize as not encounterzone as fail-safe for locations without keywords that are not settlements
    bool isEncounterzone = false;
    auto* locationEditorID = location->GetFormEditorID();
    // Check for any Encounter keywords, this means enemies can spawn here
    if (location->ContainsKeywordString("locenc") || location->ContainsKeywordString("LocEnc")) {
        isEncounterzone = true;
    }
    // Check if the location EditorID contains "MQ" (main quest locations are not considered encounter zones)
    if (locationEditorID && std::strstr(locationEditorID, "MQ") != nullptr) {
        isEncounterzone = false;
    }
    // Return result
    return isEncounterzone;
}

// Helper to check if the current cell is a settlement
bool CheckIsCurrentCellSettlement_Internal() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell)
        return false;
    auto* cell = player->parentCell;
    auto* encounterZone = cell->GetEncounterZone();
    if (!encounterZone)
        return false;
    // Check if the encounter zone data has the kWorkshopZone flag
    return encounterZone->data.flags.all(RE::ENCOUNTER_ZONE_DATA::FLAG::kWorkshopZone);
}

// Create tracked data for an actor
TrackedActorData CreateTrackedData_Internal(RE::Actor* actor, ENEMY_TIER tier) {
    TrackedActorData data{};
    data.actor = actor;
    data.tier = tier;
    data.lastUpdate = std::chrono::steady_clock::now();
    if (!actor)
        return data;
    // Distance
    data.distanceToPlayer = GetActorDistanceToPlayer_Internal(actor);
    // States
    auto states = CheckActorStates(actor);
    data.lifeState = states.lifeState;
    data.weaponState = states.weaponState;
    data.gunState = states.gunState;
    data.interactingState = states.interactingState;
    // Health
    auto* health = g_actorValueSingleton->health;
    if (health) {
        float currentHealth = actor->GetActorValue(*health);
        data.maxHealth = actor->GetPermanentActorValue(*health);
        data.healthPercent = (data.maxHealth > 0) ? (currentHealth / data.maxHealth) : 0.0f;
    }
    // Combat state
    data.isAlerted = actor->IsInCombat();
    // NPC info
    auto* npc = actor->GetNPC();
    if (npc) {
        data.isUnique = npc->IsUnique();
        data.isLegendary = (npc->legendTemplate != nullptr || npc->legendChance != nullptr);
    }
    return data;
}

// Analyze enemy threat level of an actor
EnemyAnalysis EnemyActorAnalyze_Internal(RE::Actor* actor) {
    EnemyAnalysis analysis{};
    analysis.tier = ENEMY_TIER::LOW; // Default
    if (!actor)
        return analysis;
    auto* npc = actor->GetNPC();
    if (!npc)
        return analysis;
    // Get actor's current/max health
    float currentHealth = 0.0f;
    float maxHealth = 0.0f;
    auto* healthAV = g_actorValueSingleton->health;
    if (healthAV) {
        currentHealth = actor->GetActorValue(*healthAV);
        maxHealth = actor->GetPermanentActorValue(*healthAV);
    }
    // Calculate health percentage relative to strongest enemy
    analysis.healthPercentOfMax = (maxHealth > 0.0f) ? (currentHealth / g_enemyMaxHealthInCell.load()) : 0.0f;
    // Check unique/legendary status
    analysis.isUnique = npc->IsUnique();
    analysis.isLegendary = (npc->legendTemplate != nullptr || npc->legendChance != nullptr);
    // Check if alerted/in combat
    analysis.isAlerted = actor->IsInCombat();
    // Check equipped weapon to determine combat type
    int weaponThreatBonus = 0;
    if (actor->currentProcess && actor->currentProcess->middleHigh) {
        auto& equippedItems = actor->currentProcess->middleHigh->equippedItems;
        for (auto& equippedItem : equippedItems) {
            auto* weapon = equippedItem.item.object->As<RE::TESObjectWEAP>();
            if (weapon) {
                std::uint32_t weaponValue = weapon->weaponData.value;
                // Weapon value-based threat scaling
                if (weaponValue >= 1000) {
                    weaponThreatBonus = 3; // Legendary/Unique weapons
                } else if (weaponValue >= 500) {
                    weaponThreatBonus = 2; // Heavily modified/enchanted
                } else if (weaponValue >= 250) {
                    weaponThreatBonus = 1; // Standard modified weapon
                }
                // Check weapon damage
                float weaponDamage = weapon->weaponData.attackDamage;
                if (weaponDamage > 100) {
                    weaponThreatBonus += 1; // High-damage weapon
                }
                // Determine combat style from animation type
                auto weaponType = weapon->weaponData.type.get();
                if (weaponType == RE::WEAPON_TYPE::kHandToHand || weaponType == RE::WEAPON_TYPE::kOneHandSword || weaponType == RE::WEAPON_TYPE::kOneHandDagger || weaponType == RE::WEAPON_TYPE::kOneHandAxe || weaponType == RE::WEAPON_TYPE::kOneHandMace || weaponType == RE::WEAPON_TYPE::kTwoHandSword || weaponType == RE::WEAPON_TYPE::kTwoHandAxe) {
                    analysis.isMelee = true;
                    weaponThreatBonus += 2; // Melee = high threat (gets in your face)
                } else if (weaponType == RE::WEAPON_TYPE::kGrenade || weaponType == RE::WEAPON_TYPE::kMine) {
                    analysis.hasGrenades = true;
                    analysis.isRanged = true;
                    weaponThreatBonus += 2; // Explosives = high threat (area damage)
                } else if (weaponType == RE::WEAPON_TYPE::kGun || weaponType == RE::WEAPON_TYPE::kBow || weaponType == RE::WEAPON_TYPE::kStaff) {
                    analysis.isRanged = true;
                    weaponThreatBonus += 1; // Ranged = moderate threat (can be avoided)
                }
            }
        }
    }
    // Calculate tier based on multiple factors
    int threatScore = 0;
    // Add weapon threat bonus
    threatScore += (weaponThreatBonus * THREAT_WEAPON_BONUS);
    // Check display name for "Legendary" prefix (more reliable than template check)
    auto displayName = actor->GetDisplayFullName();
    if (displayName) {
        std::string nameStr(displayName);
        if (nameStr.find("Legendary") != std::string::npos) {
            analysis.isLegendary = true;
            threatScore += (3 * THREAT_LEGENDARY_BONUS); // always high threat for legendary enemies
        }
    }
    // Health factor (0-3 points)
    if (analysis.healthPercentOfMax > 0.8f)
        threatScore += (3 * THREAT_HEALTH_BONUS);
    else if (analysis.healthPercentOfMax > 0.5f)
        threatScore += (2 * THREAT_HEALTH_BONUS);
    else if (analysis.healthPercentOfMax > 0.3f)
        threatScore += (1 * THREAT_HEALTH_BONUS);
    // Special status (0-3 points) - only if not already counted from name
    if (!analysis.isLegendary && npc->legendTemplate) {
        analysis.isLegendary = true;
        threatScore += (3 * THREAT_LEGENDARY_BONUS);
    } else if (analysis.isUnique) {
        threatScore += (2 * THREAT_UNIQUE_BONUS);
    }
    // Alert status (0-1 point)
    if (analysis.isAlerted)
        threatScore += (1 * THREAT_ALERT_BONUS);
    // Assign tier based on total score
    if (threatScore >= 7) {
        analysis.tier = ENEMY_TIER::HIGH;
    } else if (threatScore >= 4) {
        analysis.tier = ENEMY_TIER::MEDIUM;
    } else {
        analysis.tier = ENEMY_TIER::LOW;
    }
    return analysis;
}

// Informational function to analyze all enemies and return tier distribution
std::map<ENEMY_TIER, int> EnemyActorAnalyzeThreatLevel_Internal(std::vector<TrackedActorData> enemyData) {
    std::map<ENEMY_TIER, int> tierCount{{ENEMY_TIER::LOW, 0}, {ENEMY_TIER::MEDIUM, 0}, {ENEMY_TIER::HIGH, 0}};
    if (enemyData.empty())
        return tierCount;
    // Analyze each enemy
    for (auto& enemyDataEntry : enemyData) {
        if (!enemyDataEntry.actor)
            continue;
        auto analysis = EnemyActorAnalyze_Internal(enemyDataEntry.actor);
        tierCount[analysis.tier]++;
    }
    return tierCount;
}

// Equip the best items from inventory
void EquipCompanions_Internal(std::vector<TrackedActorData> companionData) {
    std::vector<int> slotOrder = AI_EQUIP_SLOTS;
    // Go over each companion and equip best armor item
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        if (!IsActorRaceHumanoid_Internal(actor))
            continue; // Skip non-humanoid
        if (IsActorInScene_Internal(actor))
            continue; // Skip if in a scene
        auto* compInv = actor->inventoryList;
        if (!compInv) {
            if (DEBUGGING)
                REX::INFO("EquipCompanions_Internal: Skipping {}, no inventory.", actor->GetDisplayFullName());
            continue;
        }
        if (companion.isAlerted) {
            if (DEBUGGING)
                REX::INFO("EquipCompanions_Internal: Skipping {}, is in combat.", actor->GetDisplayFullName());
            continue; // Skip if in combat
        }
        // Equip best armor for each slot
        if (AI_EQUIP_ARMOR) {
            for (int slot : slotOrder) {
                RE::BGSInventoryItem* bestArmorItem = nullptr;
                float bestArmorValue = 0.0f;
                // Iterate through inventory to find best item for the slot
                for (auto& item : compInv->data) {
                    // Early exit if not armor/weapon
                    if (!IsArmorItem_Internal(item.object))
                        continue;
                    if (IsArmorPower_Internal(&item)) {
                        continue; // Skip power armor items
                    }
                    auto* armor = item.object->As<RE::TESObjectARMO>();
                    // Check if armor fits the slot
                    if ((armor && (armor->bipedModelData.bipedObjectSlots & GetSlotMaskFromIndex_Internal(slot))) != 0) {
                        if (!EquipSlotCheck_Internal(actor, armor)) {
                            continue; // Skip if it unequips other slots
                        }
                        // Get armor value
                        float armorValue = armor->armorData.value;
                        // Check if this is the best item so far
                        if (armorValue > bestArmorValue) {
                            bestArmorValue = armorValue;
                            bestArmorItem = &item;
                        }
                    }
                }
                // Equip the best item found for the slot
                if (bestArmorItem && !IsActorItemEquipped_Internal(actor, bestArmorItem)) {
                    EquipInventoryItem_Internal(actor, bestArmorItem);
                    if (DEBUGGING)
                        REX::INFO("EquipCompanions_Internal: Equipped {} on {} for slot {}", bestArmorItem->GetDisplayFullName(std::uint8_t(0)), actor->GetDisplayFullName(), slot);
                }
            }
        }
        if (AI_EQUIP_WEAPON) {
            // Equip best weapon
            RE::BGSInventoryItem* bestWeaponItem = nullptr;
            float bestWeaponValue = 0.0f;
            for (auto& item : compInv->data) {
                // Early exit if not weapon
                if (!IsWeaponItem_Internal(item.object))
                    continue;
                auto* weapon = item.object->As<RE::TESObjectWEAP>();
                // Get weapon value
                float weaponValue = weapon->weaponData.value;
                // Check if this is the best weapon so far
                if (weaponValue > bestWeaponValue) {
                    bestWeaponValue = weaponValue;
                    bestWeaponItem = &item;
                }
            }
            // Equip the best weapon found
            if (bestWeaponItem && !IsActorItemEquipped_Internal(actor, bestWeaponItem)) {
                EquipInventoryItem_Internal(actor, bestWeaponItem);
                if (DEBUGGING)
                    REX::INFO("EquipCompanions_Internal: Equipped {} on {} for weapon slot", bestWeaponItem->GetDisplayFullName(std::uint8_t(0)), actor->GetDisplayFullName());
            }
        }
    }
}

// Equip the best items from inventory
void EquipAmmunition_Internal(std::vector<TrackedActorData> companionData) {
    // Go over each companion and equip ammo for equipped weapons
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        auto* compInv = actor->inventoryList;
        if (!compInv)
            continue;
        // Check for ammo needs
        if (actor->currentProcess && actor->currentProcess->middleHigh) {
            auto& equippedItems = actor->currentProcess->middleHigh->equippedItems; // Ensure equippedItems is valid
            for (const auto& equippedItem : equippedItems) {
                auto* weapon = equippedItem.item.object->As<RE::TESObjectWEAP>();
                if (weapon) {
                    auto* ammo = weapon->weaponData.ammo;
                    if (ammo) {
                        // Now check inventory for ammo count
                        int ammoCount = 0;
                        auto* invList = actor->inventoryList;
                        if (invList) {
                            for (const auto& item : invList->data) {
                                if (item.object == ammo) {
                                    ammoCount = item.GetCount();
                                    break;
                                }
                            }
                        }
                        // If ammo count is less than desired amount, add more
                        if (ammoCount < AI_EQUIP_AMMO_AMOUNT) {
                            int ammoToAdd = AI_EQUIP_AMMO_AMOUNT - ammoCount;
                            actor->AddObjectToContainer(weapon->weaponData.ammo, nullptr, ammoToAdd, nullptr, RE::ITEM_REMOVE_REASON::kStoreContainer);
                            if (DEBUGGING)
                                REX::INFO("EquipCompanions_Internal: Adding {} of ammo {} to NPC inventory.", ammoToAdd, ammo->GetFullName());
                            return; // Only equip ammo for the equipped weapon
                        }
                    }
                }
            }
        }
    }
}

// Helper function to equip a single inventory item to an NPC
void EquipInventoryItem_Internal(RE::Actor* aCompanion, RE::BGSInventoryItem* aInvItem) {
    if (!aInvItem || !aInvItem->object || !aCompanion)
        return;
    if (IsArmorPower_Internal(aInvItem)) {
        return; // Skip power frames and power armor
    }
    // Get instance data and stack ID
    RE::TBO_InstanceData* instanceData = nullptr;
    std::uint32_t validStackID = 0;
    for (std::uint32_t i = 0; i < aInvItem->stackData->count; ++i) {
        instanceData = aInvItem->GetInstanceData(i);
        if (instanceData) {
            validStackID = i;
            break;
        }
    }
    if (!instanceData) {
        if (DEBUGGING)
            REX::INFO("EquipInventoryItem_Internal: No valid instanceData found for FormID: {:08X}. Using default.", aInvItem->object->GetFormID());
        validStackID = 0;
        instanceData = nullptr;
    }
    // RE::TBO_InstanceData* instanceData = aInvItem->GetInstanceData(stackID);
    //  Create object instance
    RE::BGSObjectInstance objInstance(aInvItem->object, instanceData);
    // Determine equip slot (armor or weapon)
    const RE::BGSEquipSlot* equipSlot = nullptr;
    if (aInvItem->object->GetFormType() == RE::ENUM_FORM_ID::kARMO) {
        auto armor = aInvItem->object->As<RE::TESObjectARMO>();
        equipSlot = armor ? armor->equipSlot : nullptr;
    } else if (aInvItem->object->GetFormType() == RE::ENUM_FORM_ID::kWEAP) {
        auto weapon = aInvItem->object->As<RE::TESObjectWEAP>();
        equipSlot = weapon ? weapon->equipSlot : nullptr;
    }
    // Equip using ActorEquipManager
    if (!g_actorEquipMgrSingleton) {
        if (DEBUGGING)
            REX::WARN("EquipInventoryItem_Internal: ActorEquipManager singleton not found!");
        return;
    }
    g_actorEquipMgrSingleton->EquipObject(aCompanion, objInstance, validStackID,
                          1,         // count
                          equipSlot, // slot (nullptr for weapons)
                          false,     // queueEquip
                          true,      // forceEquip
                          true,      // playSounds
                          true,      // applyNow
                          false      // locked
    );
}

// Helper function to check if equipping an item would unequip items in other slots
bool EquipSlotCheck_Internal(RE::Actor* actor, RE::TESObjectARMO* armor) {
    if (!actor || !armor)
        return false;
    auto* invList = actor->inventoryList;
    if (!invList)
        return false;
    // Get the new item's biped slot mask
    auto armorItemMask = armor->bipedModelData.bipedObjectSlots;
    // Check the inventory of already equipped items if there are conflicts
    for (auto& item : invList->data) {
        if (!item.stackData->IsEquipped())
            continue; // Skip we only need equipped items
        auto* invArmor = item.object->As<RE::TESObjectARMO>();
        if (!invArmor)
            continue;
        if (invArmor == armor) {
            if (DEBUGGING)
            return false; // Same item, skip and do not re-equip
        }
        // Compare biped slot masks
        auto invItemMask = invArmor->bipedModelData.bipedObjectSlots;
        if ((armorItemMask & invItemMask) != 0) {
            if (armorItemMask == invItemMask && !IsArmorPower_Internal(&item)) {
                continue; // the slots are the same and not power armor, allow re-equip
            }
            if (DEBUGGING) {
                REX::INFO("EquipSlotCheck: BLOCKING {} - would unequip {}", armor->GetFullName(), invArmor->GetFullName());
            }
            return false; // Conflict found, would unequip an already equipped item in another slot
        }
    }
    if (DEBUGGING) {
        REX::INFO("EquipSlotCheck_Internal: No slot conflicts for {}", armor->GetFullName());
    }
    return true; // No equipped armour that would be unequipped due to slot conflicts
}

// Helper to get all Actors in the player's current cell
std::vector<RE::Actor*> GetAllActors_Internal() {
    std::unordered_set<RE::Actor*> actorSet;
    std::vector<RE::Actor*> actors;
    // Get the player's current cell
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !g_processListsSingleton)
        return actors;
    // Check if player is in a scene
    if (IsActorInScene_Internal(player)) {
        if (DEBUGGING)
            REX::WARN("GetAllActors_Internal: Player is currently in a scene, cannot get actors");
        return actors;
    }
    auto playerPos = player->GetPosition();
    // Check high priority actors
    for (auto& actorHandle : g_processListsSingleton->highActorHandles) {
        auto* actor = actorHandle.get().get();
        if (actor) {
            // Filter by distance instead of cell (true "radar" approach)
            auto actorPos = actor->GetPosition();
            float dx = actorPos.x - playerPos.x;
            float dy = actorPos.y - playerPos.y;
            float dz = actorPos.z - playerPos.z;
            float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            // Include actors within a reasonable radius
            // Adjust this value based on your needs
            if (distance <= ACTOR_SEARCH_RADIUS) {
                actorSet.insert(actor);
            }
        }
    }
    // Also check medium-high actors (companions might be here when further away)
    for (auto& actorHandle : g_processListsSingleton->middleHighActorHandles) {
        auto* actor = actorHandle.get().get();
        if (actor) {
            auto actorPos = actor->GetPosition();
            float dx = actorPos.x - playerPos.x;
            float dy = actorPos.y - playerPos.y;
            float dz = actorPos.z - playerPos.z;
            float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (distance <= ACTOR_SEARCH_RADIUS) {
                actorSet.insert(actor);
            }
        }
    }
    // This adds 1000+ critters, so disabled for now
    // Also check low actors (companions might be here when further away)
    //for (auto& actorHandle : g_processListsSingleton->lowActorHandles) {
    //    auto* actor = actorHandle.get().get();
    //    if (actor) {
    //        auto actorPos = actor->GetPosition();
    //        float dx = actorPos.x - playerPos.x;
    //        float dy = actorPos.y - playerPos.y;
    //        float dz = actorPos.z - playerPos.z;
    //        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    //        if (distance <= ACTOR_SEARCH_RADIUS) {
    //            actorSet.insert(actor);
    //        }
    //    }
    //}
    if (!actorSet.empty())
        return std::vector<RE::Actor*>(actorSet.begin(), actorSet.end());
    // Fallback: check cell references if no actors found
    auto* currentCell = player->parentCell;
    if (currentCell) {
        for (auto& refHandle : currentCell->references) {
            auto* ref = refHandle.get();
            if (ref) {
                auto* actor = ref->As<RE::Actor>();
                if (actor) {
                    actors.push_back(actor);
                }
            }
        }
    }
    return actors;
}

// Helper to get all references in the player's current cell
std::vector<RE::TESObjectREFR*> GetAllLootReferencesInCurrentCell_Internal() {
    std::vector<RE::TESObjectREFR*> references;
    // Get the player's current cell
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell) {
        if (DEBUGGING)
            REX::WARN("GetAllLootReferencesInCurrentCell_Internal: Player or parent cell pointer is null");
        return references;
    }
    // Check if player is in a scene
    if (IsActorInScene_Internal(player)) {
        if (DEBUGGING)
            REX::WARN("GetAllLootReferencesInCurrentCell_Internal: Player is currently in a scene, cannot get references");
        return references;
    }
    auto* currentCell = player->parentCell;
    // Iterate through all references in the cell
    if (!currentCell->references.empty()) {
        for (auto& refHandle : currentCell->references) {
            if (auto* ref = refHandle.get()) {
                // Check if the reference is lootable
                if (!IsLootableFormType_Internal(ref)) {
                    continue; // Skip non-lootable types
                }
                // Check if it is excluded by keyword
                if (IsLootKeywordExcluded_Internal(ref)) {
                    continue; // Skip excluded keywords
                }
                auto* refActor = ref->As<RE::Actor>();
                // check if the reference is an actor and skip alive actors and vendors
                if (refActor && (!refActor->IsDead(true) || IsActorVendor_Internal(refActor))) {
                    continue; // Skip alive or vendor actors
                }
                // Check if the reference has vendor in the editor ID to skip vendor chests
                const char* editorID = ref->GetObjectReference()->GetFormEditorID();
                if (editorID && std::string(editorID).find("Vendor") != std::string::npos) {
                    continue; // Skip vendor chest
                }
                // Check if the reference is a power amor frame or power armor and skip it
                if (IsArmorPowerFrame_Internal(ref)) {
                    continue; // Skip power frames and power armor
                }
                references.push_back(ref);
            }
        }
    }
    return references;
}

// Calculate angle between two actors
float GetActorAngleToActor(const RE::Actor* src, const RE::Actor* dst) {
    auto srcPos = src->GetPosition(); // NiPoint3
    auto dstPos = dst->GetPosition();
    float dx = dstPos.x - srcPos.x;
    float dy = dstPos.y - srcPos.y;
    return std::atan2(dy, dx); // angle in radians
}

// Calculate distance between actor and object
float GetActorDistanceToObject_Internal(RE::Actor* actor, RE::TESObjectREFR* object) {
    // if we cannot get a distance, return max float to avoid selection as the closest
    if (!actor)
        return FLT_MAX;
    if (!object)
        return FLT_MAX;
    // Calculate distance
    auto actorPos = actor->GetPosition();
    auto objectPos = object->GetPosition();
    float dx = actorPos.x - objectPos.x;
    float dy = actorPos.y - objectPos.y;
    float dz = actorPos.z - objectPos.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
// Calculate distance between actor and player
float GetActorDistanceToPlayer_Internal(RE::Actor* actor) {
    // if we cannot get a distance, return max float to avoid selection as the closest
    if (!actor)
        return FLT_MAX;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player)
        return FLT_MAX;
    // Calculate distance
    auto actorPos = actor->GetPosition();
    auto playerPos = player->GetPosition();
    float dx = actorPos.x - playerPos.x;
    float dy = actorPos.y - playerPos.y;
    float dz = actorPos.z - playerPos.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Helper function to get the StackData of an inventory item
RE::BGSInventoryItem::Stack* GetInventoryItemStackData_Internal(RE::BGSInventoryItem* invItem) {
    for (std::uint32_t i = 0; i < invItem->stackData->count; ++i) {
        auto* instanceData = invItem->GetInstanceData(i);
        if (instanceData) {
            return invItem->stackData.get();
        }
    }
    return nullptr;
}

// Helper to find a valid X,Y position by scanning around
RE::NiPoint3 GetPointXY_Internal(RE::NiPoint3 a_pos, RE::CFilter a_filter, float a_stepRadians, float a_scanDistance, float a_moveDistance) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell)
        return a_pos;
    const float scanDistance = a_scanDistance;
    const float stepRadians = a_stepRadians;
    const float moveDistance = a_moveDistance;
    const float TWO_PI = 6.28318530717958647692f;
    RE::bhkPickData pickData;
    bool found = false;
    float bestDist = FLT_MAX;
    float bestDirX = 0.0f, bestDirY = 0.0f;
    // For each sampled direction cast a horizontal-ish ray and record closest hit
    for (float angle = 0.0f; angle < TWO_PI; angle += stepRadians) {
        float dx = std::cos(angle);
        float dy = std::sin(angle);
        RE::NiPoint3 rayStart{a_pos.x, a_pos.y, a_pos.z + 50.0f};
        RE::NiPoint3 rayEnd{a_pos.x + dx * scanDistance, a_pos.y + dy * scanDistance, a_pos.z + 50.0f};
        pickData.SetStartEnd(rayStart, rayEnd);
        pickData.collisionFilter.filter = a_filter.filter;
        // Find the hitpoint along the ray
        auto* hitObj = player->parentCell->Pick(pickData);
        if (pickData.HasHit()) {
            // fraction is along the ray from rayStart->rayEnd (0..1)
            float fraction = pickData.GetHitFraction();
            float hitDist = fraction * scanDistance;
            if (hitDist < bestDist) {
                bestDist = hitDist;
                bestDirX = dx;
                bestDirY = dy;
                found = true;
            }
        }
    }
    // If hit, move position towards closest hit point
    if (found) {
        // Move position towards closest hit point
        a_pos.x += bestDirX * moveDistance;
        a_pos.y += bestDirY * moveDistance;
    }
    return a_pos;
}

// Helper to find the ground Z coordinate at a given X,Y position
float GetPointZ_Internal(RE::NiPoint3 a_pos, RE::CFilter a_filter, float a_scanDistanceUp, float a_scanDistanceDown) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell)
        return a_pos.z;
    // Get the physics world from the current cell
    auto* world = player->parentCell->GetbhkWorld();
    if (!world)
        return a_pos.z;
    // Offset matrix to test multiple rays around the position
    RE::NiPoint3 offsets[] = {
        { 0.0f, 0.0f, 0.0f }, // The center point
        { 10.0f, 0.0f, 0.0f },
        { -10.0f, 0.0f, 0.0f },
        { 0.0f, 10.0f, 0.0f },
        { 0.0f, -10.0f, 0.0f } // Cross pattern around the point
    };
    int hits = 0;
    double sumZ = 0.0;
    for (auto& off : offsets) {
        RE::bhkPickData pickData; // Initializing PickData every iteration to reset previous hit info
        pickData.collisionFilter.filter = a_filter.filter;
        RE::NiPoint3 rayStart{ a_pos.x + off.x, a_pos.y + off.y, a_pos.z + a_scanDistanceUp };
        RE::NiPoint3 rayEnd  { a_pos.x + off.x, a_pos.y + off.y, a_pos.z - a_scanDistanceDown };
        pickData.SetStartEnd(rayStart, rayEnd);
            player->parentCell->Pick(pickData);
            if (pickData.HasHit()) {
                float frac = pickData.GetHitFraction();
                float hitZ = rayStart.z + (rayEnd.z - rayStart.z) * frac;
                sumZ += hitZ;
                ++hits;
            }
        }
    if (hits > 0) {
        // Return average Z of all hits
        return static_cast<float>(sumZ / hits);
    }
    // Return original if no ground was found
    return a_pos.z;
}

// Helper function to convert a Papyrus slot index to a bitmask.
// This function now handles all slots from 30 to 61 using a bitwise shift.
std::uint32_t GetSlotMaskFromIndex_Internal(std::int32_t aiSlotIndex) {
    if (aiSlotIndex >= 30 && aiSlotIndex <= 61) {
        return 1 << (aiSlotIndex - 30);
    }
    return 0;
}

// Helper function to heal an actor downed condition
void HealActorDowned_Internal(RE::Actor* actor) {
    if (!actor)
        return;
    // Set downed condition to 0 (not downed)
    actor->SetActorValue(*g_actorValueHCDowned, 0.0f);
}

// Helper function to heal all actor conditions to max
void HealActorLimbs_Internal(RE::Actor* actor) {
    if (!actor) {
        if (DEBUGGING)
            REX::WARN("HealActorLimbs_Internal: Actor pointer is null");
        return;
    }
    auto* enduranceInfo = g_actorValueSingleton->enduranceCondition;
    auto* leftAttackInfo = g_actorValueSingleton->leftAttackCondition;
    auto* rightAttackInfo = g_actorValueSingleton->rightAttackCondition;
    auto* leftMobilityInfo = g_actorValueSingleton->leftMobiltyCondition;
    auto* rightMobilityInfo = g_actorValueSingleton->rightMobilityCondition;
    auto* brainInfo = g_actorValueSingleton->brainCondition;
    float current = actor->GetActorValue(*enduranceInfo);
    float max = actor->GetBaseActorValue(*enduranceInfo);
    if (current < max) {
        actor->RestoreActorValue(*enduranceInfo, max - current);
    }
    current = actor->GetActorValue(*leftAttackInfo);
    max = actor->GetBaseActorValue(*leftAttackInfo);
    if (current < max) {
        actor->RestoreActorValue(*leftAttackInfo, max - current);
    }
    current = actor->GetActorValue(*rightAttackInfo);
    max = actor->GetBaseActorValue(*rightAttackInfo);
    if (current < max) {
        actor->RestoreActorValue(*rightAttackInfo, max - current);
    }
    current = actor->GetActorValue(*leftMobilityInfo);
    max = actor->GetBaseActorValue(*leftMobilityInfo);
    if (current < max) {
        actor->RestoreActorValue(*leftMobilityInfo, max - current);
    }
    current = actor->GetActorValue(*rightMobilityInfo);
    max = actor->GetBaseActorValue(*rightMobilityInfo);
    if (current < max) {
        actor->RestoreActorValue(*rightMobilityInfo, max - current);
    }
    current = actor->GetActorValue(*brainInfo);
    max = actor->GetBaseActorValue(*brainInfo);
    if (current < max) {
        actor->RestoreActorValue(*brainInfo, max - current);
    }
}

// Helper function to heal an actor to % of max health
void HealActorHealth_Internal(RE::Actor* actor, float healthPercent) {
    if (!actor) {
        if (DEBUGGING)
            REX::WARN("HealActor_Internal: Actor pointer is null");
        return;
    }
    auto* healthAV = g_actorValueSingleton->health;
    if (!healthAV) {
        if (DEBUGGING)
            REX::WARN("HealActor_Internal: Health ActorValue pointer is null");
        return;
    }
    float currentHealth = actor->GetActorValue(*healthAV);
    float maxHealth = actor->GetPermanentActorValue(*healthAV);
    float targetHealth = maxHealth * (healthPercent / 100.0f);
    float delta = targetHealth - currentHealth;
    if (delta > 0.0f) {
        actor->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *healthAV, delta);
        float newHealth = actor->GetActorValue(*healthAV);
        float newPercent = (maxHealth > 0) ? (newHealth / maxHealth) * 100.0f : 0.0f;
        if (DEBUGGING)
            REX::INFO("HealActorHealth_Internal: Healed actor {} from {:.2f} to {:.2f} health ({:.2f}%).", actor->GetDisplayFullName(), currentHealth, newHealth, newPercent);
    }
}

// Helper function to heal all actor Power Armour
void HealActorPA_Internal(RE::Actor* actor) {
    if (!actor) return;
    // Power armour are equpped items that have the ArmorTypePower keyword
    auto* invList = actor->inventoryList;
    if (!invList) return;
    for (auto& item : invList->data) {
        if (!IsArmorItem_Internal(item.object))
            continue;
        auto* armor = item.object->As<RE::TESObjectARMO>();
        if (!armor)
            continue;
        // Armor pieces but not the frame itself
        if (IsArmorPower_Internal(&item) && !armor->HasKeyword(g_kwdIsPowerArmorFrame)) {
            if (!item.stackData)
                continue;
            if (!item.stackData->IsEquipped())
                continue; // only repair equipped items
            RE::BSTSmartPointer<RE::BGSInventoryItem::Stack> stackData = item.stackData;
            if (stackData && stackData->extra) {
                // Access ExtraHealth directly instead of using methods
                auto* extraHealth = stackData->extra->GetByType<RE::ExtraHealth>();
                float curPct = stackData->extra->GetHealthPercent();   // 0..1
                if (curPct < 1.0f) {
                    stackData->extra->SetHealthPercent(0.99f); // if health value is invalid, set to 90%
                    if (DEBUGGING)
                        REX::INFO("HealActorPA_Internal: Repaired power armor item: {} from invalid value to 90.00%.", armor->GetFullName());
                    continue;
                }
                if (curPct >= 0.99f)
                    continue; // already nearly maxed
                float newPct = std::clamp(curPct + PA_REPAIR_AMOUNT, 0.0f, 1.0f); // heal 1% and cap at 100%
                stackData->extra->SetHealthPercent(newPct);
                if (DEBUGGING)
                    REX::INFO("HealActorPA_Internal: Repaired power armor item: {} from {:.2f}% to {:.2f}%.", armor->GetFullName(), curPct * 100.0f, newPct * 100.0f);
            }
        }
    }
}

// Initialize global variables
void InitializeVariables_Internal() {
    if (g_perkList.empty() || g_perkList.size() < PERK_ID_LIST.size()) {
        g_perkList.clear();
        for (std::uint32_t perkID : PERK_ID_LIST) {
            auto* perk = GetFormByFileAndID_Internal<RE::BGSPerk>(perkID);
            if (perk) {
                g_perkList.push_back(perk);
                if (DEBUGGING)
                    REX::INFO("InitializeVariables_Internal: Perk found with ID 0x{:08X}", perkID);
            } else {
                if (DEBUGGING)
                    REX::WARN("InitializeVariables_Internal: Perk with ID 0x{:08X} not found", perkID);
            }
        }
    }
    if (g_keywordList.empty() || g_keywordList.size() < KEYWORD_ID_LIST.size()) {
        g_keywordList.clear();
        for (std::uint32_t keywordID : KEYWORD_ID_LIST) {
            auto* keyword = GetFormByFileAndID_Internal<RE::BGSKeyword>(keywordID);
            if (keyword) {
                g_keywordList.push_back(keyword);
                if (DEBUGGING)
                    REX::INFO("InitializeVariables_Internal: Keyword found with ID 0x{:08X}", keywordID);
            } else {
                if (DEBUGGING)
                    REX::WARN("InitializeVariables_Internal: Keyword with ID 0x{:08X} not found", keywordID);
            }
        }
    }
    // Initialize companion faction pointer
    if (!g_companionFaction) {
        g_companionFaction = GetFormByFileAndID_Internal<RE::TESFaction>(CURRENT_COMPANION_FACTION_ID);
        if (g_companionFaction) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Companion faction found with ID 0x{:08X}", CURRENT_COMPANION_FACTION_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Companion faction with ID 0x{:08X} not found", CURRENT_COMPANION_FACTION_ID);
        }
    }
    // Initialize actor value pointers
    if (!g_actorValueHCDowned) {
        g_actorValueHCDowned = GetFormByFileAndID_Internal<RE::ActorValueInfo>(ACTORVALUE_HC_DOWNED_ID);
        if (g_actorValueHCDowned) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: HCDowned ActorValue pointer initialized");
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: HCDowned ActorValue pointer not found");
        }
    }
    // Initialize stimpak and repair kit items
    if (!g_itemStimpak) {
        g_itemStimpak = GetFormByFileAndID_Internal<RE::AlchemyItem>(ITEM_STIMPAK_ID);
        if (g_itemStimpak) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Stimpak item found with ID 0x{:08X}", ITEM_STIMPAK_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Stimpak item with ID 0x{:08X} not found", ITEM_STIMPAK_ID);
        }
    }
    if (!g_itemRepairKit) {
        // This is in DLCRobot.esm
        g_itemRepairKit = GetFormByFileAndID_Internal<RE::AlchemyItem>(ITEM_REPAIRKIT_ID, "DLCRobot.esm");
        if (g_itemRepairKit) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Repair Kit item found with ID 0x{:08X}", ITEM_REPAIRKIT_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Repair Kit item with ID 0x{:08X} not found", ITEM_REPAIRKIT_ID);
        }
    }
    // Race form for stimpak usage check
    if (g_raceStimpak.size() == 0 || g_raceStimpak.size() < RACE_STIMPAK_ID.size()) {
        g_raceStimpak.clear();
        for (std::uint32_t raceID : RACE_STIMPAK_ID) {
            auto* race = GetFormByFileAndID_Internal<RE::TESRace>(raceID);
            if (race) {
                g_raceStimpak.push_back(race);
            }
        }
    }
    // initialize animation idles
    if (!g_idleStimpak) {
        g_idleStimpak = GetFormByFileAndID_Internal<RE::TESIdleForm>(IDLE_STIMPAK_ID);
        if (g_idleStimpak) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Stimpak idle found with ID 0x{:08X}", IDLE_STIMPAK_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Stimpak idle with ID 0x{:08X} not found", IDLE_STIMPAK_ID);
        }
    }
    // Keywords
    if (!g_kwdArmorTypePower) {
        g_kwdArmorTypePower = GetFormByFileAndID_Internal<RE::BGSKeyword>(KYWD_ARMORTYPEPOWER_ID);
        if (g_kwdArmorTypePower) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Power Armor Type keyword found with ID 0x{:08X}", KYWD_ARMORTYPEPOWER_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Power Armor Type keyword with ID 0x{:08X} not found", KYWD_ARMORTYPEPOWER_ID);
        }
    }
    if (!g_kwdIsPowerArmorFrame) {
        g_kwdIsPowerArmorFrame = GetFormByFileAndID_Internal<RE::BGSKeyword>(KYWD_ISPOWERARMORFRAME_ID);
        if (g_kwdIsPowerArmorFrame) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Is Power Armor keyword found with ID 0x{:08X}", KYWD_ISPOWERARMORFRAME_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Is Power Armor keyword with ID 0x{:08X} not found", KYWD_ISPOWERARMORFRAME_ID);
        }
    }
    if (!g_kwdAnimal) {
        g_kwdAnimal = GetFormByFileAndID_Internal<RE::BGSKeyword>(KYWD_ANIMAL_ID);
        if (g_kwdAnimal) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Animal keyword found with ID 0x{:08X}", KYWD_ANIMAL_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Animal keyword with ID 0x{:08X} not found", KYWD_ANIMAL_ID);
        }
    }
    if (!g_kwdRobot) {
        g_kwdRobot = GetFormByFileAndID_Internal<RE::BGSKeyword>(KYWD_ROBOT_ID);
        if (g_kwdRobot) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Robot keyword found with ID 0x{:08X}", KYWD_ROBOT_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Robot keyword with ID 0x{:08X} not found", KYWD_ROBOT_ID);
        }
    }
    if (!g_kwdSynth) {
        g_kwdSynth = GetFormByFileAndID_Internal<RE::BGSKeyword>(KYWD_SYNTH_ID);
        if (g_kwdSynth) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Synth keyword found with ID 0x{:08X}", KYWD_SYNTH_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Synth keyword with ID 0x{:08X} not found", KYWD_SYNTH_ID);
        }
    }
    if (g_kwdLootExclude.size() == 0 || g_kwdLootExclude.size() < KYWD_LOOTEXCLUDE_ID_LIST.size()) {
        g_kwdLootExclude.clear();
        for (std::uint32_t keywordID : KYWD_LOOTEXCLUDE_ID_LIST) {
            auto* keyword = GetFormByFileAndID_Internal<RE::BGSKeyword>(keywordID);
            if (keyword) {
                g_kwdLootExclude.push_back(keyword);
                if (DEBUGGING)
                    REX::INFO("InitializeVariables_Internal: Loot Exclude keyword found with ID 0x{:08X}", keywordID);
            } else {
                if (DEBUGGING)
                    REX::WARN("InitializeVariables_Internal: Loot Exclude keyword with ID 0x{:08X} not found", keywordID);
            }
        }
    }
    if (g_itemLootAlways.size() == 0 || g_itemLootAlways.size() < ITEM_LOOTALWAYS_ID_LIST.size()) {
        g_itemLootAlways.clear();
        for (std::uint32_t itemID : ITEM_LOOTALWAYS_ID_LIST) {
            auto* item = GetFormByFileAndID_Internal<RE::TESForm>(itemID);
            if (item) {
                g_itemLootAlways.push_back(item);
                if (DEBUGGING)
                    REX::INFO("InitializeVariables_Internal: Loot Always item found with ID 0x{:08X}", itemID);
            } else {
                if (DEBUGGING)
                    REX::WARN("InitializeVariables_Internal: Loot Always item with ID 0x{:08X} not found", itemID);
            }
        }
    }
    // Packages
    if (!g_packFollowersCompanion) {
        g_packFollowersCompanion = GetFormByFileAndID_Internal<RE::TESPackage>(PACK_FOLLOWERSCOMPANION_ID);
        if (g_packFollowersCompanion) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: Follow Player package found with ID 0x{:08X}", PACK_FOLLOWERSCOMPANION_ID);
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: Follow Player package with ID 0x{:08X} not found", PACK_FOLLOWERSCOMPANION_ID);
        }
    }
    // GetSingletons
    if (!g_actorValueSingleton) {
        g_actorValueSingleton = RE::ActorValue::GetSingleton();
        if (g_actorValueSingleton) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: ActorValue singleton initialized");
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: ActorValue singleton not found");
        }
    }
    if (!g_processListsSingleton) {
        g_processListsSingleton = RE::ProcessLists::GetSingleton();
        if (g_processListsSingleton) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: ProcessLists singleton initialized");
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: ProcessLists singleton not found");
        }
    }
    if (!g_actorEquipMgrSingleton) {
        g_actorEquipMgrSingleton = RE::ActorEquipManager::GetSingleton();
        if (g_actorEquipMgrSingleton) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: ActorEquipManager singleton initialized");
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: ActorEquipManager singleton not found");
        }
    }
    if (!g_uiSingleton) {
        g_uiSingleton = RE::UI::GetSingleton();
        if (g_uiSingleton) {
            if (DEBUGGING)
                REX::INFO("InitializeVariables_Internal: UI singleton initialized");
        } else {
            if (DEBUGGING)
                REX::WARN("InitializeVariables_Internal: UI singleton not found");
        }
    }
}

// Helper function to check if an actor is a companion
bool IsActorActiveCompanion_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    // Check for a specific faction, keyword, or other criteria that defines a companion
    if (!g_companionFaction) {
        if (DEBUGGING)
            REX::WARN("IsActorActiveCompanion_Internal: Companion faction with ID 0x{:08X} not found", CURRENT_COMPANION_FACTION_ID);
        return false;
    }
    // Pre-check if actor is in companion faction
    if (!actor->IsInFaction(g_companionFaction)) {
        return false;
    }
    // Further check if actor is actively following the player or temporary in combat
    if (actor->IsFollowing() || actor->IsInCombat() || actor->IsDead(false))
        return true;
    // If not following or in combat, do not consider as active companion
    return false;
}

// Helper function to check if an actor is an enemy
bool IsActorEnemy_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    // Check if actor is being deleted or in bad state
    if (actor->IsDeleted())
        return false;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        if (DEBUGGING)
            REX::WARN("IsActorEnemy_Internal: Player pointer is null");
        return false;
    }
#ifdef _MSC_VER
    __try {
        return actor->GetHostileToActor(player);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (DEBUGGING)
            REX::WARN("IsActorEnemy_Internal: Crash prevented calling GetHostileToActor on actor");
        return false; // Treat crashed check as non-hostile
    }
#else
    return actor->GetHostileToActor(player);
#endif
}

// Check if an actor is in the exclusion list
bool IsActorExcluded_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    auto* npc = actor->GetNPC();
    if (!npc)
        return false;
    // Check against exclusion list
    auto npcRefFormID = actor->GetFormID();
    auto npcBaseFormID = npc->GetFormID();
    auto otherForm = actor->GetObjectReference();
    std::uint32_t otherFormID = 0;
    if (otherForm) {
        otherFormID = otherForm->GetFormID();
    }
    for (std::uint32_t excludedID : EXCLUDE_ACTOR_ID_LIST) {
        // Check any of the relevant form IDs
        if (npc->formID == excludedID) {
            return true;
        }
        if (npcRefFormID == excludedID) {
            return true;
        }
        if (npcBaseFormID == excludedID) {
            return true;
        }
        if (otherFormID == excludedID) {
            return true;
        }
    }
    return false;
}

// Helper function to check if an actor is currently in a scene
bool IsActorInScene_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    auto* currentScene = actor->GetCurrentScene();
    if (currentScene) {
        if (DEBUGGING)
            REX::INFO("IsActorInScene_Internal: Actor {} is in scene {}", actor->GetDisplayFullName(), currentScene->GetObjectTypeName());
        return true;
    }
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        if (DEBUGGING)
            REX::WARN("IsActorInScene_Internal: Player pointer is null returning true for safety");
        return true;
    }
    if (player->sceneActionActive) {
        if (DEBUGGING)
            REX::INFO("IsActorInScene_Internal: Player has an active scene action, assuming actor {} is in scene", actor->GetDisplayFullName());
        return true;
    }
    return false;
}

// Helper function to check if an item is already equipped
bool IsActorItemEquipped_Internal(RE::Actor* actor, RE::BGSInventoryItem* invItem) {
    if (!actor || !invItem)
        return false;
    if (!actor->currentProcess || !actor->currentProcess->middleHigh)
        return false;
    auto& actorEquippedItems = actor->currentProcess->middleHigh->equippedItems;
    for (const auto& entry : actorEquippedItems) {
        if (!entry.item.object)
            continue;
        // Checking object pointers first is slightly faster
        if (entry.item.object == invItem->object)
            return true;
        // Fallback check on FormID
        if (entry.item.object->GetFormID() == invItem->object->GetFormID())
            return true;
    }
    return false;
}

// Helper function to check if an actor is the player or a companion
bool IsActorPlayerOrCompanion_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    // Check if player
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (actor->GetFormID() == player->GetFormID()) {
        return true;
    }
    return IsActorActiveCompanion_Internal(actor); // Check if companion
}

// Helper function to check if an actor is a humanoid
bool IsActorRaceHumanoid_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    auto* race = actor->race;
    if (!race) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor {} has no race assigned, not humanoid.", actor->GetDisplayFullName());
        return false;
    }
    // Get the number of biped object slots to determine if the model can have animations
    auto* biped = reinterpret_cast<RE::BGSBipedObjectForm*>(race);
    if (biped == nullptr) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has no BGSBipedObjectForm, not humanoid.", race->GetFullName());
        return false;
    }
    uint32_t definedSlots = biped->bipedModelData.bipedObjectSlots;
    //uint32_t definedSlots = race->RE::BGSBipedObjectForm::GetFilledSlots();
    if (definedSlots < 12) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has only {} defined biped slots, not humanoid.", race->GetFullName(), definedSlots);
        return false;
    }
    // Check race flags for flags of non-humanoid types
    if (!race->data.flags2) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has no flags2 defined, not humanoid.", race->GetFullName());
        return false;
    }
    auto flags2 = race->data.flags2;
    // This is the flag for "Cannot Use Playable Items" which indicates robots, gorillas, etc
    // Missing animations for idles and other actions
    if (race->data.flags2 & (1 << 10)) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has flag Cannot Use Playable Items, not humanoid.", race->GetFullName());
        return false;
    }
    if (g_kwdAnimal && actor->HasKeyword(g_kwdAnimal)) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has Keyword Animal, not humanoid.", race->GetFullName());
        return false;
    }
    // Check if it's a synth race and allow non-humanoid synths
    if (IsActorRaceSynth_Internal(actor)) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} is a synth, not humanoid.", race->GetFullName());
        return true;
    }
    // Check for Robot keyword last, not synth but still non-humanoid
    if (g_kwdRobot && actor->HasKeyword(g_kwdRobot)) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceHumaniod_Internal: Actor with race {} has Keyword Robot, not humanoid.", race->GetFullName());
        return false;
    }
    return true;
}

// Helper function to check if an actor is a synth
bool IsActorRaceSynth_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    auto* race = actor->race;
    if (!race) {
        if (DEBUGGING)
            REX::INFO("IsActorRaceSynth_Internal: Actor {} has no race assigned, not synth.", actor->GetDisplayFullName());
        return false;
    }
    if (g_raceStimpak.empty()) {
        if (DEBUGGING)
            REX::WARN("IsActorRaceSynth_Internal: Stimpak race list is empty");
        return false;
    }
    // Check if race is in the stimpak race list
    for (auto* raceStimpak : g_raceStimpak) {
        if (race->GetFormID() == raceStimpak->GetFormID()) {
            // Check for robot keyword
            if ((g_kwdRobot && actor->HasKeyword(g_kwdRobot)) || (g_kwdSynth && actor->HasKeyword(g_kwdSynth))) {
                // handle robot case
                if (DEBUGGING)
                    REX::INFO("IsActorRaceSynth_Internal: Actor with race {} is synth.", race->GetFullName());
                return true;
            }
        }
    }
    return false;
}

// Helper function to check if an actor is a vendor
bool IsActorVendor_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    return actor->vendorFaction != nullptr;
}

// Helper function to check if an actor has reached their weight limit
bool IsActorWeightLimit_Internal(RE::Actor* actor) {
    if (!actor)
        return false;
    auto* avCarryWeight = g_actorValueSingleton->carryWeight;
    if (!avCarryWeight)
        return true; // cannot check carry weight return true
    float maxWeight = actor->GetActorValue(*avCarryWeight);
    auto* invItems = actor->inventoryList;
    if (!invItems)
        return true; // cannot check inventory return true
    // Calculate total weight manually since cachedWeight is unreliable
    float currentWeight = 0.0f;
    for (auto& item : invItems->data) {
        if (!item.object)
            continue;
        
        float itemWeight = RE::TESWeightForm::GetFormWeight(item.object, nullptr);
        int itemCount = item.GetCount();
        currentWeight += itemWeight * static_cast<float>(itemCount);
    }
    if (!avCarryWeight)
        return true; // cannot check carry weight return true
    if (DEBUGGING)
        REX::INFO("IsActorWeightLimit_Internal: Actor {} carries {:.2f} of max carry weight: {:.2f}", actor->GetDisplayFullName(), currentWeight, maxWeight);
    if (currentWeight >= maxWeight) {
        return true; // weight limit reached
    }
    return false; // weight limit not reached
}

// Helper to check if an item is an armor
bool IsArmorItem_Internal(RE::TESForm* itemForm) {
    if (!itemForm)
        return false;
    auto formType = itemForm->GetFormType();
    return (formType == RE::ENUM_FORM_ID::kARMO);
}

// Helper to check if an object is a power armor frame
bool IsArmorPower_Internal(RE::TESObjectREFR* armor) {
    if (!armor)
        return false;
    // Is this reference actually an armor object (not a container/actor)?
    auto* baseForm = armor->GetObjectReference();
    if (!baseForm || baseForm->GetFormType() != RE::ENUM_FORM_ID::kARMO)
        return false;
    // Fallback: check keywords on the armor object
    if (!g_kwdIsPowerArmorFrame && !g_kwdArmorTypePower)
        return false;
    if (armor->HasKeyword(g_kwdArmorTypePower) || armor->HasKeyword(g_kwdIsPowerArmorFrame))
        return true;
    return false;
}
bool IsArmorPower_Internal(RE::BGSInventoryItem* invArmor) {
    if (!invArmor)
        return false;
    // Is this reference actually an armor object?
    auto* armor = invArmor->object->As<RE::TESObjectARMO>();
    if (!armor)
        return false;
    // Fallback: check keywords on the armor object
    if (!g_kwdIsPowerArmorFrame && !g_kwdArmorTypePower)
        return false;
    if (armor->HasKeyword(g_kwdArmorTypePower) || armor->HasKeyword(g_kwdIsPowerArmorFrame))
        return true;
    return false;
}

// Helper to check if an object is a power armor frame
bool IsArmorPowerFrame_Internal(RE::TESObjectREFR* armor) {
    if (!armor)
        return false;
    // Get the base form
    auto* baseForm = armor->GetObjectReference();
    if (!baseForm) {
        return false;
    }
    // Power armor frames are furniture, not armor
    if (baseForm->GetFormType() != RE::ENUM_FORM_ID::kFURN) {
        return false;
    }
    // Check keywords on the armor object
    if (!g_kwdIsPowerArmorFrame && !g_kwdArmorTypePower) {
        return false;
    }
    // Check keywords on the TESObjectREFR
    if (armor->HasKeyword(g_kwdArmorTypePower) || armor->HasKeyword(g_kwdIsPowerArmorFrame)) {
        return true;
    }
    // Fallback: Check editor ID for "PowerArmor" string
    if (baseForm->GetFormEditorID()) {
        std::string editorID(baseForm->GetFormEditorID());
        if (editorID.find("PowerArmor") != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Crash-resistant check if an item is owned by the player, returns true on failure to prevent looting
bool IsItemOwnedByPlayer_Internal(RE::TESObjectREFR* source) {
    if (!source)
        return false;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player)
        return false;
    // Initialize result
    bool ownerIsPlayer = false;
    // Get the owner form
    RE::TESForm* owner = nullptr;
    // The whole GetOwner() call can crash on bad pointers, so wrap in SEH
#ifdef _MSC_VER
    __try {
        owner = source->GetOwner();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return true; // GetOwner crashed, treat as owned by player to prevent looting
    }
#else
    owner = source->GetOwner();
#endif
    if (!owner)
        return false;
    // Fast pointer check (no dereference)
    if (owner == player) return true;
    if (owner && player) {
#ifdef _MSC_VER
        __try {
            ownerIsPlayer = (owner->GetFormID() == player->GetFormID());
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ownerIsPlayer = true; // GetFormID crashed, treat as owned by player to prevent looting
        }
#else
        // Non-MSVC: be conservative — avoid calling into possibly-bad pointer
        // Optionally call GetFormID() if you trust owner to be valid on this platform
        ownerIsPlayer = (owner->GetFormID() == player->GetFormID());
#endif
    }
    return ownerIsPlayer;
}

// Helper to check if a reference is of a lootable form type
bool IsLootableFormType_Internal(RE::TESObjectREFR* source) {
    if (!source)
        return false;
    auto* baseForm = source->GetObjectReference();
    if (!baseForm)
        return false;
    auto formType = baseForm->GetFormType();
    // Check against lootable form types
    for (auto excludedType : LOOTABLE_FORM_TYPES) {
        if (formType == excludedType) {
            return true; // Found in lootable types
        }
    }
    return false; // Not found in lootable types
}

// Helper to check if an item is in the always loot list
bool IsLootAlways_Internal(RE::TESForm* itemForm) {
    if (!itemForm)
        return false;
    for (auto* alwaysItem : g_itemLootAlways) {
        if (itemForm->GetFormID() == alwaysItem->GetFormID()) {
            return true; // Item is in the always loot list
        }
    }
    return false; // Item not in the always loot list
}

// Helper to check if an item has any exclusion keywords
bool IsLootKeywordExcluded_Internal(RE::TESForm* itemForm) {
    if (!itemForm)
        return false;
    auto* formKeywords = itemForm->As<RE::BGSKeywordForm>();
    if (!formKeywords)
        return false;
    // Check against exclusion keywords
    for (auto* excludeKwd : g_kwdLootExclude) {
        if (formKeywords->HasKeyword(excludeKwd)) {
            return true; // Item has an exclusion keyword
        }
    }
    return false; // No exclusion keywords found
}

// Helper to check if any menu is open
bool IsMenuOpen_Internal() {
    if (!g_uiSingleton)
        return false;
    // Go over menu stack and check if any menu is open
    for (const auto& menuPtr : g_uiSingleton->menuStack) {
        if (menuPtr->menuFlags.any(RE::UI_MENU_FLAGS::kInventoryItemMenu) ||
            menuPtr->menuFlags.any(RE::UI_MENU_FLAGS::kPausesGame) ||
            menuPtr->menuFlags.any(RE::UI_MENU_FLAGS::kModal)) {
            if (g_uiSingleton->GetMenuOpen(menuPtr->menuName))
                return true;
            if (g_uiSingleton->GetMenuOpen("BarterMenu")) {
                return true;
            }
        }
    }
    return false;
}

// Helper to check if an item is a weapon
bool IsWeaponItem_Internal(RE::TESForm* itemForm) {
    if (!itemForm)
        return false;
    auto formType = itemForm->GetFormType();
    return (formType == RE::ENUM_FORM_ID::kWEAP);
}

// Loot items from all references in the current cell by active companions in the loot radius
std::int32_t LootItems_Internal(std::vector<TrackedActorData> companionData) {
    if (companionData.empty()) return 0;
    std::vector<RE::TESObjectREFR*> objectReferences = GetAllLootReferencesInCurrentCell_Internal();
    // Clear looted items set before looting
    if (objectReferences.size() > 0) {
        g_lootedItems.clear(); // Clear looted items set before looting
    }
    std::int32_t lootedRefCount = 0;
    for (auto* object : objectReferences) {
        if (!object)
            continue;
        // Check if the object has an owner and LOOT_STEAL is false
        if (object->IsCrimeToActivate() && LOOT_STEAL == false)
            continue;
        // Get the total weight of the objects items
        float objectWeight = object->GetWeightInContainer();
        // iterate through companions to find the closest one
        RE::Actor* closestCompanion = nullptr;
        // Only consider companions within LOOT_RADIUS
        float closestDistance = LOOT_RADIUS;
        auto* carryweightAV = g_actorValueSingleton->carryWeight;
        for (auto& companion : companionData) {
            // Skip if none or in combat and LOOT_COMBAT is false
            if (!companion.actor)
                continue;
            if (companion.isAlerted && !LOOT_COMBAT)
                continue;
            if (companion.actor->IsDead(false))
                continue;
            if (IsActorInScene_Internal(companion.actor))
                continue;
            if (LOOT_WEIGHT_LIMIT && IsActorWeightLimit_Internal(companion.actor)) {
                continue; // Cannot carry more weight
            }
            // Calculate distance
            float distance = GetActorDistanceToObject_Internal(companion.actor, object);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestCompanion = companion.actor;
            }
        }
        if (closestCompanion) {
            if (LootItemsFromReference_Internal(object, closestCompanion)) {
                lootedRefCount++;
            }
        }
    }
    return lootedRefCount;
}

// Filter function to determine if an item should be looted
bool LootItemFilter_Internal(RE::TESForm* aForm) {
    if (!aForm)
        return false;
    // Check for keyword exclusion
    auto* formKeywords = aForm->As<RE::BGSKeywordForm>();
    if (IsLootKeywordExcluded_Internal(aForm) && !IsLootAlways_Internal(aForm)) {
        return false; // Do not loot items with exclusion keywords
    }
    // Get the form type
    auto formType = aForm->GetFormType();
    // Check against INI settings
    switch (formType) {
    case RE::ENUM_FORM_ID::kMISC: // Junk items
        return LOOT_JUNK;
    case RE::ENUM_FORM_ID::kAMMO: // Ammunition
        return LOOT_AMMO;
    case RE::ENUM_FORM_ID::kALCH: // Aid items (stimpaks, chems)
        return LOOT_AID;
    case RE::ENUM_FORM_ID::kWEAP: // Weapons
        if (!LOOT_GEAR)
            return false;
        if (!IsWeaponItem_Internal(aForm))
            return false; // Sanity check for CTD
        if (!aForm->GetPlayable(nullptr)) // Do not loot quest or unplayable items
            return false;
        if (auto* weapon = aForm->As<RE::TESObjectWEAP>()) {
            if (weapon && weapon->weaponData.value) {
                return weapon->weaponData.value >= LOOT_MIN_VALUE && weapon->weaponData.value <= LOOT_MAX_VALUE;
            }
        }
    case RE::ENUM_FORM_ID::kARMO: // Armor
        if (!LOOT_GEAR)
            return false;
        if (!IsArmorItem_Internal(aForm))
            return false; // Sanity check for CTD
        if (!aForm->GetPlayable(nullptr)) // Do not loot quest or unplayable items
            return false;
        if (auto* armor = aForm->As<RE::TESObjectARMO>()) {
            // Check value
            if (armor && armor->armorData.value) {
                return armor->armorData.value >= LOOT_MIN_VALUE && armor->armorData.value <= LOOT_MAX_VALUE;
            }
        }
    default:
        return false; // Don't loot unknown types
    }
}

// Helper to find and pick up dropped weapons near a corpse
bool LootItemsWeaponLooseNearCorpse_Internal(RE::TESObjectREFR* looseItem, RE::Actor* companion) {
    if (!looseItem || !companion)
        return false;
    // Must be a weapon
    auto* baseForm = looseItem->GetObjectReference();
    if (!baseForm)
        return false;
    if (!IsWeaponItem_Internal(baseForm))
        return false;
    // Apply loot filter for value check
    if (!LootItemFilter_Internal(baseForm))
        return false;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->parentCell)
        return false;
    auto* currentCell = player->parentCell;
    // Skips alive actors
    auto AllReferences = GetAllLootReferencesInCurrentCell_Internal();
    // Get the position of the loose item
    auto looseItemPos = looseItem->GetPosition();
    bool pickedUpAny = false;
    // Search cell references for weapons near this loose item
    for (auto* ref : AllReferences) {
        if (!ref) continue;
        auto* refBaseForm = ref->GetObjectReference();
        if (!refBaseForm) continue;
        // Check if reference is an actor
        if (refBaseForm->GetFormType() != RE::ENUM_FORM_ID::kACHR &&
            refBaseForm->GetFormType() != RE::ENUM_FORM_ID::kNPC_) {
            continue;
        }
        // Must be close to loose item (dropped weapons are usually within 50-100 units)
        float distance = ref->GetPosition().GetDistance(looseItemPos);
        if (distance > 100.0f) // Adjust radius as needed
            continue;
        // Check if already looted this item type to avoid duplicates
        auto formID = looseItem->GetFormID();
        if (!g_lootedItems.insert(formID).second) {
            continue; // was already present
        }
        // Pick it up
        companion->PickUpObject(looseItem, 1, false);
        pickedUpAny = true;
        break;
    }
    return pickedUpAny;
}

// Loot the items from a reference to a companion based on item filter
bool LootItemsFromReference_Internal(RE::TESObjectREFR* source, RE::Actor* companion) {
    if (!source || !companion)
        return false;
    // Check if source is owned by player
    if (IsItemOwnedByPlayer_Internal(source)) {
        return false; // Do not loot items owned by player
    }
    // Access the inventory list
    auto* invList = source->inventoryList;
    // Check if this is a power armor frame
    if (IsArmorPowerFrame_Internal(source)) {
        return false; // Do not loot power armor frames
    }
    // It's a loose item, we need to handle it first in case it is a dropped weapon
    if (!invList) {
        return LootItemsWeaponLooseNearCorpse_Internal(source, companion);
    }
    // It is a container, check if it has items
    if (invList->data.empty())
        return false; // No items to loot
    // Check Lock status
    RE::REFR_LOCK* lock = source->GetLock();
    if (lock && lock->GetLockLevel(source) != RE::LOCK_LEVEL::kUnlocked)
        return false; // Cannot loot locked containers
    // Cast RE::Actor to RE::TESObjectREFR for destination
    auto* companionRef = companion->As<RE::TESObjectREFR>();
    if (!companionRef)
        return false; // should always succeed
    // Get all the items to transfer
    auto totalItemCount = 0;
    std::vector<RE::BGSInventoryItem> toTransfer;
    for (auto & itemEntry : invList->data) {
        if (!itemEntry.object)
            continue;
        if (LootItemFilter_Internal(itemEntry.object)) {
            toTransfer.push_back(itemEntry);
        }
    }
    for (auto& itemEntry : toTransfer) {
        if (!itemEntry.object || itemEntry.GetCount() <= 0)
            continue;
        // Check if already looted this item type to avoid duplicates
        auto formID = itemEntry.object->GetFormID();
        if (!g_lootedItems.insert(formID).second) {
            continue; // was already present
        }
        // Get the count to transfer
        int itemCount = itemEntry.GetCount();
        totalItemCount += itemCount;
        // Transfer the item
        auto removeData = LootBuildRemoveItemData_Internal(&itemEntry, companionRef, itemCount);
        source->RemoveItem(removeData);
    }
    // Some items were looted
    if (totalItemCount > 0)
        return true;
    // No items were looted
    return false;
}

// Helper to construct the RemoveItemData for the RemoveItem(RemoveItemData zData) function
// std::int32_t
RE::TESObjectREFR::RemoveItemData LootBuildRemoveItemData_Internal(RE::BGSInventoryItem *aInventoryItem, RE::TESObjectREFR *aContainer, std::int32_t aCount) {
    // --- RemoveItemData ---
    // Main constructor that takes a TESBoundObject and the item count:
    //    RemoveItemData(TESBoundObject* a_object, std::int32_t a_count)
    // members
    // This array holds unique instance data, like FormIDs of attached mods,
    // which are part of the BGSInventoryItem::Stack.
    // BSTSmallArray<std::uint32_t, 4> stackData;
    // The base object of the item (e.g., TESObjectWEAP, TESObjectARMO).
    // TESBoundObject* object{ nullptr };
    // The number of items to remove.
    // std::int32_t count{ 0 };
    // An enum specifying the reason for the item's removal (e.g., kEquip, kStoreTeammate).
    // ITEM_REMOVE_REASON reason{ ITEM_REMOVE_REASON::kNone };
    // The destination container for the item if it's being transferred.
    // If null, the item is typically dropped.
    // TESObjectREFR* a_otherContainer{ nullptr };
    // Pointers to the position and rotation data if the item is being dropped in the world.
    // const NiPoint3* dropLoc{ nullptr };
    // const NiPoint3* rotate{ nullptr };
    // The constructor for RemoveItemData takes the base object and count.
    RE::TESObjectREFR::RemoveItemData newData(aInventoryItem->object, aCount);
    // Set the destination container and the reason for the transfer.
    newData.a_otherContainer = aContainer;
    // Needs to be RE::ITEM_REMOVE_REASON::kStoreTeammate to work properly
    newData.reason = RE::ITEM_REMOVE_REASON::kStoreTeammate;
    return newData;
}

void RemoveAIAggression_Internal(std::vector<TrackedActorData> companionData) {
    for (auto& companion : companionData) {
        auto* actor = companion.actor;
        if (!actor)
            continue;
        auto* npc = actor->GetNPC();
        if (!npc)
            continue;
        // Test if aiData is safely accessible
        if (!AIAccessTest_Internal(npc)) {
            continue;
        }
        // Apply changes to aggression radius
        if (npc->aiData.useAggroRadius != 0 ||
            npc->aiData.aggroRadius[0] != 0 ||
            npc->aiData.aggroRadius[1] != 0 ||
            npc->aiData.aggroRadius[2] != 0) {
            npc->aiData.useAggroRadius = 0;
            npc->aiData.aggroRadius[0] = 0;
            npc->aiData.aggroRadius[1] = 0;
            npc->aiData.aggroRadius[2] = 0;
        }
    }
}

// Set companion chatter multiplier
void SetCompanionChatter_Internal(RE::Actor* comp) {
    if (!comp)
        return;
    // Get current idle chatter AV values
    auto* idleChatterMinAV = g_actorValueSingleton->idleChatterTimeMin;
    auto* idleChatterMaxAV = g_actorValueSingleton->idleChatterTimeMAx;
    auto idleChatterMin = comp->GetActorValue(*idleChatterMinAV);
    auto idleChatterMax = comp->GetActorValue(*idleChatterMaxAV);
    auto idleChatterBaseMin = comp->GetBaseActorValue(*idleChatterMinAV);
    auto idleChatterBaseMax = comp->GetBaseActorValue(*idleChatterMaxAV);
    float targetMin = 0.0f;
    float targetMax = 0.0f;
    // Adjust based on sneaking status
    if (comp->IsSneaking()) {
        targetMin = idleChatterBaseMin * CHATTER_MULTIPLIER_SNEAK;
        targetMax = idleChatterBaseMax * CHATTER_MULTIPLIER_SNEAK;
    } else {
        targetMin = idleChatterBaseMin * CHATTER_MULTIPLIER;
        targetMax = idleChatterBaseMax * CHATTER_MULTIPLIER;
    }
    // Apply changes if different from current
    if (std::abs(idleChatterMin - targetMin) > 0.1f) {
        comp->SetActorValue(*idleChatterMinAV, targetMin);
    }
    if (std::abs(idleChatterMax - targetMax) > 0.1f) {
        comp->SetActorValue(*idleChatterMaxAV, targetMax);
    }
}

// Helper function to set companion combat AI parameters
void SetCompanionCombatAI_Internal(RE::Actor* comp, RE::TESCombatStyle* combatStyle) {
    if (!comp || !combatStyle)
        return;
    /* // General
    if (DEBUGGING) REX::INFO("SetCompanionCombatAI_Internal: Setting combat style for companion {}", comp->GetDisplayFullName());
    if (DEBUGGING) REX::INFO(" - Current Offensive Multiplier: {}", combatStyle->generalData.offensiveMult);
    if (DEBUGGING) REX::INFO(" - Current Defensive Multiplier: {}", combatStyle->generalData.defensiveMult);
    if (DEBUGGING) REX::INFO(" - Current Ranged Score Multiplier: {}", combatStyle->generalData.rangedScoreMult);
    if (DEBUGGING) REX::INFO(" - Current Melee Score Multiplier: {}", combatStyle->generalData.meleeScoreMult);
    // Ranged
    if (DEBUGGING) REX::INFO(" - Current Ranged Adjust Range Multiplier: {}", combatStyle->longRangeData.adjustRangeMult);
    if (DEBUGGING) REX::INFO(" - Current Ranged Crouch Multiplier: {}", combatStyle->longRangeData.crouchMult);
    if (DEBUGGING) REX::INFO(" - Current Ranged Strafe Multiplier: {}", combatStyle->longRangeData.strafeMult);
    if (DEBUGGING) REX::INFO(" - Current Ranged Wait Multiplier: {}", combatStyle->longRangeData.waitMult);
    if (DEBUGGING) REX::INFO(" - Current Ranged Accuracy Multiplier: {}", combatStyle->rangedData.accuracyMult);
    // Close-Quarters
    if (DEBUGGING) REX::INFO(" - Current Close Fallback Multiplier: {}", combatStyle->closeRangeData.fallbackMult);
    if (DEBUGGING) REX::INFO(" - Current Close Circle Multiplier: {}", combatStyle->closeRangeData.circleMult);
    if (DEBUGGING) REX::INFO(" - Current Close Disengage Probability: {}", combatStyle->closeRangeData.disengageProbability);
    if (DEBUGGING) REX::INFO(" - Current Close Flank Variance Multiplier: {}", combatStyle->closeRangeData.flankVarianceMult);
    if (DEBUGGING) REX::INFO(" - Current Close Throw Max Targets: {}", combatStyle->closeRangeData.throwMaxTargets);
    // Cover
    if (DEBUGGING) REX::INFO(" - Current Cover Search Distance Multiplier: {}", combatStyle->coverData.coverSearchDistanceMult); */
    // Apply new settings from INI
    if (combatStyle->generalData.offensiveMult != COMBAT_OFFENSIVE && COMBAT_OFFENSIVE != 1.0f)
        combatStyle->generalData.offensiveMult = COMBAT_OFFENSIVE;
    if (combatStyle->generalData.defensiveMult != COMBAT_DEFENSIVE && COMBAT_DEFENSIVE != 1.0f)
        combatStyle->generalData.defensiveMult = COMBAT_DEFENSIVE;
    if (combatStyle->generalData.rangedScoreMult != COMBAT_RANGED && COMBAT_RANGED != 1.0f)
        combatStyle->generalData.rangedScoreMult = COMBAT_RANGED;
    if (combatStyle->generalData.meleeScoreMult != COMBAT_MELEE && COMBAT_MELEE != 1.0f)
        combatStyle->generalData.meleeScoreMult = COMBAT_MELEE;
    // Ranged
    if (combatStyle->longRangeData.adjustRangeMult != COMBAT_RANGED_ADJUSTMENT && COMBAT_RANGED_ADJUSTMENT != 1.0f)
        combatStyle->longRangeData.adjustRangeMult = COMBAT_RANGED_ADJUSTMENT;
    if (combatStyle->longRangeData.crouchMult != COMBAT_RANGED_CROUCHING && COMBAT_RANGED_CROUCHING != 1.0f)
        combatStyle->longRangeData.crouchMult = COMBAT_RANGED_CROUCHING;
    if (combatStyle->longRangeData.strafeMult != COMBAT_RANGED_STRAFE && COMBAT_RANGED_STRAFE != 1.0f)
        combatStyle->longRangeData.strafeMult = COMBAT_RANGED_STRAFE;
    if (combatStyle->longRangeData.waitMult != COMBAT_RANGED_WAITING && COMBAT_RANGED_WAITING != 1.0f)
        combatStyle->longRangeData.waitMult = COMBAT_RANGED_WAITING;
    if (combatStyle->rangedData.accuracyMult != COMBAT_RANGED_ACCURACY && COMBAT_RANGED_ACCURACY != 1.0f)
        combatStyle->rangedData.accuracyMult = COMBAT_RANGED_ACCURACY;
    // Close-Quarters
    if (combatStyle->closeRangeData.fallbackMult != COMBAT_CLOSE_FALLBACK && COMBAT_CLOSE_FALLBACK != 1.0f)
        combatStyle->closeRangeData.fallbackMult = COMBAT_CLOSE_FALLBACK;
    if (combatStyle->closeRangeData.circleMult != COMBAT_CLOSE_CIRCLE && COMBAT_CLOSE_CIRCLE != 1.0f)
        combatStyle->closeRangeData.circleMult = COMBAT_CLOSE_CIRCLE;
    if (combatStyle->closeRangeData.disengageProbability != COMBAT_CLOSE_DISENGAGE && COMBAT_CLOSE_DISENGAGE != 1.0f)
        combatStyle->closeRangeData.disengageProbability = COMBAT_CLOSE_DISENGAGE;
    if (combatStyle->closeRangeData.flankVarianceMult != COMBAT_CLOSE_FLANK && COMBAT_CLOSE_FLANK != 1.0f)
        combatStyle->closeRangeData.flankVarianceMult = COMBAT_CLOSE_FLANK;
    if (combatStyle->closeRangeData.throwMaxTargets != COMBAT_CLOSE_THROW_GRENADE && COMBAT_CLOSE_THROW_GRENADE != 1.0f)
        combatStyle->closeRangeData.throwMaxTargets = COMBAT_CLOSE_THROW_GRENADE;
    // Cover
    if (combatStyle->coverData.coverSearchDistanceMult != COMBAT_COVER_DISTANCE && COMBAT_COVER_DISTANCE != 1.0f)
        combatStyle->coverData.coverSearchDistanceMult = COMBAT_COVER_DISTANCE;
    /* if (DEBUGGING) REX::INFO("SetCompanionCombatAI_Internal: Combat style for companion {} updated.", comp->GetDisplayFullName());
    if (DEBUGGING) REX::INFO(" - New Offensive Multiplier: {}", combatStyle->generalData.offensiveMult);
    if (DEBUGGING) REX::INFO(" - New Defensive Multiplier: {}", combatStyle->generalData.defensiveMult);
    if (DEBUGGING) REX::INFO(" - New Ranged Score Multiplier: {}", combatStyle->generalData.rangedScoreMult);
    if (DEBUGGING) REX::INFO(" - New Melee Score Multiplier: {}", combatStyle->generalData.meleeScoreMult);
    // Ranged
    if (DEBUGGING) REX::INFO(" - New Ranged Adjust Range Multiplier: {}", combatStyle->longRangeData.adjustRangeMult);
    if (DEBUGGING) REX::INFO(" - New Ranged Crouch Multiplier: {}", combatStyle->longRangeData.crouchMult);
    if (DEBUGGING) REX::INFO(" - New Ranged Strafe Multiplier: {}", combatStyle->longRangeData.strafeMult);
    if (DEBUGGING) REX::INFO(" - New Ranged Wait Multiplier: {}", combatStyle->longRangeData.waitMult);
    if (DEBUGGING) REX::INFO(" - New Ranged Accuracy Multiplier: {}", combatStyle->rangedData.accuracyMult);
    // Close-Quarters
    if (DEBUGGING) REX::INFO(" - New Close Fallback Multiplier: {}", combatStyle->closeRangeData.fallbackMult);
    if (DEBUGGING) REX::INFO(" - New Close Circle Multiplier: {}", combatStyle->closeRangeData.circleMult);
    if (DEBUGGING) REX::INFO(" - New Close Disengage Probability: {}", combatStyle->closeRangeData.disengageProbability);
    if (DEBUGGING) REX::INFO(" - New Close Flank Variance Multiplier: {}", combatStyle->closeRangeData.flankVarianceMult);
    if (DEBUGGING) REX::INFO(" - New Close Throw Max Targets: {}", combatStyle->closeRangeData.throwMaxTargets);
    // Cover
    if (DEBUGGING) REX::INFO(" - New Cover Search Distance Multiplier: {}", combatStyle->coverData.coverSearchDistanceMult); */
}

// Populate global arrays with current cell actors
std::int32_t UpdateGlobalActorArrays_Internal() {
    // Timestamp
    auto now = std::chrono::steady_clock::now();
    // Get all actors in cell
    auto actors = GetAllActors_Internal();
    // Collect enemies to calculate max health
    std::vector<RE::Actor*> tempEnemies;
    for (auto* actor : actors) {
        if (actor && !actor->IsDead(true) && IsActorEnemy_Internal(actor)) {
            tempEnemies.push_back(actor);
        }
    }
    // Calculate max enemy health for relative comparison
    float maxHealth = 0.0f;
    for (auto* enemy : tempEnemies) {
        if (!enemy)
            continue;
        float health = 0.0f;
        auto* healthAV = g_actorValueSingleton->health;
        if (healthAV) {
            health = enemy->GetPermanentActorValue(*healthAV);
        }
        if (health > maxHealth) {
            maxHealth = health;
        }
    }
    if (maxHealth > 0.0f) {
        g_enemyMaxHealthInCell = maxHealth;
    }
    // Create tracked data for each actor
    auto dataCompanionActors = std::vector<TrackedActorData>();
    auto dataEnemyActors = std::vector<TrackedActorData>();
    auto dataNeutralActors = std::vector<TrackedActorData>();
    // Categorize and store
    for (auto* actor : actors) {
        if (!actor || actor->IsDead(true))
            continue;
        // Skip player
        if (actor->IsPlayerRef())
            continue;
        // Skip excluded actors
        if (IsActorExcluded_Internal(actor))
            continue;
        // Companion
        if (IsActorActiveCompanion_Internal(actor)) {
            auto dataCompanionActor = CreateTrackedData_Internal(actor, ENEMY_TIER::LOW); // Companions are not enemies, tier is irrelevant
            // Compare with previous state for changes
            auto prevOpt = ActorTracking::GetPreviousCompanionData(actor);
            if (prevOpt) {
                // Stimpak usage and health/distance changes
                bool wasUsingStimpak = prevOpt->usesStimpak;
                // Check if the companion is using a stimpak
                if (actor->race && !actor->IsDead(false) && !g_raceStimpak.empty()) {
                    bool isStimpakRace = false;
                    for (auto* humanRace : g_raceStimpak) {
                        if (actor->race && actor->race->GetFormID() == humanRace->GetFormID()) {
                            isStimpakRace = true;
                            break;
                        }
                    }
                    if (isStimpakRace) {
                        dataCompanionActor.usesStimpak = true;
                        if (IsActorRaceSynth_Internal(actor)) {
                            dataCompanionActor.usesStimpak = false;
                        }
                    } else {
                        dataCompanionActor.usesStimpak = false;
                    }
                } else {
                    dataCompanionActor.usesStimpak = wasUsingStimpak;
                }
                // Check if the companion took damage
                float healthDelta = dataCompanionActor.healthPercent - prevOpt->healthPercent;
                // Update position
                dataCompanionActor.position = actor->GetPosition();
                // Update velocity
                float velocity = actor->GetPosition().GetDistance(prevOpt->position) / UPDATE_INTERVAL;
                dataCompanionActor.velocity = velocity;
                // Update stuck counter
                int stuckCounter = ActorTracking::GetActorStuckCounterFast(actor);
                dataCompanionActor.stuckCounter = stuckCounter;
                // Update lost status
                dataCompanionActor.lost = ActorTracking::GetActorLostStatusFast(actor);
            }
            dataCompanionActors.push_back(dataCompanionActor);
        } else if (IsActorEnemy_Internal(actor)) {
            // Enemy - analyze threat with CORRECT max health
            auto analysis = EnemyActorAnalyze_Internal(actor);
            auto dataEnemyActor = CreateTrackedData_Internal(actor, analysis.tier);
            dataEnemyActor.isRanged = analysis.isRanged;
            dataEnemyActor.isMelee = analysis.isMelee;
            dataEnemyActor.hasGrenades = analysis.hasGrenades;
            dataEnemyActors.push_back(dataEnemyActor);
        } else {
            // Neutral NPC
            auto dataNeutralActor = CreateTrackedData_Internal(actor, ENEMY_TIER::LOW);
            dataNeutralActors.push_back(dataNeutralActor);
        }
    }
    // Get a diff between current and previous state to apply RemoveAIAggression to the missing companions
    // Create a temporary tracked data to pass to RemoveAIAggression_Internal
    std::vector<TrackedActorData> missingCompanionData;
    auto companionData = ActorTracking::GetCompanionData();
    for (const auto& prevCompanion : companionData) {
        bool found = false;
        for (const auto& currCompanion : dataCompanionActors) {
            if (prevCompanion.actor == nullptr || currCompanion.actor == nullptr)
                continue;
            if (prevCompanion.actor == currCompanion.actor) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Companion is missing from current snapshot
            missingCompanionData.push_back(prevCompanion);
        }
    }
    // Remove AI aggression from missing companions
    RemoveAIAggression_Internal(missingCompanionData);
    // Cache current state
    ActorTracking::CacheCurrentState();
    // Clear old data
    ActorTracking::g_enemies.clear();
    ActorTracking::g_companions.clear();
    ActorTracking::g_neutralNPCs.clear();
    // Push new data
    ActorTracking::ReplaceCompanionData(dataCompanionActors);
    ActorTracking::ReplaceEnemyData(dataEnemyActors);
    ActorTracking::ReplaceNeutralNPCData(dataNeutralActors);
    // Synchronize companion flags with current snapshot
    ActorTracking::SyncCompanionFlagsWithSnapshot(dataCompanionActors);
    return actors.size();
}

// --- HOOKS ---

void ActorTracking::EnsureCompanionFlagEntry(RE::Actor* actor) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    g_companionFlags.try_emplace(actor, std::make_unique<ActorTracking::CompanionFlags>());
}

void ActorTracking::SyncCompanionFlagsWithSnapshot(const std::vector<TrackedActorData>& snapshot) {
    // Build a new set of current actors from snapshot (NO LOCK - it's local)
    std::unordered_map<RE::Actor*, std::unique_ptr<CompanionFlags>> newFlags;
    newFlags.reserve(snapshot.size() * 2);
    // Copy the existing data from the old map to the new map
    {
        std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
        for (const auto &d : snapshot) {
            if (!d.actor) continue;
            
            auto it = g_companionFlags.find(d.actor);
            if (it != g_companionFlags.end() && it->second) {
                // Move existing entry to preserve counter values
                newFlags[d.actor] = std::move(it->second);
            } else {
                // Create fresh entry
                newFlags[d.actor] = std::make_unique<CompanionFlags>();
            }
            
            // Update values in new map
            auto& flags = newFlags[d.actor];
            if (flags) { 
                flags->lastPosX = d.position.x;
                flags->lastPosY = d.position.y;
                flags->lastPosZ = d.position.z;
                flags->velocity = d.velocity;
                flags->stuckCounter = d.stuckCounter;
                flags->lost = d.lost;
            }
        }
    }
    // Replace old map with new map
    {
        std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
        g_companionFlags = std::move(newFlags);
    }
}

// Fast getters/setters (use atomic fields; small mutex only for map lookup/creation)
bool ActorTracking::GetActorStuckStatusFast(RE::Actor* actor) {
    if (!actor) return false;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        return it->second->stuck;
    }
    return false;
}
void ActorTracking::SetActorStuckStatusFast(RE::Actor* actor, bool stuck) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        it->second->stuck = stuck;
    }
    return;
}

int ActorTracking::GetActorStuckCounterFast(RE::Actor* actor) {
    if (!actor) return 0;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        return it->second->stuckCounter;
    }
    return 0;
}
void ActorTracking::IncrementActorStuckCounterFast(RE::Actor* actor) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        ++(it->second->stuckCounter);
    }
    return;
}
void ActorTracking::SetActorStuckCounterFast(RE::Actor* actor, int counter) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        it->second->stuckCounter = counter;
    }
    return;
}
float ActorTracking::GetActorVelocityFast(RE::Actor* actor) {
    if (!actor) return 0.0f;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        return it->second->velocity;
    }
    return 0.0f;
}
void ActorTracking::SetActorVelocityFast(RE::Actor* actor, float vel) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        it->second->velocity = vel;
    }
    return;
}

void ActorTracking::SetActorLastPositionFast(RE::Actor* actor, const RE::NiPoint3& pos) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        it->second->lastPosX = pos.x;
        it->second->lastPosY = pos.y;
        it->second->lastPosZ = pos.z;
    }
    return;
}
void ActorTracking::GetActorLastPositionFast(RE::Actor* actor, RE::NiPoint3& outPos) {
    outPos = RE::NiPoint3{0.0f, 0.0f, 0.0f};
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        outPos.x = it->second->lastPosX;
        outPos.y = it->second->lastPosY;
        outPos.z = it->second->lastPosZ;
    }
    return;
}
bool ActorTracking::GetActorLostStatusFast(RE::Actor* actor) {
    if (!actor) return false;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        return it->second->lost;
    }
    return false;
}
void ActorTracking::SetActorLostStatusFast(RE::Actor* actor, bool lost) {
    if (!actor) return;
    std::lock_guard<std::mutex> lk(g_companionFlagsMutex);
    auto it = g_companionFlags.find(actor);
    // Check if entry exists
    if (it != g_companionFlags.end()) {
        it->second->lost = lost;
    }
    return;
}

// Companion Movement task management
namespace MovementSystem {
std::mutex g_companionTasksMutex;
std::vector<CompanionTask> g_companionTasks;
void AddCompanionTask(RE::Actor* companion, float duration) {
    if (!companion || !companion->currentProcess || !companion->currentProcess->middleHigh)
        return;
    std::lock_guard<std::mutex> lock(g_companionTasksMutex);
    // Check if already in list
    for (auto& task : g_companionTasks) {
        if (task.companion == companion) {
            task.timeRemaining = duration; // Reset timer for the existing companion task
            return;
        }
    }
    // Add new Companion task
    g_companionTasks.push_back({companion, duration, 0.0f});
}
void ProcessCompanionTasks(float deltaTime) {
    std::lock_guard<std::mutex> lock(g_companionTasksMutex);
    for (auto it = g_companionTasks.begin(); it != g_companionTasks.end();) {
        // Validate companion
        if (!it->companion) {
            it = g_companionTasks.erase(it);
            continue;
        }
        if (it->companion->IsDeleted() || !it->companion->Get3D()) {
            // Manually remove stuck measures and erase task to increment iterator
            MovementSystem::RemoveStuckMeasures(it->companion);
            it = g_companionTasks.erase(it);
            continue;
        }
        // Check if a scene is running - skip processing during scenes
        if (IsActorInScene_Internal(it->companion)) {
            ++it;
            continue;
        }
        it->timeRemaining -= deltaTime;
        // Timer expired - check if we should remove the task
        if (it->timeRemaining <= 0.0f) {
            // Remove stuck measures if any
            MovementSystem::RemoveStuckMeasures(it->companion);
            // Remove companion and increment iterator
            it = g_companionTasks.erase(it);
        } else {
            // Check if companion is moving and stuck to apply measures if needed
            if (it->companion->currentProcess && it->companion->currentProcess->middleHigh) {
                // Checks for stuck status
                bool isStuck = false;
                // Get current velocity between main loop updates
                float velocity = ActorTracking::GetActorVelocityFast(it->companion);
                // Check if the AI is trying to move
                if (it->companion->currentProcess->middleHigh->desiredSpeed <= 0.0f) {
                    // Not trying to move - skip velocity stuck check
                    ActorTracking::SetActorStuckCounterFast(it->companion, 0);
                } else {
                    // Check velocity-based stuck (moving too slowly while not idling)
                    if (velocity < AI_STUCK_SPEED &&
                        it->companion->currentProcess->middleHigh->desiredSpeed > velocity &&
                        // Make dure the companion is not idling
                        !it->companion->currentProcess->middleHigh->currentIdle &&
                        !it->companion->currentProcess->middleHigh->furnitureIdle) {
                        // speed < threshold indicates little to no real movement
                        // While desiredSpeed > velocity indicates the companion is trying to move
                        // Could be stuck before an obstacle
                        isStuck = true;
                    } else if (velocity >= AI_STUCK_SPEED) {
                        // Moving normally - reset counter
                        ActorTracking::SetActorStuckCounterFast(it->companion, 0);
                    }
                    // Check collision-based stuck
                    // Someone or something runs into the companion, could be the player
                    // compCharCtrl->numCollisions > 0 indicates collisions with the player or other objects
                    auto* compCharCtrl = it->companion->currentProcess->middleHigh->charController.get();
                    if (compCharCtrl && compCharCtrl->numCollisions > AI_STUCK_COLLISIONS) {
                        isStuck = true;
                    }
                }
                // Apply progressive escalation based on stuck status
                if (isStuck) {
                    ActorTracking::SetActorStuckStatusFast(it->companion, true);
                    ActorTracking::IncrementActorStuckCounterFast(it->companion);
                    
                    // Get current stuck counter to determine which tier of measures to apply
                    int currentCount = ActorTracking::GetActorStuckCounterFast(it->companion);
                    
                    if (currentCount <= UPDATE_INTERVAL * 10) {
                        // Tier 1: Soft NavMesh adjustment (Z position bump)
                        // First full main loop cycle - try gentle measures
                        MovementSystem::ApplyStuckMeasures1(it->companion);
                    } else if (currentCount <= 2 * UPDATE_INTERVAL * 10) {
                        // Tier 2: Aggressive measures (disable bumper, fake support, step height)
                        // Second full main loop cycle - try more aggressive measures
                        MovementSystem::ApplyStuckMeasures2(it->companion);
                    } else {
                        // Tier 3: Totally stuck - set lost flag for main loop teleport
                        // After two full main loop cycles, give up and mark for teleport
                        ActorTracking::SetActorLostStatusFast(it->companion, true);
                    }
                } else {
                    // Not stuck - remove any applied measures
                    MovementSystem::RemoveStuckMeasures(it->companion);
                    // Clear stuck status
                    ActorTracking::SetActorStuckStatusFast(it->companion, false);
                    ActorTracking::SetActorStuckCounterFast(it->companion, 0);
                }
            }
            ++it;
        }
    }
}
void RemoveCompanionTask(RE::Actor* companion) {
    if (!companion)
        return;
    std::lock_guard<std::mutex> lock(g_companionTasksMutex);
    for (auto it = g_companionTasks.begin(); it != g_companionTasks.end(); ++it) {
        if (it->companion == companion) {
            // Disable companion movement measures if any
            MovementSystem::RemoveStuckMeasures(it->companion);
            // Clear stuck status
            ActorTracking::SetActorStuckStatusFast(it->companion, false);
            g_companionTasks.erase(it);
            return;
        }
    }
}
// NavMesh stuck measures (most often)
void ApplyStuckMeasures1(RE::Actor* companion) {
    if (!companion)
        return;
    for (auto& task : g_companionTasks) {
        if (task.companion == companion) {
            // Apply stuck measures immediately
            if (task.companion && task.companion->currentProcess && task.companion->currentProcess->middleHigh) {
                // Move the companion upwards slightly to help get unstuck
                // Get current position
                RE::NiPoint3 currentPosition = task.companion->GetPosition();
                // Move up by 0.5 units to pick up the new navmesh and force a re-evaluation
                // This should be barely visible to the player
                currentPosition.z += 0.1f;
                // Teleport and true to update CharController position as well
                task.companion->SetPosition(currentPosition, true);
                // Update the character 3D position
                //task.companion->UpdateActor3DPosition();
            }
            return;
        }
    }
}
// Velocity/collision stuck measures (i.e. running against an object)
void ApplyStuckMeasures2(RE::Actor* companion) {
    if (!companion)
        return;
    for (auto& task : g_companionTasks) {
        if (task.companion == companion) {
            // Apply stuck measures immediately
            if (task.companion && task.companion->currentProcess && task.companion->currentProcess->middleHigh) {
                auto* compCharCtrl = task.companion->currentProcess->middleHigh->charController.get();
                if (compCharCtrl) {
                    // Reset collisions to 0
                    compCharCtrl->numCollisions = 0;
                    // Disable bumper
                    compCharCtrl->useBumper = false;
                    // Enable fake support
                    compCharCtrl->fakeSupport = true;
                    // Increase step height
                    compCharCtrl->stepHeightMod = 2.0f; // Increase step height to help overcome obstacles
                    // Update the character 3D position
                    //task.companion->UpdateActor3DPosition();
                }
            }
        }
    }
}
void RemoveStuckMeasures(RE::Actor* companion) {
    if (!companion)
        return;
    for (auto& task : g_companionTasks) {
        if (task.companion == companion) {
            // Remove stuck measures
            if (task.companion && task.companion->currentProcess && task.companion->currentProcess->middleHigh) {
                auto* compCharCtrl = task.companion->currentProcess->middleHigh->charController.get();
                if (compCharCtrl) {
                    // Reset collisions to 0
                    compCharCtrl->numCollisions = 0;
                    // Enable bumper
                    compCharCtrl->useBumper = true;
                    // Disable fake support
                    compCharCtrl->fakeSupport = false;
                    // Restore step height
                    compCharCtrl->stepHeightMod = 0.0f; // Reset step height to default
                    // Update the character 3D position
                    //task.companion->UpdateActor3DPosition();
                }
            }
            return;
        }
    }
}
} // namespace MovementSystem

// --- PAPYRUS ---

// Finally register Papyrus functions
bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    if (DEBUGGING)
        REX::INFO("RegisterPapyrusFunctions: Attempting to register Papyrus functions. VM pointer: {}", static_cast<const void*>(vm));
    // vm->BindNativeMethod("<Name of the script binding the function>", "<Name of the function in Papyrus>", <Name of
    // the function in F4SE>, <can run parallel to Papyrus>);
    if (DEBUGGING)
        REX::INFO("RegisterPapyrusFunctions: All Papyrus functions registration attempts completed.");
    return true;
}
