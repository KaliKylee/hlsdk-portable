# O que foi alterado em relacao ao codigo original do HL:E

## Build
- CMake (hlsdk-portable) com listas de fontes baseadas no hldll.vcxproj / hl_cdll.vcxproj do HL:E.
- Workflow: .github/workflows/android.yml (NDK, arm64-v8a e armeabi-v7a).
- mod_options.txt: GAMEDIR=hl_extended, SERVER_LIBRARY_NAME=hlex, CLIENT_WEAPONS=ON.

## Servidor (dlls)
- util.h: MAKE_STRING seguro para 64 bits (usa ALLOC_STRING quando a diferenca nao cabe em 32 bits).
- nodes.h: versao do grafo de nos diferente em 64 bits (arquivos .nod/.nrp antigos sao regenerados).
- Save/Restore (util.cpp + saverestore.h + eiface.h + cbase.h): trocado pela versao 64 bits do hlsdk-portable
  (ponteiros de funcao como void*, tamanhos de entrada, endianness).
- util.cpp: SET_MODEL/PRECACHE_MODEL nao usam mais std::filesystem com caminho relativo
  (no Android isso faria TODOS os modelos virarem error.mdl). Agora usam o sistema de arquivos do motor.
- ctf/*.h: qualificacoes extras de classe removidas (erro no clang).
- C++17.

## Cliente (cl_dll)
- Input do Xash3D: input_xash3d.cpp + input_mouse.cpp (do hlsdk-portable); inputw32.cpp removido.
- HUD_MobilityInterface exportado (touch/vibracao do Xash3D).
- Discord rich presence: funcoes viraram no-op (Discord SDK removido).
- fog.cpp: sem OpenGL direto; usa somente pTriAPI->Fog (API de fog do motor).
- in_camera.cpp: sem SDL2.
- HUD_GetStudioModelInterface retorna 0 (renderizador do motor) a menos que HLEX_STUDIO_RENDERER=ON.
- common/cvardef.h: flags FCVAR_* extras do Xash3D.
- utils/vgui/include: headers do hlsdk-portable (compativeis com FreeVGUI).

## Verificado
- Servidor: 184 arquivos compilam sem erros (g++ 13, modo estrito, Linux x86_64); libhlex linka sem simbolos indefinidos
  e exporta GiveFnptrsToDll, GetEntityAPI, GetEntityAPI2 e GetNewDLLFunctions.
- Cliente: 107 arquivos compilam; linka sem simbolos indefinidos (exceto VGUI, fornecido pelo FreeVGUI no workflow).

## NAO verificado (precisa testar no celular)
- Build com o NDK (clang) no GitHub Actions: pode surgir algum erro especifico do clang.
- Execucao no Xash3D: fullbright _light.mdl, fog, HUD, particulas (particleman nao existe no Android), mapas com lumps desalinhados em 32 bits.
