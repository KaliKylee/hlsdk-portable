# Launcher do Absolute Zero (Android)

Projeto Gradle do APK "Absolute Zero". Ele so inicia o Xash3D FWGS com o mod; as libs do mod
(libaz_android_<arch>.so e libclient_android_<arch>.so) vao dentro do APK, em `app/src/main/jniLibs/<abi>/`.

- O workflow `.github/workflows/android.yml` compila as libs, copia para ca e gera o artefato **AbsoluteZero-apk**.
- gamedir usado: `AbsoluteZero` (pasta do mod dentro dos dados do Xash3D FWGS).
- Montar na mao: coloque as libs em `app/src/main/jniLibs/arm64-v8a/` (e `armeabi-v7a/`) e rode `./gradlew assembleDebug`.
