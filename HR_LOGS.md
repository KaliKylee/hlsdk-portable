# Logs do Half-Rats: Parasomnia (Android)

As libs `libhrp_android_*` (server) e `libclient_android_*` (client) gravam dois arquivos na
pasta de trabalho do Xash, normalmente `/sdcard/xash/` (a mesma do `engine.log`):

- `hr_log.txt`   eventos: lib carregada (com arquitetura e data do build), GameDLLInit, mapa
                 (ServerActivate), jogador entrando, cliente iniciando. Passa de 512 KB e e zerado.
- `hr_crash.log` relatorio de crash: sinal e causa (ex.: "acesso DESALINHADO"), registradores,
                 funcao e modulo da falha (PC), quem chamou (LR), cadeia de chamadas, varredura da pilha
                 e as **ultimas 64 linhas do historico**: ultimas entidades criadas (`spawn monster_...`)
                 e armas pre-carregadas (`precache weapon ...`). Mostra em que ponto do "novo jogo" caiu.

Tambem sai no logcat com as tags HR e HR_CRASH.

## Se o jogo cair, mande
1. `hr_crash.log`
2. `hr_log.txt`
3. `engine.log` (ja gerado com `-log`)
4. O modelo do aparelho e se e 32 ou 64 bits.

## Resolver enderecos (quem tem as libs com simbolos)
O workflow gera o artefato **hlrats-libs-symbols** (libs sem strip, por arquitetura).
Para cada linha `pc 0000abcd libhrp_android_armv7l.so`:

    addr2line -f -C -e hlrats-libs-symbols/armeabi-v7a/libhrp_android_armv7l.so 0x0000abcd

Use a lib da MESMA build que o aparelho rodou (confira a data do build no cabecalho do log).
