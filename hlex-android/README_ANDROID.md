# Half-Life: Extended - port Android (Xash3D FWGS)

Base: codigo-fonte do HL:E (halflife-extended-old) + infraestrutura do hlsdk-portable (FWGS).
Port: Kali Kyle.

## Como compilar (GitHub Actions)
1. Suba este repositorio no GitHub.
2. Aba Actions > "Build Android libs" > Run workflow (ou faca um push na branch main/master).
3. Baixe os artifacts: hlex-android-arm64-v8a e hlex-android-armeabi-v7a.
   Cada um tem a lib do servidor (hlex) e a do cliente (client), com o nome no padrao do Xash3D (ex.: libhlex_android_arm64.so).
4. Coloque as libs no APK (ou em hlex_base/dlls e hlex_base/cl_dlls, conforme o seu launcher).

O workflow baixa o FreeVGUI sozinho (FWGS/freevgui) se a pasta freevgui estiver vazia.

## Opcao experimental
Run workflow > studio_renderer = true liga o renderizador de modelos proprio do HL:E
(sistema fullbright dos modelos _light.mdl). Por padrao fica DESLIGADO no Android,
porque ele mexe em estruturas internas do motor.

## Assets
Use a pasta hlex_base do hlex-SDK-1.1.3 (renomeie a pasta para hl_extended) (mapas, modelos, sons).
No liblist.gam / gameinfo.txt: gamedll "dlls/hlex.dll" (mesmo nome de SERVER_LIBRARY_NAME no mod_options.txt) e cldll "1".
