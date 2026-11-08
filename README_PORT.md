# HL: Invasion -> Xash3D FWGS (Android) — branch `invasion`

Código do mod (Uncle Roy / HL: Invasion) em cima do **hlsdk-portable** do FWGS,
usando o **FreeVGUI** (submódulo `freevgui`) em vez do `vgui.so` x86.

## O que mudou em relação ao hlsdk-portable (master)

- `dlls/` e `cl_dll/`: código do Invasion. Os `CMakeLists.txt` e `wscript` usam a
  lista de arquivos do Invasion (o master compila "tudo que tiver na pasta").
- `game_shared/` e `pm_shared/`: versões do Invasion (+ os arquivos do master).
- `common/const.h`, `common/hltv.h`: ajustes do Invasion (ELECTRO_DISK_*, DRC_CMD_FADE/SHAKE).
- `cl_dll/cdll_int.cpp`: referência a `InitVGUISupportAPI` (o engine procura esse
  símbolo na lib do cliente quando o FreeVGUI está embutido).
- `dlls/inv_crash.cpp` + `dlls/inv_log.h`: sistema de logs/crash (veja `LOGS_INVASION.md`).
- `mod_options.txt`: `GAMEDIR=invasion`, `SERVER_LIBRARY_NAME=invasion`.
- `android/`: launcher do Invasion (pacote `su.xash.invasion`).
- `.github/workflows/android.yml`: gera as libs (arm64 e armv7) e o APK.
  Os workflows originais do master (build.yml / manual.yml) foram removidos.

## Dependências

- `freevgui/` — submódulo (https://github.com/FWGS/freevgui), commit testado: `73bfb4659f3d9eb28b79e5b26baea8c6b888d5eb`
- `miniaudio/` — pasta com https://github.com/mackron/miniaudio (música do cliente),
  commit testado: `4d813cfe23c28db165cce6785419fee9d2399766`.
  Se a pasta não existir no repositório, o workflow baixa sozinho.

## Build

CMake (Android, libs): ver `.github/workflows/android.yml`.
waf (PC): `./waf configure -T release --64bits && ./waf build` (os warnings não são fatais).
