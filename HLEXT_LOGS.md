# Logs do Half-Life: Extended (Android)

As libs `libhlex_android_*` (server) e `libclient_android_*` (client) gravam dois arquivos na
pasta de trabalho do Xash, normalmente `/sdcard/xash/` (a mesma do `engine.log`):

- `hlext_log.txt`   eventos: lib carregada (com arquitetura e data do build), GameDLLInit, mapa
                    (ServerActivate), jogador entrando, cliente iniciando e modelos que nao foram
                    encontrados (viram `models/error.mdl`). Passa de 512 KB e e zerado.
- `hlext_crash.log` relatorio de crash: sinal e causa (ex.: "acesso DESALINHADO"), registradores,
                    funcao e modulo da falha (PC), quem chamou (LR), cadeia de chamadas, varredura da pilha
                    e as **ultimas 64 linhas do historico**: ultimas entidades criadas (`spawn monster_...`)
                    e armas pre-carregadas (`precache weapon ...`). Mostra em que ponto do mapa caiu.

Tambem sai no logcat com as tags `HLEXT` e `HLEXT_CRASH`.

## Se o jogo cair (ex.: ao abrir o c0a0 pelo console), mande
1. `hlext_crash.log`
2. `hlext_log.txt`
3. `engine.log` (ja gerado com `-log`)
4. O modelo do aparelho e se e 32 ou 64 bits.

## Resolver enderecos (quem tem as libs com simbolos)
O workflow gera o artefato **HLExtended-libs-symbols-<abi>** (libs sem strip, por arquitetura).
Para cada linha `pc 0000abcd libhlex_android_arm64.so`:

    addr2line -f -C -e HLExtended-libs-symbols-arm64-v8a/libhlex_android_arm64.so 0x0000abcd

Use a lib da MESMA build que o aparelho rodou (confira a data do build no cabecalho do log).
