# Half-Life: Absolute Zero — Android / Xash3D FWGS

Código do mod **Half-Life: Absolute Zero** integrado à infraestrutura do
[hlsdk-portable](https://github.com/FWGS/hlsdk-portable) (FWGS) e ao
[FreeVGUI](https://github.com/FWGS/FreeVGUI), para rodar no
[Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs) em Android (e outras plataformas não-x86).

O gameplay vem do código original do mod (pastas `dlls/`, `cl_dll/`, `game_shared/`, `pm_shared/`, `common/`,
`engine/`, `config/`). Do hlsdk-portable vêm o sistema de build (CMake), o esquema de nomes de bibliotecas do
Xash3D, a API mobile e o FreeVGUI (VGUI1 de código aberto: placar, menus de time, painel de espectador).

> A pasta `cl_dlls` do mod original passou a se chamar `cl_dll` (padrão do hlsdk-portable). O diretório de instalação
> dentro do gamedir continua `cl_dlls` (`CLIENT_INSTALL_DIR` em `mod_options.txt`). O `_docs/AZ_readme.txt` é o readme antigo.

## Saída do build

| Plataforma | Server | Client |
|---|---|---|
| Android arm64 | `libaz_android_arm64.so` | `libclient_android_arm64.so` |
| Android 32 bits | `libaz_android_armv7l.so` | `libclient_android_armv7l.so` |

Os nomes são gerados por `cmake/LibraryNaming.cmake` (padrão do Xash3D). O `az` vem de `SERVER_LIBRARY_NAME`
em `mod_options.txt` e é o nome do `gamedll` do mod. As libs ficam em `lib/<abi>/` do APK (o Xash3D carrega por nome).

## Como compilar

### Pelo GitHub (recomendado)
O workflow `.github/workflows/android.yml` roda a cada push e gera os artefatos:

- **AbsoluteZero-apk** — APK do launcher já com as libs dentro (instale este).
- **az-android-libs** — só as libs (`arm64-v8a/` e `armeabi-v7a/`), sem símbolos.
- **az-libs-unstripped-arm64-v8a / -armeabi-v7a** — libs com símbolos, só para depuração (`addr2line`).
- **build-log** — só se o build falhar (o resumo dos erros também aparece na página da execução).

### Na mão (Linux / Termux / NDK)
```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static
cmake --build build
# resultado em build/out/arm64-v8a/
```
Em outras plataformas o CMake gera server e client normalmente (`cmake -S . -B build && cmake --build build`).
**waf não é suportado** neste fork: o código legado não passa nas flags `-Werror` do waf.

## Opções (`mod_options.txt`)

| Opção | Padrão | O que faz |
|---|---|---|
| `AZ_NO_VGUI` | OFF | Compila o client sem VGUI (placar, menus de time e painel de espectador ficam desligados) |
| `AZ_CRASH_HANDLER` | ON | Grava `az_crash.log` (pilha de chamadas) quando as libs crasham, e no logcat (tag `AZ_CRASH`) |
| `AZ_NO_STRIP` | OFF | Não remove símbolos em Release (o CI usa para gerar os artefatos de depuração) |
| `CLIENT_WEAPONS` | ON | Predição de armas no client |

## Launcher

`launcher/` é o app Android "Absolute Zero" (projeto Gradle). Ele só inicia o Xash3D FWGS com o gamedir
`AbsoluteZero`; as libs do mod vão dentro do APK (`launcher/app/src/main/jniLibs/<abi>/`, copiadas pelo workflow).
O pacote do app é `su.xash.az` e o gamedir fica em `launcher/app/src/main/java/su/xash/az/MainActivity.java`.

## Diferenças em relação ao código original do mod

- **Compat Windows → POSIX** (`public/az_compat.h`, incluído em todos os arquivos): `itoa`, `strupr`, `min/max`,
  `__declspec`, `BOOLEAN`, `_rotr`...
- **`-fsigned-char`**: o código assume `char` com sinal (a tabela de rotas dos NPCs dependia disso).
- **64 bits**: `MAKE_STRING`, save/restore de ponteiros de função e de arrays de ponteiros, `mstudioseqgroup_t`,
  versão do grafo de nós (`.nod`) por tamanho de ponteiro, lista temporária `m_pLink`.
- **Server browser removido** (`hud_servers.cpp` usa winsock; no Windows continua sendo compilado).
- **Entrada em POSIX** (`cl_dll/input_unix.cpp`): mouse via engine; toque e gamepad ficam com o engine.
- **Crash handler** (`public/az_crash_handler.cpp`).

## Créditos e licenças

- Half-Life: Absolute Zero — autor(es) do mod (código derivado do Half-Life SDK; alterações marcadas como `MODDD` no código). Veja `_docs/`.
- [hlsdk-portable](https://github.com/FWGS/hlsdk-portable) — FWGS. README original em `_docs/hlsdk-portable-README.md`.
- [FreeVGUI](https://github.com/FWGS/FreeVGUI) — Nagist, Valve, Alibek Omarov (licença em `freevgui/LICENSE`).
- Half-Life SDK — Valve Corp. Veja `LICENSE` (distribuição apenas gratuita).
- Port Android e launcher: Kali Kylee.
