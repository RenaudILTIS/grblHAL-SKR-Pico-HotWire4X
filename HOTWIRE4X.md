# HotWire4X — Firmware grblHAL pour découpeuse fil chaud 4 axes

**Variante personnalisée** par Renaud ILTIS, basée sur grblHAL/RP2040 pour la carte BigTreeTech SKR Pico v1.0. Fournie en plug-and-play pour les machines amateur de découpe au fil chaud avec deux tours synchrones (X/Y et Z/A).

Ce document est la **fiche de référence** du firmware : identité, fonctionnalités, valeurs par défaut, préparation matérielle indispensable. Pour le câblage détaillé (module MOSFET hotwire, alimentation, recompilation), voir [`README.md`](README.md).

---

## Identification du firmware

Après flash de `build/grblHAL.uf2`, la commande `$I` doit renvoyer :

```
[BOARD:SKR Pico v1.0 HotWire4X - R.ILTIS]
```

Si vous voyez `[BOARD:BTT SKR Pico 1.0 SERVO]` ou autre, c'est un firmware antérieur ou non personnalisé — re-flasher.

Le rapport `[NEWOPT:...]` doit également contenir `TMC=15` (4 drivers TMC2209 répondants en UART). Sans ce marqueur, vérifier que la carte est bien alimentée par VMOT 12-24 V (l'USB seul ne suffit pas).

---

## Fonctionnalités

| Capacité | Détail |
|---|---|
| **4 axes** | X, Y, Z, A (4ᵉ axe sur le port E0) — kinematique cartésienne, les deux tours pilotent indépendamment |
| **Drivers TMC2209 UART** | Courant + microsteps configurables au runtime via `$140-$143` et `$150-$153`, monitoring `M122` |
| **Sortie chauffe PWM** | Sur le header SERVO (GP29), pour module MOSFET externe opto-isolé |
| **Arrêt d'urgence** | Sur le header WD-DET (GP22), opto-couplé sur la carte |
| **Bouton Cycle Start (Resume)** | Sur le header TFT, pin GP1 |
| **Bouton Feed Hold (pause)** | Sur le header TFT, pin GP0 |
| **Polarité par défaut NO** | Tous les boutons et fins de course sont configurés en normalement-ouvert (`$5=15`, `$14=70`) — câblage direct vers GND, plug-and-play avec des boutons-poussoir grand public |
| **Probe désactivé** | GP22 est dédié à l'E-Stop ; le probe peut être réactivé sur GP21 (header HB) après modification (voir README) |

---

## Préparation matérielle obligatoire

### Jumper P_S/IO22 sur la carte

La SKR Pico v1.0 possède un **jumper physique** (ou strap soudé) qui route le signal GP22 vers l'un des deux connecteurs : `WD-DET` (utilisé ici pour l'E-Stop) ou `PROBE`. Il est étiqueté `P_S/IO22` à proximité de ces connecteurs.

**Position requise pour ce firmware :** placer le jumper côté `P_S` du connecteur `WD-DET`. Sans ce jumper, l'entrée E-Stop est flottante et la machine restera bloquée en alarme `error:79` au démarrage.

C'est une opération unique faite à la fabrication ; aucune raison de le manipuler ensuite.

### Alimentation VMOT — obligatoire pour configurer les drivers

Le bornier 12-24 V (VMOT/GND, à côté des fusibles) **doit** être alimenté pour que les drivers TMC2209 fonctionnent. L'USB seul alimente le RP2040 mais pas les drivers.

**Conséquence directe** : toute écriture sur les paramètres TMC `$140-$143` (courants moteurs) et `$150-$153` (microsteps) **est silencieusement ignorée si VMOT n'est pas branché**. Le terminal renvoie `ok`, la valeur semble écrite, mais elle n'atteint jamais les drivers et `$$` continue de retourner les anciennes valeurs (typiquement `$140-143=500` et `$150-153=16`).

**Symptôme caractéristique** : `$I` ne contient pas `TMC=15` dans la ligne `[NEWOPT:...]`. Si ce marqueur est absent, c'est qu'aucun driver ne répond en UART → vérifier VMOT avant toute autre piste.

Pour toute opération de configuration des drivers (changement de courant, de microstepping, exécution de `M122`), brancher VMOT au minimum 12 V même si la machine n'a pas besoin de bouger.

---

## Mapping des entrées/sorties

### Fins de course (4 axes)

| Axe | Connecteur | GPIO |
|---|---|---|
| X | X-STOP | GP4 |
| Y | Y-STOP | GP3 |
| Z | Z-STOP | GP25 |
| A | E0-STOP | GP16 |

### Boutons de contrôle

| Bouton | Connecteur | Pin | GPIO |
|---|---|---|---|
| **Cycle Start** (Resume) | TFT (5 broches 2.54mm) | RXD0 | GP1 |
| **Feed Hold** (pause) | TFT (5 broches 2.54mm) | TXD0 | GP0 |
| **E-Stop** | WD-DET | P_S | GP22 (opto-couplé) |

Brochage du header TFT : `[+5V] [+5V] [GND] [RXD0=GP1] [TXD0=GP0]`. GP1 (Cycle Start) est en pin 4, adjacent à la GND centrale (pin 3), ce qui permet de câbler un bouton Cycle Start avec un simple fil 2 broches sur les pins 3 et 4.

Câblage de chaque bouton : un poussoir momentané NO entre la pin GPIO et GND. Les pull-ups internes du RP2040 maintiennent la pin haute au repos.

### Sorties spéciales

| Fonction | Connecteur | GPIO | Niveau |
|---|---|---|---|
| Hotwire PWM | SERVO | GP29 | 3.3 V |
| Spindle Enable | FAN1 | GP17 | 3.3 V |
| Coolant Flood | FAN3 | GP20 | 3.3 V |
| Coolant Mist | HE | GP23 | 3.3 V |

---

## Valeurs par défaut (`$$` après `$RST=$`)

Toutes les valeurs ci-dessous sont compilées dans le firmware et appliquées sur première écriture en NVS, ou après `$RST=$` suivi d'un **reboot complet** (Ctrl-X ne suffit pas pour appliquer les masques d'inversion au niveau GPIO).

### Mécanique commune aux 4 axes

| Paramètre | Valeur | Signification |
|---|---:|---|
| `$0` | 5 µs | Largeur d'impulsion step |
| `$11` | 0.010 mm | Junction deviation (très serrée pour fil chaud) |
| `$100-$103` | 160 | Steps/mm (courroie GT2 × poulie 20 dents × moteur 200 pas × 32 µsteps) |
| `$110-$113` | 2400 mm/min | Vitesse max par axe |
| `$120-$123` | 2000 mm/s² | Accélération par axe (rampes très raides — privilégie vitesse linéaire constante) |
| `$140-$143` | 800 mA RMS | Courant moteur TMC2209 — **modification possible uniquement avec VMOT branché** |
| `$150-$153` | 32 | Microsteps TMC2209 — **idem, requiert VMOT actif** |

### Courses différenciées par axe

| Paramètre | Axe | Valeur | Note |
|---|---|---:|---|
| `$130` | X | 850 mm | Rail horizontal, tour 1 |
| `$131` | Y | 450 mm | Tour vertical 1 |
| `$132` | Z | 850 mm | Rail horizontal, tour 2 |
| `$133` | A | 450 mm | Tour vertical 2 |

### Polarité des entrées (NO partout)

| Paramètre | Valeur | Signification |
|---|---:|---|
| `$5` | 15 | Limites X/Y/Z/A toutes en normalement-ouvert |
| `$14` | 70 | Feed Hold + Cycle Start + E-Stop tous en normalement-ouvert |

Bits de `$14` : `64` (E-Stop) + `4` (Cycle Start) + `2` (Feed Hold) = `70`.

> Pour un E-Stop fail-safe NC (recommandé en milieu industriel mais pas nécessaire en amateur), définir `$14=6` au runtime — cela conserve Feed Hold et Cycle Start en NO et bascule l'E-Stop en NC.

### Homing

`$22=0` (homing désactivé). Découpe au fil chaud n'a typiquement pas de cycle de référence automatique ; le zéro machine est posé manuellement avec `G92 X0 Y0 Z0 A0`.

---

## Procédure de premier démarrage

1. **Flasher** `build/grblHAL.uf2` (mode BOOTSEL → glisser-déposer sur le drive `RPI-RP2`).
2. **Alimenter VMOT** 12-24 V au bornier (en plus de l'USB).
3. **Vérifier le jumper P_S/IO22** côté WD-DET sur la carte.
4. Connecter sur le port USB CDC, ouvrir un terminal série (ex. ioSender, gSender, ou simple `screen`/`putty` à 115200 bauds).
5. Envoyer `Ctrl-X` (soft reset). Attendre la bannière `GrblHAL 1.1f`.
6. Envoyer `$RST=$` pour appliquer toutes les valeurs par défaut HotWire4X.
7. **Reboot complet** (couper USB **et** VMOT, attendre ~5 s, rebrancher). Indispensable pour que les masques d'inversion `$5=15` et `$14=70` soient appliqués au niveau du peripheral GPIO — un Ctrl-X ne suffit pas.
8. Au reboot, envoyer `?` directement. L'état attendu est :

   ```
   <Idle|MPos:0.000,0.000,0.000,0.000|Bf:100,1022|FS:0,0|Ov:100,100,100|A:|TLR:0>
   ```

   Si vous voyez `<Alarm|...|Pn:HSE>` ou similaire : un cavalier ou un fil parasite est posé sur GP0/GP1/GP22, ou le jumper P_S/IO22 n'est pas bien positionné.

9. Vérifier `$I` :
   - `[BOARD:SKR Pico v1.0 HotWire4X - R.ILTIS]` ✓
   - `[NEWOPT:...,TMC=15,...]` ✓
   - `[SIGNALS:HSE]` (capacités présentes : Feed Hold, Cycle Start, E-Stop)
10. Tester chaque entrée individuellement en posant un cavalier vers GND :
    - X-STOP/Y-STOP/Z-STOP/E0-STOP → `Pn:X` / `Pn:Y` / `Pn:Z` / `Pn:A`
    - GP0 → `Pn:H`
    - GP1 → `Pn:S`
    - WD-DET P_S → `Pn:E`

---

## Comportement opérationnel

### Pendant un mouvement

- **Bouton Feed Hold** (GP0) → état `Run` → `Hold` → mouvement décéléré et mis en pause, chauffe maintenue
- **Bouton Cycle Start** (GP1) après un Feed Hold → état `Hold` → `Run` → mouvement repris
- **Bouton E-Stop** (GP22) → arrêt immédiat tous moteurs + chauffe coupée + état `Alarm:1`

### Pour sortir d'un E-Stop

1. Relâcher le bouton E-Stop physique
2. `Ctrl-X` (soft reset) ou `$X` (unlock)
3. État revient à `Idle`

---

## Références

- **Câblage détaillé du module MOSFET hotwire** : voir `README.md` § "Câblage du module MOSFET"
- **Recompilation depuis les sources** : voir `README.md` § "Recompiler après modification"
- **Surcharger une valeur par défaut** : éditer le bloc `target_compile_definitions(grblHAL PUBLIC ...)` dans `CMakeLists.txt`. Les valeurs sont reflétées vers `grbl/*.c` via `-D`. Une simple modification de `my_machine.h` ne suffit pas (le core grbl n'inclut pas ce fichier).
- **Source du board map** : `boards/btt_skr_pico_10_map.h`
