#include <PCH.h>
#include <Global.h>

const char* defaultIni = R"(
; Companion Control Booster Configuration File
;------------------------------------------------------------------------
; This file contains configuration settings for the Companion Control Booster mod.
; The settings are applied to all current companions travelling with the player.

; Enable/disable debugging messages.
DEBUGGING=false
; Global Update Interval in seconds.
UPDATE_INTERVAL=3.0
; Read the ini every x updates (0 = only on game start).
; This can help with tweaking settings without restarting the game.
INI_RELOAD_INTERVAL=0
; Actor search radius around the player in game units.
ACTOR_SEARCH_RADIUS=4000.0

; --- AI Settings ---
; Configure AI settings
AI_HEALTH_THRESHOLD=40.0           ; Health percentage threshold to react (0.0 - 100.0)
AI_USE_STIMPAK=true                ; Allow companions to use stimpaks or repair kits
AI_USE_STIMPAK_UNLIMITED=false     ; Allow companions to use unlimited stimpaks or repair kits without consuming them (contradicts fleeing behavior)
AI_AUTO_REVIVE=true                ; Allow companions to auto-revive when downed (also in HC mode)
AI_FLEE_COMBAT=true                ; Allow companions to flee combat
AI_FLEE_DISTANCE=500.0             ; Maximum distance to flee
; Items
AI_EQUIP_ITEMS=true                ; Enable armor, weapon and ammo equipping for companions
AI_EQUIP_GEAR=false                ; Allow companions to equip the best armor and weapons from inventory (be careful in conjunction with looting)
AI_EQUIP_AMMO_REFILL=true          ; Supply ammunition to the companion for the equipped weapon
AI_EQUIP_AMMO_AMOUNT=50            ; Minimum Amount of ammunition companions always have for their equipped weapon
; Movement
; A fast update loop runs at 10hz to monitor companion movement.
AI_STUCK_CHECK=true                ; Enable stuck check for companions
AI_STUCK_THRESHOLD=59              ; How many fast updates stuck to consider a companion lost and teleport (59 fast update loops at 10hz with 3s main loop = 6 seconds)
AI_STUCK_COLLISIONS=2              ; Number of collisions to consider a companion stuck (apply blocked measures)
AI_STUCK_SPEED=10.0                ; Speed threshold to consider a companion stuck when pathing (units/second).
AI_STUCK_DISTANCE=1500.0           ; Distance threshold to consider a companion lost and teleport (too far away from the player)
; Aggression
; This enables companions to engage in combat on their own and not wait for the enemy to shoot at them or the player.
AI_AGGRESSION_ENABLE=true          ; Enable AI aggression settings on standard follow AI package
AI_AGGRESSION_ALL=true             ; Enable AI aggression on all AI packages
AI_AGGRESSION_SNEAK=false          ; Enable AI aggression when sneaking (overrides AI_AGGRESSION_ALL)
AI_AGGRESSION_RADIUS0=1600.0       ; Distance when companions could detect nearby enemies
AI_AGGRESSION_RADIUS1=1200.0       ; Distance when companions could attack nearby enemies
AI_AGGRESSION_RADIUS2=800.0        ; Distance when companions Attack on sight

; --- Chatter Settings ---
; Companion idle chatter multiplier.
CHATTER_ENABLED=true               ; Enable companion idle chatter adjustment
CHATTER_MULTIPLIER=1.0             ; 0.5 = Very frequent, 1.0 = Default, 2.0 = Half as often, 5.0 = Very quiet
CHATTER_MULTIPLIER_SNEAK=5.0       ; 0.5 = Very frequent, 1.0 = Default, 2.0 = Half as often, 5.0 = Very quiet

; --- Combat AI Settings ---
; These settings are good for an allround balanced combat behavior.
; Adjust the values to tweak companion combat behavior.
; Setting 1.0 means no change to the actors default behavior.
COMBAT_ENABLED=true                ; Enable companion combat settings
COMBAT_TARGET=1                    ; Target selection: 0=closest, 1=lowest_threat, 2=highest_threat
COMBAT_OFFENSIVE=0.6               ; Offensive combat multiplier (0.0 = passive, 0.9 = aggressive) (1.0 = no change)
COMBAT_DEFENSIVE=0.6               ; Defensive combat multiplier (0.0 = reckless, 0.9 = cautious) (1.0 = no change)
COMBAT_RANGED=1.0                  ; Prefered ranged combat (0.0 = none, 0.9 = standard ranged combat) (1.0 = no change)
COMBAT_MELEE=1.0                   ; Prefered melee combat (0.0 = none, 0.9 = standard melee combat) (1.0 = no change)
; Ranged
COMBAT_RANGED_ADJUSTMENT=0.5       ; Chance to adjust the weapon (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_RANGED_CROUCHING=0.8        ; Chance to crouch (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_RANGED_STRAFE=0.5           ; Chance to strafe (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_RANGED_WAITING=0.5          ; Chance to wait between attacks (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_RANGED_ACCURACY=0.4         ; Shooting accuracy (0.0 = poor, 0.9 = perfect) (1.0 = no change)
; Close-Quarters
COMBAT_CLOSE_FALLBACK=0.5          ; Chance to fall back (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_CLOSE_CIRCLE=0.5            ; Chance to circle target (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_CLOSE_DISENGAGE=0.5         ; Chance to disengage (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_CLOSE_FLANK=0.5             ; Chance to flank target (0.0 = never, 0.9 = always) (1.0 = no change)
COMBAT_CLOSE_THROW_GRENADE=3       ; Minimum targets to consider throwing a grenade
; Cover
COMBAT_COVER_DISTANCE=0.5          ; Distance to search for cover (0.0 = close, 0.9 = far) (1.0 = no change)

; --- Companion Looting Settings ---
; Enable looting (hoovering) by companions.
; This is limited to unlocked containers and corpses only.
; Companions will not loot in settlements.
; Quest items, if flagged, are excluded automatically.
; Companions will stop looting when they reach their carry weight limit by default.
LOOT_ENABLED=true                  ; Enable companion looting
LOOT_COMBAT=false                  ; Loot when in combat
LOOT_RADIUS=600.0                  ; Radius to search for lootable items around the companion, containers and corpses
LOOT_JUNK=true                     ; Loot junk items
LOOT_AMMO=true                     ; Loot ammunition
LOOT_AID=true                      ; Loot aid items
LOOT_MIN_VALUE=0                   ; Loot armor/weapons above this value
LOOT_MAX_VALUE=9999                ; Loot armor/weapons under this value
LOOT_STEAL=false                   ; Commit a crime when looting items
LOOT_WEIGHT_LIMIT=true             ; Companions stop looting when they reach their carry weight limit

; --- Companion XP Settings ---
; Enable XP gain for companion kills
XP_ENABLED=true                    ; Enable XP gain from companion kills
XP_RATIO=0.5                       ; Ratio for XP gain (0.0 = none, 1.0 = full XP)
XP_KILLER_TOLERANCE=3000.0         ; Max distance from the player to the victim to award XP

; --- Companion Power Armor Settings ---
; Enable power armor repair for companions.
PA_ENABLED=true                    ; Enable power armor repair for companions
PA_REPAIR_AMOUNT=0.01              ; Amount of power armor condition repaired every update (0.0 - 1.0)

; --- The settings below will modify companion stats permanently ---

; --- Companion Buff Settings ---
; Enable buff for companions.
; Those floor values are not overpowered and companions will outgrow them later in the game.
BUFF_ENABLED=true                  ; Enable companion buffs
BUFF_HEAL_RATE=5.0                 ; Minimum heal rate (basic NPC have 1.0)
BUFF_COMBAT_HEAL_RATE=5.0          ; Minimum Heal rate in combat (basic NPC have 0.0)
BUFF_DAMAGE_RESIST=200.0           ; Minimum physical damage absorbed (basic NPC have 0.0)
BUFF_FIRE_RESIST=50.0              ; Minimum fire damage absorbed (basic NPC have 0.0)
BUFF_ELECTRICAL_RESIST=50.0        ; Minimum electrical damage absorbed (basic NPC have 0.0)
BUFF_FROST_RESIST=50.0             ; Minimum cold damage absorbed (basic NPC have 0.0)
BUFF_ENERGY_RESIST=50.0            ; Minimum energy damage absorbed (basic NPC have 0.0)
BUFF_POISON_RESIST=50.0            ; Minimum poison damage absorbed (basic NPC have 0.0)
BUFF_RADIATION_RESIST=50.0         ; Minimum radiation damage absorbed (basic NPC have 0.0)
BUFF_AGILITY=5.0                   ; Minimum agility (basic NPC have 0.0)
BUFF_ENDURANCE=5.0                 ; Minimum endurance (basic NPC have 0.0)
BUFF_INTELLIGENCE=5.0              ; Minimum intelligence (basic NPC have 0.0)
BUFF_LOCKPICK=30.0                 ; Minimum lockpick (Locksmith04 = 50.0) (basic NPC have 0.0)
BUFF_LUCK=5.0                      ; Minimum luck (basic NPC have 0.0)
BUFF_PERCEPTION=5.0                ; Minimum perception (basic NPC have 0.0)
BUFF_SNEAK=30.0                    ; Minimum sneak (Sneak04 = 50.0) (basic NPC have 0.0)
BUFF_STRENGTH=5.0                  ; Minimum strength (basic NPC have 0.0)
BUFF_CARRYWEIGHT=1000.0            ; Minimum carry weight (basic NPC have 150.0)

; --- Companion Perk Settings ---
; Enable perk assignment and list of perk IDs to apply to companions.
; These perks will be stored in the save game.
; Some examples are commented by default to not make companions overpowered too early.
; Base perks to enable basic features.
PERK_ENABLED=true                  ; Enable companion perk assignment
PERK_TO_APPLY=0002A6FC             ; crNoFallDamage "crNoFallDamage" [PERK:0002A6FC] -> no fall damage
PERK_TO_APPLY=000BA440             ; ModDetectionMovement [PERK:000BA440] -> needs to be added to companions to use the BUFF_SNEAK buff properly
PERK_TO_APPLY=00245BE8             ; mod_armor_StealthMovePerk [PERK:00245BE8] -> 5% improved sneak movement speed
PERK_TO_APPLY=000FA292             ; PlayerBaseLockpick "Locksmith" [PERK:000FA292] -> Lockpick gate
; Perks to consider enabling
;PERK_TO_APPLY=001D97D5             ; CompanionPreventLimbDamage [PERK:001D97D5] -> no limb damage
;PERK_TO_APPLY=000523FF             ; Locksmith01 "Locksmith" [PERK:000523FF] -> needs the keyword to work
;PERK_TO_APPLY=00052400             ; Locksmith03 "Locksmith" [PERK:00052400] -> needs the keyword to work
;PERK_TO_APPLY=00052401             ; Locksmith03 "Locksmith" [PERK:00052401] -> needs the keyword to work
;PERK_TO_APPLY=001D246A             ; Locksmith04 "Locksmith" [PERK:001D246A] -> needs the keyword to work
;PERK_TO_APPLY=00052403             ; Hacker01 "Hacker" [PERK:00052403] -> needs the keyword to work
;PERK_TO_APPLY=00052404             ; Hacker02 "Hacker" [PERK:00052404] -> needs the keyword to work
;PERK_TO_APPLY=00052405             ; Hacker03 "Hacker" [PERK:00052405] -> needs the keyword to work
;PERK_TO_APPLY=001D245D             ; Hacker04 "Hacker" [PERK:001D245D] -> needs the keyword to work
;PERK_TO_APPLY=0004C935             ; Sneak01 "Sneak" [PERK:0004C935] -> needs ModDetectionMovement perk to work
;PERK_TO_APPLY=000B9882             ; Sneak02 "Sneak" [PERK:000B9882] -> not trigger floor based traps
;PERK_TO_APPLY=000B9883             ; Sneak03 "Sneak" [PERK:000B9883] -> not trigger enemy mines
;PERK_TO_APPLY=000B9884             ; Sneak04 "Sneak" [PERK:000B9884] -> faster sneak movement
;PERK_TO_APPLY=000B9881             ; Sneak05 "Sneak" [PERK:000B9881] -> enemies lose you when entering sneak
; Can be expanded here with more perks as needed

; --- Companion Keyword Settings ---
; List of Keywords to add to companions
KEYWORD_ENABLED=true               ; Enable companion keyword assignment
KEYWORD_TO_APPLY=001760E4          ; Followers_Command_HackTerminal_Allowed [KYWD:001760E4] -> allows hacking terminals
KEYWORD_TO_APPLY=000F4B91          ; Followers_Command_LockPick_Allowed [KYWD:000F4B91] -> allows lockpicking
; Can be expanded here with more keywords as needed

; --- Other settings ---

; --- Actor exclusion settings ---
; List of Actor base or reference FormIDs to exclude from mod as an example.
; This can be the base FormID or the reference FormID of a specific instance, or both.
; This is useful to exclude specific companions that may not work well with the mod.
EXCLUDE_ACTOR_ID_LIST=000179FF     ; 0001A7D4 Codsworth [ACTOR:000179FF]
EXCLUDE_ACTOR_ID_LIST=0001CA7D     ; 0001A7D4 Codsworth [ACTOR:0001A7D4]
EXCLUDE_ACTOR_ID_LIST=000865D1     ; 0001A7D5 Curie as Miss Nanny [ACTOR:0001A7D5]
EXCLUDE_ACTOR_ID_LIST=00027686     ; 00027686 Curie as Miss Nanny [ACTOR:00027686]
EXCLUDE_ACTOR_ID_LIST=0001D15C     ; 0001D15C Dogmeat [ACTOR:0001D15C]
EXCLUDE_ACTOR_ID_LIST=0001D162     ; 0001D162 Dogmeat [ACTOR:0001D162]
; Can be expanded here with more Actor IDs as needed

; --- Threat-Level Settings ---
; These are multipliers for calculating threat levels of nearby enemies.
; 0 = disabled, 1 = normal, 2 = double, etc.
THREAT_WEAPON_BONUS=1.0            ; Weapon bonus
THREAT_LEGENDARY_BONUS=1.0         ; legendary enemy bonus
THREAT_UNIQUE_BONUS=1.0            ; unique enemy bonus
THREAT_HEALTH_BONUS=1.0            ; current health bonus
THREAT_ALERT_BONUS=1.0             ; alert status bonus

;------------------------------------------------------------------------
; DATA used by the mod - do not edit below this line
;------------------------------------------------------------------------
; Companion Faction ID
; CurrentCompanionFaction[FACT:00023C01]
CURRENT_COMPANION_FACTION_ID=00023C01
; ActorValue for HC downed state
ACTORVALUE_HC_DOWNED_ID=00249F6D   ; 00249F6D HC_IsCompanionInNeedOfHealing
; Forms used by the mod
ITEM_STIMPAK_ID=00023736           ; 00023736 Stimpak
ITEM_REPAIRKIT_ID=00004f12         ; 00004f12 Repair Kit needs DLCRobot.esm
; Races who use stimpaks
RACE_STIMPAK_ID=00013746           ; 00013746 HumanRace
RACE_STIMPAK_ID=000EAFB6           ; 000EAFB6 GhoulRace
; Synth3 component
RACE_SYNTH3C_ID=000CFF74           ; 000CFF74 Synth3 component
; IDLE Animations
IDLE_STIMPAK_ID=000B1CF9           ; 000B1CF9 3rdPUseStimpakOnSelf
; Keywords
KYWD_ArmorTypePower_ID=0004D8A1    ; 0004D8A1 ArmorTypePower
KYWD_IsPowerArmorFrame_ID=0015503F ; 0015503F isPowerArmorFrame
; Packages
PACK_FollowersCompanion_ID=0002A101 ; 0002A101 FollowersCompanion
)";