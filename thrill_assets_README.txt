ThrillEX Android port (work in progress)

Gamedir: thrill (mod_options.txt). Based on FWGS/hlsdk-portable.
Reconstructed from IDA decompile of the original DLLs; spots that could not be
verified are marked with TODO(verify) in the code.

Done (compiles in syntax check):
  - skill cvars (sk_archer_*, sk_diablo_*, ...) in skill.h / game.cpp / gamerules.cpp
  - triggers.cpp: env_size, teledeath, target_command/trigger_command, trigger_shower,
    trigger_suitupdate, trigger_pardon, target_killclient, trigger_speed, test_grenade, func_glass
  - items.cpp: item_adrenaline, item_rad/item_radiation, item_shield; player fields added
  - ammo_9mmARclip, ammo_buckshot_small
  - barney.cpp: monster_barney_evil, monster_barney_traitor
  - m44.cpp: monster_m44
  - archer.cpp: monster_archer / monster_archerfish (unverified draft)
Pending: Fly, Friendly(+spit), Hassault, Diablo, Kingpin, Stealth, StukaBat(+bomb), SnapBug,
  XenSphere/XenLaser, Panther, SnarkNest, CineApache, weapon_chub, weapon_flare, client HUD.
