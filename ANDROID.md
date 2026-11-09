# Half-Life: Induction (Featureful SDK) para Android / Xash3D

Este repositório é o Featureful SDK com `GAMEDIR=induction`. Só o Xash3D FWGS
é necessário para rodar.

## Compilar
Push no GitHub e o workflow `.github/workflows/android.yml` gera dois artefatos:

- `induction-android-arm64-v8a`  -> `lib/arm64-v8a/libclient.so` + `libserver.so`
- `induction-android-armeabi-v7a` -> `lib/armeabi-v7a/libclient.so` + `libserver.so`

Localmente (precisa do NDK r25c, qualquer outra versão não é aceita pelo xcompile.py):

    export ANDROID_NDK_HOME=/caminho/android-ndk-r25c
    python3 ./waf configure -T release --android=aarch64,clang,21 --disable-goldsrc-support --disable-werror
    python3 ./waf build
    python3 ./waf install --destdir=dist

## Montar o APK
Use o APK do Xash3D FWGS (xash3d-fwgs android) e coloque:

- libs do Induction: `lib/<abi>/libclient.so` e `libserver.so` no APK (ou na
  pasta de libs do jogo, conforme o launcher que você usa);
- conteúdo do mod (baixado do Induction 1.4): pasta `induction/` com maps,
  models, sound, sprites, `liblist.gam` etc.;
- `induction/features/` e `halflife_featureful.fgd` (já saem no artefato).

O jogo precisa do `valve/` original do Half-Life junto.

## Limitações conhecidas
- Neblina (fog) do Featureful: `gl_dynamic.h` desliga o código de fog no
  Android (`!__ANDROID__`), então fog definido nos mapas não aparece.
- VGUI desligada: placar/MOTD usam as versões sem VGUI, o que é o normal no Xash mobile.

## Mudanças em relação ao zip original
- `dlls/wscript`: biblioteca do servidor chama `server` no Android
  (antes sairia `libhl.so`) e inclui `game_shared/parsetext.cpp` e `tex_materials.c`
  (faltavam, o link falhava).
- `cl_dll/wscript`: inclui `dlls/weapons_shared.cpp` e ignora o `cl_dll/parsetext.cpp`
  antigo (duplicava símbolos com o de `game_shared`).
- Novo `.github/workflows/android.yml`.
- O `Android.mk` antigo ficou sem uso (está desatualizado); o build é pelo waf.

## APK do launcher "Induction" (versao 1.4)
O workflow `.github/workflows/apk.yml` gera o APK completo (launcher + libs) e roda
sozinho a cada push na branch `main` (ou manualmente em Actions > Build Induction (APK)).

- Projeto Android em `android/` (Gradle + CMake do proprio repositorio).
- Pacote: `su.xash.induction`, nome "Induction", versao 1.4, gamedir `induction`.
- As libs saem com os nomes que o Xash3D FWGS procura
  (`libhl_android_arm64.so`, `libclient_android_arm64.so`, e `_armv7l` no 32 bits),
  entao nao precisa renomear nada na mao.
- ABIs: arm64-v8a e armeabi-v7a (edite `abiFilters` em `android/app/build.gradle` para mudar).
- O APK precisa do app Xash3D FWGS instalado e da pasta `induction/` do mod em `/sdcard/xash/`.
- O workflow antigo (`android.yml`, waf) ficou so manual; serve apenas para gerar as .so soltas.

### Mudancas de CMake para o APK
- `cmake/LibraryNaming.cmake`: o Android deixou de forcar o sufixo vazio.
- `dlls/CMakeLists.txt` e `cl_dll/CMakeLists.txt`: o nome de saida (`hl`, `client`) vale tambem no Android.

## VGUI via FreeVGUI (como no hlsdk-portable master)
- `freevgui/` e um submodulo git (https://github.com/FWGS/freevgui.git), fixado no commit
  `73bfb4659f3d9eb28b79e5b26baea8c6b888d5eb`. Depois de clonar: `git submodule update --init --recursive`.
- O APK compila com `-DUSE_VGUI=ON`: o FreeVGUI e ligado estaticamente ao `libclient_*.so`
  e o Xash3D usa o `InitVGUISupportAPI` exportado pelo cliente. Nao precisa de `vgui.so` separado.
- Headers originais do VGUI1 em `utils/vgui/include`; `engine/vgui_api.h`, `cursor_type.h` e
  `key_modifiers.h` vieram do hlsdk-portable master (o FreeVGUI precisa deles).
- Esta integracao so existe no CMake (o caminho do APK). O build antigo por waf nao tem o FreeVGUI.
- Builds com `GOLDSOURCE_SUPPORT=ON` (x86 PC) continuam usando o `vgui_support` do Featureful.
