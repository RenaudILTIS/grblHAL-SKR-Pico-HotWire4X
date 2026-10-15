# grblHAL Hotwire SERVO — BTT SKR Pico v1.0

Variante SERVO du board map `BOARD_BTT_SKR_PICO_10` de grblHAL/RP2040 pour la découpe par fil chaud (foam cutter), 4 axes, drivers TMC2209 en UART, sortie de chauffe sur le connecteur SERVO via module MOSFET externe.

> **Alimentation** : pour la mise au point complète (test des drivers TMC2209 via UART, `M122`, persistance des microsteps, etc.) il est **impératif** d'alimenter la carte par le bornier 12–24 V (`VMOT`/`GND`, à côté des fusibles), **pas par le jumper USB seul**. Sans VM, les TMC2209 ne sont pas alimentés et ne répondent pas en UART : `[NEWOPT:...]` ne contient pas `TMC=15`, et toute écriture `$15x=...` est silencieusement ignorée (la valeur lue par `$$` reste à 16).

## Vérification du flash

Après flash de `build/grblHAL.uf2` (mode BOOTSEL puis copie sur le lecteur RPI-RP2), la commande grblHAL `$I` doit renvoyer une ligne contenant :

```
[BOARD:SKR Pico v1.0 HotWire4X SERVOS - R.ILTIS]
```

Si vous voyez `[BOARD:BTT SKR Pico 1.0]` ou tout autre nom ne contenant pas « HotWire4X » et le suffixe « - R.ILTIS », c'est le firmware standard ou une variante antérieure — re-flasher. Selon la variante flashée, le nom est `HotWire4X SERVOS` (sortie GP29, module externe) ou `HotWire4X HB` (sortie GP21, MOSFET embarqué).

Vérifier également que `[NEWOPT:...]` contient `TMC=15` : c'est le mask des 4 drivers TMC2209 ayant répondu en UART. Si `TMC=15` est absent, vérifier l'alimentation 12–24 V du bornier VMOT (cf. encadré en tête de README).

## Pinout des connecteurs utiles (sérigraphie SKR Pico v1.0)

### Moteurs (drivers TMC2209, UART)

| Axe grblHAL | Connecteur moteur | STEP | DIR  | EN   |
| ----------- | ----------------- | ----:| ----:| ----:|
| X           | X-MOT             | GP11 | GP10 | GP12 |
| Y           | Y-MOT             | GP6  | GP5  | GP7  |
| Z           | Z-MOT             | GP19 | GP28 | GP2  |
| A (4ᵉ axe)  | **E-MOT**         | GP14 | GP13 | GP15 |

### Fins de course

| Endstop | Connecteur  | GPIO | Notes                        |
| ------- | ----------- | ----:| ---------------------------- |
| X       | X-STOP      | GP4  | Entrée 3 broches SIG/+5V/GND |
| Y       | Y-STOP      | GP3  | idem                         |
| Z       | Z-STOP      | GP25 | idem                         |
| A (M3)  | **E0-STOP** | GP16 | endstop du 4ᵉ axe            |

### Sorties spéciales

| Fonction              | Connecteur | GPIO | Niveau                 |
| --------------------- | ---------- | ----:| ---------------------- |
| Hotwire PWM (spindle) | **SERVO**  | GP29 | 3.3 V CMOS             |
| Spindle enable        | FAN1       | GP17 | 3.3 V                  |
| Coolant flood         | FAN3       | GP20 | 3.3 V                  |
| Coolant mist          | HE         | GP23 | 3.3 V                  |
| GPIO libre            | HB         | GP21 | 3.3 V (sans isolation) |

### Sortie « chauffe en marche » (FAN1) — voyant / extraction

En plus du PWM de chauffe, le firmware pilote **GP17 sur le connecteur FAN1** : c'est le signal *Spindle Enable* de grblHAL, activé par `M3` et désactivé par `M5`. La LED voisine du connecteur FAN1 s'allume donc dès que la chauffe est commandée — **c'est normal**, ce n'est pas un défaut.

Caractéristiques :

- **Tout-ou-rien** : l'état ne dépend pas de la puissance demandée (identique à `M3 S1` et `M3 S1000`), il indique seulement « chauffe activée ».
- **Commutée par MOSFET sur le rail VMOT (12–24 V)**, comme les autres sorties FAN/HE/HB de la carte.
- **Synchronisée automatiquement** avec la chauffe, sans aucun réglage ni G-code supplémentaire.

Cela en fait un contact « chauffe en marche » directement exploitable :

```
SKR Pico FAN1                     Charge 12-24 V
+-------------+                   (voyant ou ventilateur)
| V+  --------+---------------------- (+)
| OUT --------+---------------------- (-)
+-------------+
   commuté à la masse par le MOSFET quand M3 est actif
```

Usages typiques en fil chaud :

- **Voyant de sécurité « fil sous tension »** — essentiel car un fil chaud est visuellement identique à un fil froid.
- **Ventilateur d'extraction de fumées**, qui démarre et s'arrête exactement en même temps que la chauffe.

Respecter le calibre de la sortie FAN1 (courant modeste, de l'ordre de l'ampère) : pour une charge plus lourde, passer par un relais ou un module MOSFET piloté depuis cette sortie.

> Si FAN1 est déjà affecté à un autre usage sur votre machine, garder à l'esprit que cette sortie est commutée à chaque `M3`/`M5`.

### Entrée sécurité

| Fonction | Connecteur | Broche signal | GPIO | Caractéristique                                  |
| -------- | ---------- | ------------- | ----:| ------------------------------------------------ |
| STOP     | **PROBE**  | `PROBE`       | GP22 | Entrée porte, GPIO brut via strap côté `PROBE`   |

GND disponible sur tous les connecteurs (broche dédiée ou rail commun).

## Câblage du module MOSFET (sortie hotwire)

Module recommandé : carte MOSFET opto-isolée 3.3–24 V trigger (très courant, vendu pour pilotage de chauffe ou de bandeaux LED).

```
SKR Pico SERVO header                Module MOSFET opto-isolé
+-----------------+                  +---------------------+
| GP29 (signal) --+------------------+-- IN                |
| GND ------------+------------------+-- GND (logique)     |
| +5V (libre)     |                  |   VCC <- 5V logique |
+-----------------+                  |                     |
                                     |   V+  <- alim 48V   |
                                     |   V-  -> retour     |
                                     |        alim 48V     |
                                     |   OUT+ -> fil (+)   |
                                     |   OUT- -> fil (-)   |
                                     +---------------------+
```

**Points critiques** :

- **Pas de résistance série** entre GP29 et IN. Les modules opto-isolés intègrent déjà la résistance de limitation côté entrée, dimensionnée pour 3.3–24 V. Ajouter une résistance externe ferait passer la tension d'entrée sous le seuil de l'opto-coupleur.
- Alimentations **séparées** : la logique du module (VCC) tire son 5 V/12 V de la SKR Pico ou d'une source dédiée ; la charge 48 V passe uniquement par les bornes de puissance V+/V-.
- **GND commun obligatoire** entre la SKR Pico et la masse logique du module pour que le signal IN ait une référence.
- La masse 48 V de l'alim de chauffe peut rester séparée ; c'est tout l'intérêt de l'opto-isolation.

## Câblage du bouton STOP

Le bouton STOP de HotWire4X est l'entrée **porte** (safety door) de grblHAL : il arrête le mouvement en freinant, coupe la chauffe et garde les moteurs sous tension. Ce n'est **pas** un arrêt d'urgence (aucune entrée E-Stop n'est compilée) : un vrai arrêt d'urgence coupe l'alimentation des moteurs et de la chauffe.

L'entrée STOP est sur **GP22 brut**, exposé sur le connecteur **PROBE**. Le firmware ne se sert pas de l'opto-coupleur du connecteur `WD-DET` (broche `P_S`) : ce circuit attend un signal **alimenté** issu d'un capteur (typiquement 12-24 V), et un simple bouton-poussoir vers GND ne fait jamais conduire l'opto. La SKR Pico v1.0 prévoit un strap pour rerouter GP22 vers le connecteur PROBE en GPIO brut.

### Préparation du strap P_S/IO22/PROBE

La carte porte un strap 3 broches étiqueté `P_S — IO22 — PROBE` à proximité des connecteurs WD-DET et PROBE. La pin centrale est GP22 ; les pins extrêmes sont les destinations (P_S côté opto WD-DET, PROBE côté GPIO direct).

```
Strap P_S/IO22/PROBE
[ P_S ]   [ IO22 ]   [ PROBE ]
            |             |
            +----[CAVALIER]+   <- position requise (côté PROBE)
```

Mettre le cavalier sur les **deux pins de droite** (`IO22 ↔ PROBE`). GP22 est alors disponible directement sur le connecteur `PROBE` voisin, avec le pull-up interne du RP2040 actif.

### Brochage du connecteur PROBE

Connecteur 2 broches, sérigraphie BTT SKR Pico v1.0 :

| Broche silkscreen | Rôle                                       |
| ----------------- | ------------------------------------------ |
| `PROBE`           | GP22 brut (via le strap décrit ci-dessus)  |
| `GND`             | Masse                                      |

### Câblage

Bouton-poussoir **NO (normalement ouvert)** entre `PROBE` et `GND`. C'est la polarité par défaut du firmware (`$14=14`, bit 3 = porte NO) : aucun réglage CLI à faire.

```
SKR Pico PROBE           Bouton STOP
+-----------+            (contact NO — normalement ouvert)
| PROBE ----+-------+
| GND   ----+--+    |
+-----------+  |    |
              [bouton NO]
            ouvert au repos,
            fermé sur appui
```

Comportement attendu :

- **Au repos** (bouton relâché, contact **ouvert**) : la ligne reste haute via le pull-up interne. grblHAL voit la porte fermée et reste en `<Idle|...>`.
- **Sur appui** (contact fermé) : `PROBE` est tiré à GND par le bouton. grblHAL freine jusqu'à l'arrêt, coupe la chauffe et passe en `Door` (pas d'alarme).

`SAFETY_DOOR_ENABLE` (`my_machine.h`, recopié dans `CMakeLists.txt`) lie `SAFETY_DOOR_PIN = AUXINPUT1_PIN = GP22` dans le board map ; l'entrée est active dès le boot. `$61` (options de la porte) doit rester à 0.

> **Note fail-safe** : ce câblage NO n'est volontairement **pas fail-safe** (un câble coupé n'est pas détecté). C'est un compromis assumé en contexte amateur fil chaud (cf. fiche `HOTWIRE4X.md`). Pour un câblage NF/fail-safe en environnement industriel : utiliser un bouton NF entre `PROBE` et `GND`, et retirer le bit de la porte avec `$14=6` (Feed Hold + Cycle Start NO, STOP NF).

### Test de validation (avant câblage du bouton définitif)

Court-circuiter `PROBE` à `GND` avec un fil jumper, faire un soft reset (`Ctrl+X`), puis envoyer `?` :

```
?  →  doit retourner <Door:1|...|Pn:D|...>    (STOP actif tant que le pont est en place)
```

Retirer le pont, soft reset, `?` à nouveau :

```
?  →  <Door:0|...>                           (bouton relâché, machine arrêtée)
Ctrl+X, puis ?  →  <Idle|...|Pn:>
```

Si c'est conforme, câbler le bouton NO définitif.

## Premier démarrage — paramètres `$` recommandés

À adapter à votre mécanique. Connectez-vous via USB CDC (115200 bauds), puis envoyez ces commandes une par une.

### Reset des paramètres

```
$RST=*           ; restaurer tous les paramètres aux valeurs par défaut
```

Ne **pas** activer `$32=1` (mode laser) : ce mode module la PWM par la vitesse instantanée de déplacement, ce qui est sans intérêt pour un fil chaud (l'inertie thermique rend la modulation invisible) et risque au contraire de couper la chauffe pendant les ralentissements en fin de trajectoire. Garder `$32=0` (défaut, mode spindle classique) : la chauffe est commandée par `M3 S<valeur>` / `M5`.

### Drivers TMC2209 (Trinamic)

```
$150=32          ; microsteps X (32 = compromis bruit/résolution typique)
$151=32          ; microsteps Y
$152=32          ; microsteps Z (laisser même si Z non utilisé)
$153=32          ; microsteps A (4e axe)

$140=800         ; courant RMS X en mA - adapter à la datasheet du moteur
$141=800         ; courant RMS Y
$142=800         ; courant RMS Z
$143=800         ; courant RMS A
```

Garder `$140-$143` ≤ ~0.7 × le courant nominal du moteur pour limiter l'échauffement. Un NEMA17 1.5 A nominal → 800–1000 mA RMS typique.

### Cinématique

Formules à utiliser selon votre transmission :

- **Vis (leadscrew)** : `steps/mm = (motor_steps × microsteps) / pitch_mm`
  - Exemple TR8x8 (pas 8 mm/tour), moteur 200 pas/tour, 32 microsteps : `200 × 32 / 8 = 800 steps/mm`
- **Courroie (belt)** : `steps/mm = (motor_steps × microsteps) / (belt_pitch_mm × pulley_teeth)`
  - Exemple GT2 (pas 2 mm) avec poulie 20 dents, moteur 200 pas/tour, 32 microsteps : `200 × 32 / (2 × 20) = 6400 / 40 = 160 steps/mm`

L'exemple ci-dessous correspond à une configuration **courroie GT2 + poulie 20 dents**, courante en hotwire (charge légère, courroie économique et silencieuse). Recalculer pour vis, ou pour un autre nombre de dents de poulie.

```
$100=160         ; steps/mm X — courroie GT2 (2mm) × 20T × 200 pas × 32 microsteps
$101=160         ; steps/mm Y
$102=160         ; steps/mm Z
$103=160         ; steps/mm A

$110=3000        ; vitesse max X en mm/min
$111=3000        ; vitesse max Y
$112=3000        ; vitesse max Z
$113=3000        ; vitesse max A

$120=200         ; accélération X en mm/s^2
$121=200         ; accélération Y
$122=200         ; accélération Z
$123=200         ; accélération A
```

### Spindle PWM (hotwire)

```
$30=1000         ; borne supérieure du réglage possible de la puissance via M3 S...
$31=0            ; valeur S minimale
$33=200          ; fréquence PWM hotwire en Hz (200 Hz = défaut HotWire4X)
$34=0            ; duty cycle quand la chauffe est OFF (impératif : 0 %)
```

Le `$34=0` est critique en hotwire : il garantit que `M5` coupe effectivement la chauffe. Une valeur non nulle laisserait un courant résiduel dans le fil.

Test de la chauffe : `M3 S500` (S valeur sur l'échelle qui va de la valeur de `$31` à la valeur de `$30`), attendre, mesurer le courant, `M5` pour couper. Vérifier que `M5` éteint réellement le module MOSFET.

### Sauvegarder

Toutes les modifications `$xxx=` sont persistées en flash automatiquement. Vérifier l'état complet avec `$$`.

## Réactiver le probe plus tard

Dans cette variante, `PROBE_ENABLE 0` est forcé en haut de `boards/btt_skr_pico_10_map.h` car GP22 est utilisé par le bouton STOP, et le connecteur `PROBE` lui-même sert à son câblage (cf. section ci-dessus). Pour rétablir un probe :

1. **Choisir un autre GPIO libre** comme entrée probe (par exemple GP21 ex-HB). La carte ne fournit pas d'opto pour cette entrée — câbler un opto externe est recommandé en environnement bruité. (GP29 SERVO n'est plus disponible : pris par la sortie hotwire PWM. Le connecteur PROBE physique n'est plus disponible non plus, il porte désormais le bouton STOP.)
2. **Modifier le board map** : retirer la paire `#undef PROBE_ENABLE` / `#define PROBE_ENABLE 0` (en haut du fichier), et ajouter `#define AUXINPUT0_PIN <gpio_choisi>` au-dessus du bloc des AUXINPUT.
3. **Recompiler** : voir section ci-dessous.

Renoncer au bouton STOP pour récupérer GP22 comme probe est déconseillé en production.

## Recompiler après modification

Pré-requis : extension VS Code « Raspberry Pi Pico » installée (fournit le SDK 2.1.1, la toolchain `arm-none-eabi-gcc 14.2.1`, Ninja, CMake) sous `%USERPROFILE%\.pico-sdk\`.

Depuis le dossier du projet, en CMD :

```cmd
"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" -G Ninja ^
  -DCMAKE_MAKE_PROGRAM=%USERPROFILE%/.pico-sdk/ninja/v1.12.1/ninja.exe ^
  -B build -S .

"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" --build build
```

Sortie : `build\grblHAL.uf2` (~530 ko), copié en `build\grblHAL_SERVOS.uf2`. Variante HB (sortie GP21) dans un sous-dossier, lui aussi ignoré par git :

```cmd
"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" -G Ninja ^
  -DCMAKE_MAKE_PROGRAM=%USERPROFILE%/.pico-sdk/ninja/v1.12.1/ninja.exe ^
  -DHOTWIRE_HEAT=HB -B build\hb -S .

"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" --build build\hb
```

Les deux firmwares à jour sont versionnés dans `release\` (`grblHAL_SERVOS-HotWire4X.uf2`, `grblHAL_HB-HotWire4X.uf2`) — les mêmes fichiers que `firmwares\` de HotWire4X : après une recompilation, recopier les deux `.uf2` dans les deux dossiers.

Flasher en mode BOOTSEL :

```cmd
copy /B release\grblHAL_SERVOS-HotWire4X.uf2 R:\
```

(remplacer `R:` par la lettre du lecteur `RPI-RP2` qui apparaît au branchement USB en mode bootloader.)

## Liens

- grblHAL core : https://github.com/grblHAL/core
- Driver RP2040 : https://github.com/grblHAL/RP2040
- Schéma BTT SKR Pico v1.0 : https://github.com/bigtreetech/SKR-Pico
- Wiki des paramètres `$` : https://github.com/grblHAL/core/wiki/Settings

## Provenance

Branche `hotwire-servo`, basée sur la branche `master` de grblHAL/RP2040. Commits applicatifs :

```
3245145  Hotwire SERVO: document MOSFET module wiring (no series resistor)
e46ff58  Hotwire SERVO: route mandatory E-Stop input to opto-coupled GPIO 22 (WD-DET)
7cc0bb9  Hotwire SERVO variant: remap spindle PWM from HB (GPIO 21) to SERVO header (GPIO 29)
```
