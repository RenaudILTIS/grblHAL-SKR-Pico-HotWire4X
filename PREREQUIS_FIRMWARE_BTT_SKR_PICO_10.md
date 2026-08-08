# Prérequis techniques — Firmware de la machine

Le logiciel **HotWire4X** communique avec une découpeuse à fil chaud 4 axes équipée d'une carte **BigTreeTech SKR Pico v1.0** flashée avec le firmware **grblHAL — variante HotWire4X** (basé sur grblHAL/RP2040, board `BOARD_BTT_SKR_PICO_10`).

Ce firmware est **spécifique et obligatoire** : il configure les 4 axes, la sortie de chauffe PWM sur le header SERVO et l'arrêt d'urgence. Un firmware grblHAL générique ne fonctionnera pas correctement avec le logiciel HotWire4X.

> **Ne pas confondre :** le **logiciel HotWire4X** (application de pilotage sur PC) et le **firmware HotWire4X** (embarqué dans la carte de la machine). Les deux portent le même nom mais sont distincts et complémentaires.

## À vérifier avant utilisation

| Élément | Attendu |
|---|---|
| Carte | BTT SKR Pico v1.0 (RP2040) |
| Firmware | grblHAL — variante HotWire4X |
| Identification (`$I`) | `[BOARD:SKR Pico v1.0 HotWire4X - R.ILTIS]` |
| Drivers moteurs (`$I` → `[NEWOPT:…]`) | contient `TMC=15` (4 drivers TMC2209 détectés) |
| Liaison PC ↔ machine | USB, port série virtuel, 115200 bauds |
| Alimentation | Bornier moteur **12–24 V (VMOT)** branché, en plus de l'USB |

## Obtenir le firmware

Le firmware HotWire4X (fichier de flashage prêt à l'emploi) ainsi que la documentation de flashage et de câblage sont fournis sur **[hotwire4x.com](https://hotwire4x.com)**.

Le **code source** du firmware est disponible **sur demande**.
