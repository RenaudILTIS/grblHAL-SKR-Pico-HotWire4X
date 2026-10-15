# HotWire4X — Firmware grblHAL pour découpeuse fil chaud 4 axes

**Variante personnalisée** par Renaud ILTIS, basée sur grblHAL/RP2040 pour la carte BigTreeTech SKR Pico v1.0. Fournie en plug-and-play pour les machines amateur de découpe au fil chaud avec deux tours synchrones (X/Y et Z/A).

Ce document est la **fiche de référence** du firmware : identité, fonctionnalités, valeurs par défaut, préparation matérielle indispensable. Pour le câblage détaillé (module MOSFET hotwire, alimentation, recompilation), voir [`README.md`](README.md).

---

## Identification du firmware

Après flash de `build/grblHAL.uf2`, la commande `$I` doit renvoyer :

```
[BOARD:SKR Pico v1.0 HotWire4X SERVOS - R.ILTIS]
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
| **Bouton STOP** | Sur le header PROBE (GP22), bouton-poussoir NO direct vers GND. C'est l'entrée « porte » (safety door) : le mouvement freine jusqu'à l'arrêt, la chauffe est coupée, les moteurs restent sous tension (selon `$1`). Ce n'est **pas** un arrêt d'urgence : un vrai arrêt d'urgence coupe l'alimentation des moteurs et de la chauffe |
| **Bouton Cycle Start (Resume)** | Sur le header TFT, pin GP1 |
| **Bouton Feed Hold (pause)** | Sur le header TFT, pin GP0 |
| **Polarité par défaut NO** | Tous les boutons et fins de course sont configurés en normalement-ouvert (`$5=15`, `$14=14`) — câblage direct vers GND, plug-and-play avec des boutons-poussoir grand public |
| **Probe désactivé** | GP22 est dédié au bouton STOP ; le probe peut être réactivé sur un autre GPIO libre après modification (voir README) |

---

## Préparation matérielle obligatoire

### Strap P_S/IO22/PROBE sur la carte

La SKR Pico v1.0 possède un **strap 3 broches** étiqueté `P_S/IO22/PROBE` qui route GP22 soit vers l'entrée opto-couplée du connecteur `WD-DET` (broche `P_S`), soit en direct (sans isolation) vers le connecteur `PROBE`.

**Position requise pour ce firmware :** strap côté **PROBE**, c'est-à-dire reliant la pin centrale `IO22` à la pin `PROBE`. GP22 est alors disponible en GPIO brut sur le connecteur `PROBE`, où l'on câble le bouton STOP entre `PROBE` et `GND` (cf. tableau ci-dessous).

L'entrée opto-couplée `P_S` du connecteur `WD-DET` n'est **pas utilisable pour un bouton-poussoir passif** : elle attend un signal alimenté (typiquement 12-24 V) issu d'un capteur, et un simple poussoir vers GND ne fait jamais conduire l'opto-coupleur. Le routage côté `PROBE` contourne ce circuit d'entrée et ramène GP22 sur une broche directement utilisable, comme les entrées de fins de course.

C'est une opération unique faite à la fabrication ; aucune raison de manipuler ce strap ensuite.

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
| **STOP** (entrée porte) | PROBE | PROBE | GP22 (GPIO brut, pull-up interne) |

Brochage du header TFT : `[+5V] [+5V] [GND] [RXD0=GP1] [TXD0=GP0]`. GP1 (Cycle Start) est en pin 4, adjacent à la GND centrale (pin 3), ce qui permet de câbler un bouton Cycle Start avec un simple fil 2 broches sur les pins 3 et 4.

Câblage de chaque bouton : un poussoir momentané NO entre la pin GPIO et GND. Les pull-ups internes du RP2040 maintiennent la pin haute au repos.

### Sorties spéciales

| Fonction | Connecteur | GPIO | Niveau |
|---|---|---|---|
| Hotwire PWM | SERVO | GP29 | 3.3 V |
| Spindle Enable | FAN1 | GP17 | 3.3 V |
| Coolant Flood | FAN3 | GP20 | 3.3 V |
| Coolant Mist | HE | GP23 | 3.3 V |

#### Sortie « chauffe en marche » (FAN1) — indicateur exploitable

Le firmware active **GP17 (connecteur FAN1)** en même temps que la chauffe : c'est le signal *Spindle Enable* de grblHAL. `M3` met FAN1 à l'état actif — la LED voisine du connecteur s'allume — et `M5` le désactive.

Ce comportement est **normal et voulu**. La sortie est en tout-ou-rien, indépendante de la puissance : elle brille pareil à `S1` et à `S1000`, elle signale seulement que la chauffe est commandée.

Cette sortie étant commutée par un MOSFET de la carte sur le rail VMOT (12–24 V), elle constitue un **contact « chauffe en marche » automatique**, utilisable tel quel pour :

- un **voyant de sécurité** « fil sous tension » — particulièrement pertinent en fil chaud, où un fil chaud ne se distingue pas visuellement d'un fil froid ;
- un **ventilateur d'extraction de fumées**, qui démarre et s'arrête exactement en même temps que la chauffe.

Aucun réglage n'est nécessaire : il suffit de câbler la charge sur FAN1. À l'inverse, si FAN1 est déjà affecté à autre chose, garder à l'esprit que cette sortie sera commutée à chaque `M3`/`M5`.

---

## Valeurs par défaut (`$$` après `$RST=$`)

Toutes les valeurs ci-dessous sont compilées dans le firmware et appliquées sur première écriture en NVS, ou après `$RST=$` suivi d'un **reboot complet** (Ctrl-X ne suffit pas pour appliquer les masques d'inversion au niveau GPIO).

### Mécanique commune aux 4 axes

| Paramètre | Valeur | Signification |
|---|---:|---|
| `$0` | 5 µs | Largeur d'impulsion step |
| `$1` | 25 | Délai en ms avant désactivation des moteurs après le dernier pas (voir note ci-dessous) |
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
| `$14` | 14 | Feed Hold + Cycle Start + STOP (porte) tous en normalement-ouvert |

Bits de `$14` : `8` (porte = STOP) + `4` (Cycle Start) + `2` (Feed Hold) = `14`. Il n'y a plus d'entrée E-Stop : grblHAL ignore le bit `64` (HotWire4X envoie `78`, la carte garde `14`).

> Pour un bouton STOP fail-safe NC, définir `$14=6` au runtime (ou cocher « Bouton STOP » dans la colonne NC de HotWire4X) — cela conserve Feed Hold et Cycle Start en NO et bascule le STOP en NC.

### Homing

`$22=0` (homing désactivé). Découpe au fil chaud n'a typiquement pas de cycle de référence automatique ; le zéro machine est posé manuellement avec `G92 X0 Y0 Z0 A0`.

### Tenue des moteurs au repos (`$1`)

Paramètre modifiable au runtime, exprimé en millisecondes :

- `$1=0` : désactivation immédiate des drivers après le dernier pas
- `$1=1` à `$1=254` : désactivation après N ms
- `$1=255` : drivers maintenus actifs en permanence (pas de désactivation)

---

## Procédure de premier démarrage

1. **Flasher** `build/grblHAL.uf2` (mode BOOTSEL → glisser-déposer sur le drive `RPI-RP2`).
2. **Alimenter VMOT** 12-24 V au bornier (en plus de l'USB).
3. **Vérifier le strap P_S/IO22/PROBE** côté `PROBE` sur la carte.
4. Connecter sur le port USB CDC, ouvrir un terminal série (ex. ioSender, gSender, ou simple `screen`/`putty` à 115200 bauds).
5. Envoyer `Ctrl-X` (soft reset). Attendre la bannière `GrblHAL 1.1f`.
6. Envoyer `$RST=$` pour appliquer toutes les valeurs par défaut HotWire4X.
7. **Reboot complet** (couper USB **et** VMOT, attendre ~5 s, rebrancher). Indispensable pour que les masques d'inversion `$5=15` et `$14=14` soient appliqués au niveau du peripheral GPIO — un Ctrl-X ne suffit pas.
8. Au reboot, envoyer `?` directement. L'état attendu est :

   ```
   <Idle|MPos:0.000,0.000,0.000,0.000|Bf:100,1022|FS:0,0|Ov:100,100,100|A:|TLR:0>
   ```

   Si vous voyez `<Door:1|...|Pn:D>`, `Pn:H`, `Pn:S` ou similaire : un cavalier ou un fil parasite est posé sur GP0/GP1/GP22, ou le strap P_S/IO22/PROBE n'est pas bien positionné.

9. Vérifier `$I` :
   - `[BOARD:SKR Pico v1.0 HotWire4X SERVOS - R.ILTIS]` ✓
   - `[NEWOPT:...,TMC=15,...]` ✓
   - `[SIGNALS:...]` contient `H`, `S` et `D` (capacités présentes : Feed Hold, Cycle Start, porte = STOP), et plus `E`
10. Tester chaque entrée individuellement en posant un cavalier vers GND :
    - X-STOP/Y-STOP/Z-STOP/E0-STOP → `Pn:X` / `Pn:Y` / `Pn:Z` / `Pn:A`
    - GP0 → `Pn:H`
    - GP1 → `Pn:S`
    - Connecteur PROBE → `Pn:D`

---

## Comportement opérationnel

### Pendant un mouvement

- **Bouton Feed Hold** (GP0) → état `Run` → `Hold` → mouvement décéléré et mis en pause, chauffe maintenue
- **Bouton Cycle Start** (GP1) après un Feed Hold → état `Hold` → `Run` → mouvement repris
- **Bouton STOP** (GP22) → mouvement freiné jusqu'à l'arrêt, chauffe coupée, moteurs sous tension (selon `$1`), état `Door` : `Door:2` pendant le freinage, `Door:1` bouton enfoncé, `Door:0` bouton relâché. Pas d'alarme, la position est conservée.

### Après un STOP

1. Relâcher le bouton (état `Door:0`)
2. `Ctrl-X` (soft reset) : annule le reste du programme, sans alarme puisque la machine est arrêtée — HotWire4X le fait tout seul
3. État revient à `Idle`

`~` (Cycle Start) reprendrait le programme suspendu après le délai `$392` ; HotWire4X ne s'en sert pas : il recrée lui-même la suite de la découpe.

---

## Références

- **Câblage détaillé du module MOSFET hotwire** : voir `README.md` § "Câblage du module MOSFET"
- **Recompilation depuis les sources** : voir `README.md` § "Recompiler après modification"
- **Surcharger une valeur par défaut** : éditer le bloc `target_compile_definitions(grblHAL PUBLIC ...)` dans `CMakeLists.txt`. Les valeurs sont reflétées vers `grbl/*.c` via `-D`. Une simple modification de `my_machine.h` ne suffit pas (le core grbl n'inclut pas ce fichier).
- **Source du board map** : `boards/btt_skr_pico_10_map.h`
