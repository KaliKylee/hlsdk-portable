# Logs do Invasion (Android)

As libs do mod (`libinvasion_android_*` = server e `libclient_android_*` = client) gravam
dois arquivos na pasta de trabalho do Xash, normalmente `/sdcard/xash/` (a mesma do `engine.log`):

- `invasion_log.txt`   eventos: lib carregada (arquitetura e data do build), GameDLLInit, mapa
                       (ServerActivate), jogador entrando, cliente iniciando. Passa de 512 KB e e zerado.
- `invasion_crash.log` relatorio de crash: sinal e causa (ex.: "acesso DESALINHADO"), registradores,
                       funcao e modulo da falha (PC), quem chamou (LR), cadeia de chamadas, varredura
                       da pilha e as **ultimas 64 linhas do historico**: ultimas entidades criadas
                       (`spawn monster_...`) e armas pre-carregadas (`precache weapon ...`).

Tambem sai no logcat com as tags `INV` e `INV_CRASH`.

## Se o jogo cair, mande
1. `invasion_crash.log`
2. `invasion_log.txt`
3. `engine.log` (gerado com `-log`)
4. O modelo do aparelho e se e 32 ou 64 bits.

## Resolver enderecos (quem tem as libs com simbolos)
O workflow gera os artefatos **Invasion-libs-symbols-<abi>** (libs sem strip, por arquitetura).
Para cada linha `pc 0000abcd libinvasion_android_armv7l.so`:

    addr2line -f -C -e Invasion-libs-symbols-armeabi-v7a/libinvasion_android_armv7l.so 0x0000abcd

Use a lib da MESMA build que o aparelho rodou (confira a data do build no cabecalho do log).
