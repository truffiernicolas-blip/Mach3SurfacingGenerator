# Mach3 Surfacing Generator

Version 0.2.0 - Copyright 2026 Nicolas Truffier - Licence MIT.

Application desktop Qt 6 / C++17 pour générer un parcours de surfaçage
rectangulaire en zigzag, exportable en `.nc` ou `.tap` pour Mach3.

Le projet est open source sous [licence MIT](LICENSE).

## Fonctions disponibles

- sélection et édition de presets outils et matières ;
- gestion du numéro d'outil, du diamètre et du nombre de dents ;
- saisie des dimensions, profondeurs, Z de sécurité et d'approche ;
- origine en bas à gauche ou en bas à droite ;
- balayage suivant X ou Y ;
- prévisualisation 2D du parcours ;
- calcul des passes Z et du résumé d'usinage ;
- export Mach3 en `G21`, `G90`, `G17`, `G94`, `G0`, `G1`, `M3`, `M5`, `M30`.
- calcul affiché de la vitesse de coupe et de l'avance par dent.

Les presets fournis comprennent une fraise de 6 mm à 2 dents et deux
conditions Labelite :

- finition : `S12000`, `F2002,1 mm/min` ;
- rainurage : `S12000`, `F1598,9 mm/min`.

Ces avances conservent les avances par dent données pour une fraise à
3 dents. Les avances Z sont volontairement plus faibles car le générateur
effectue une plongée verticale directe.

Le format précis du post-processeur est isolé dans
`src/Mach3PostProcessor.cpp`. Il pourra être adapté à partir d'un programme
Mach3 validé sur la machine.

## Compilation sous Windows

Prérequis :

- Qt 6 avec le module Widgets ;
- CMake 3.21 ou plus récent ;
- un compilateur C++17 compatible avec le kit Qt choisi ;
- Ninja, Visual Studio ou MinGW.

Exemple avec Qt 6 et MinGW :

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\mingw_64" `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Avec Qt Creator, ouvrir directement `CMakeLists.txt`, sélectionner un kit
Qt 6, puis construire la cible `Mach3SurfacingGenerator`.

## Exécution

```powershell
.\build\Mach3SurfacingGenerator.exe
```

Les presets modifiables sont stockés dans le dossier de configuration de
l'utilisateur. Les exports sont proposés dans le dossier Documents, sous
`Mach3SurfacingGenerator`.

Pour distribuer l'exécutable MinGW avec ses DLL Qt :

```powershell
C:\Qt\6.11.0\mingw_64\bin\windeployqt.exe `
  .\build\Mach3SurfacingGenerator.exe
```

## Releases

Release Windows portable, avec DLL Qt, tests et signature locale :

```powershell
.\scripts\Build-WindowsRelease.ps1
```

Releases Linux Ubuntu/Debian :

```bash
sudo apt install cmake ninja-build g++ qt6-base-dev qt6-base-dev-tools
bash scripts/build-linux-release.sh
```

Les artefacts sont générés dans `dist` :

- ZIP autonome Windows x64 ;
- paquet Debian x64, avec installation automatique des dépendances ;
- archive Linux x64, nécessitant le runtime Qt 6 du système.

Les sommes SHA-256 sont produites avec :

```powershell
.\scripts\Generate-Checksums.ps1
```

Le workflow `.github/workflows/release.yml` construit ces artefacts et crée
automatiquement une release GitHub lorsqu'un tag `v*` est poussé.

## Signature du code

Pour les tests internes sur un poste maîtrisé, créer et installer le
certificat local dédié :

```powershell
.\scripts\Create-LocalSigningCertificate.ps1
.\scripts\Sign-Release.ps1
```

La clé privée reste dans le magasin de certificats de l'utilisateur Windows.
Seul le certificat public est exporté dans `certificates`.

Pour une distribution publique et la compatibilité Smart App Control, une
signature locale ne suffit pas. Utiliser un certificat de signature de code
émis par un fournisseur approuvé, ou Microsoft Artifact Signing. Définir
ensuite son empreinte avant d'appeler le script :

```powershell
$env:MACH3_SIGNING_CERT_THUMBPRINT = "EMPREINTE_DU_CERTIFICAT"
$env:MACH3_TIMESTAMP_URL = "URL_DU_SERVEUR_D_HORODATAGE"
.\scripts\Sign-Release.ps1
```

## Sécurité

Le programme généré doit être simulé et vérifié dans Mach3 avant tout
usinage réel. Les valeurs de presets fournies sont des exemples et ne
constituent pas des paramètres garantis pour une machine ou un outil donné.
