@echo off
setlocal

:: Vérifier si un fichier a été passé en argument
if "%~1"=="" (
    echo [ERREUR] Tu dois specifier le fichier a compiler.
    echo Usage: build_release.bat ^<nom_du_fichier.cpp^>
    exit /b 1
)

set SOURCE=%~1
:: Extrait le nom du fichier sans l'extension pour nommer l'exe
set OUTNAME=%~n1.exe

echo ====================================================
echo [BUILD] Compilation de %SOURCE% en mode RELEASE...
echo ====================================================

:: Commande de compilation (MinGW / GCC)
g++ "%SOURCE%" -o "%OUTNAME%" -O3 -DNDEBUG -std=c++17 -lraylib -lgdi32 -lwinmm

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCES] L'executable "%OUTNAME%" a ete genere !
    echo Lance-le pour ta demo.
) else (
    echo.
    echo [ERREUR FATALE] La compilation a echoue. Verifie tes librairies.
)