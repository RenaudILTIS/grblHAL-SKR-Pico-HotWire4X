# grblHAL Hotwire SERVO — BTT SKR Pico v1.0

Variante `BOARD_BTT_SKR_PICO_10_HOTWIRE` de grblHAL/RP2040 pour la découpe par fil chaud (foam cutter), 4 axes, drivers TMC2209 en UART, sortie de chauffe sur le connecteur SERVO via module MOSFET externe.

## Vérification du flash

Après flash de `build/grblHAL.uf2` (mode BOOTSEL puis copie sur le lecteur RPI-RP2), la commande grblHAL `$I` doit renvoyer une ligne contenant :

```
[BOARD:BTT SKR Pico 1.0 Hotwire SERVO]
```

Si vous voyez `[BOARD:BTT SKR Pico 1.0]` (sans le suffixe « Hotwire SERVO »), c'est le firmware standard — re-flasher.

## Pinout des connecteurs utiles (sérigraphie SKR Pico v1.0)

### Moteurs (drivers TMC2209, UART)

| Axe grblHAL | Connecteur moteur | STEP | DIR | EN |
|-------------|-------------------|-----:|----:|---:|
| X           | X-MOT             | GP11 | GP10 | GP12 |
| Y           | Y-MOT             | GP6  | GP5  | GP7  |
| Z           | Z-MOT             | GP19 | GP28 | GP2  |
| A (4ᵉ axe)  | **E-MOT**         | GP14 | GP13 | GP15 |

### Fins de course

| Endstop  | Connecteur  | GPIO | Notes                     |
|----------|-------------|-----:|---------------------------|
| X        | X-STOP      | GP4  | Entrée 3 broches SIG/+5V/GND |
| Y        | Y-STOP      | GP3  | idem                      |
| Z        | Z-STOP      | GP25 | idem                      |
| A (M3)   | **E0-STOP** | GP16 | endstop du 4ᵉ axe         |

### Sorties spéciales

| Fonction              | Connecteur | GPIO | Niveau                   |
|-----------------------|------------|-----:|--------------------------|
| Hotwire PWM (spindle) | **SERVO**  | GP29 | 3.3 V CMOS               |
| Spindle enable        | FAN1       | GP17 | 3.3 V                    |
| Coolant flood         | FAN3       | GP20 | 3.3 V                    |
| Coolant mist          | HE         | GP23 | 3.3 V                    |
| GPIO libre            | HB         | GP21 | 3.3 V (sans isolation)   |

### Entrée sécurité

| Fonction | Connecteur | GPIO | Caractéristique          |
|----------|------------|-----:|--------------------------|
| E-Stop   | **WD-DET** | GP22 | Opto-couplé sur la carte |

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

## Câblage du bouton E-Stop

Topologie **fail-safe** : contact normalement fermé (NF). Libérer le bouton ou couper le câble doit déclencher le halt.

```
SKR Pico WD-DET header
+-------------+
| GP22 -------+----+
|             |    |  contact NF (normally closed)
| GND --------+----+--- bouton coup-de-poing E-Stop industriel
| +5V (libre) |
+-------------+
```

- En fonctionnement normal : le bouton est armé, le contact est fermé, GP22 est tiré à la masse à travers l'opto-coupleur.
- Sur appui (ou rupture de câble) : le contact s'ouvre, l'entrée bascule, grblHAL déclenche `Alarm:1` (Estop) et coupe la chauffe.
- Câble blindé recommandé sur les longues distances (>1 m), tresse à la masse SKR Pico côté carte uniquement.

`CONTROL_HALT` est lié à `RESET_PIN = AUXINPUT1_PIN = GP22` dans le board map ; l'entrée E-Stop est donc active dès le boot, sans configuration logicielle additionnelle.

## Premier démarrage — paramètres `$` recommandés

À adapter à votre mécanique. Connectez-vous via USB CDC (115200 bauds), puis envoyez ces commandes une par une.

### Reset des paramètres et mode de fonctionnement

```
$RST=*           ; restaurer tous les paramètres aux valeurs par défaut
$32=1            ; mode "laser/PWM dynamique" : la chauffe suit la vitesse
                 ;   de déplacement et coupe à l'arrêt (recommandé hotwire)
```

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

```
$100=80          ; steps/mm X - calculer : (200 * microsteps) / pas_de_vis_mm
$101=80          ; steps/mm Y
$102=80          ; steps/mm Z
$103=80          ; steps/mm A

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
$30=1000         ; valeur S (0..1000) à pleine puissance
$31=0            ; valeur S minimale
$33=5000         ; fréquence PWM hotwire en Hz (5 kHz = bon compromis)
```

Test de la chauffe : `M3 S500` (50 %), attendre, mesurer le courant, `M5` pour couper. Vérifier que `M5` éteint réellement le module MOSFET.

### Sauvegarder

Toutes les modifications `$xxx=` sont persistées en flash automatiquement. Vérifier l'état complet avec `$$`.

## Réactiver le probe plus tard

Dans la variante SERVO, `PROBE_ENABLE 0` est forcé dans `boards/btt_skr_pico_10_map.h` (bloc HOTWIRE) car GP22 est utilisé comme E-Stop. Pour rétablir un probe :

1. **Choisir un autre GPIO libre** comme entrée probe (par exemple GP21 ex-HB, ou un GPIO du connecteur SERVO si l'E-Stop n'est pas câblé). La carte ne fournit pas d'opto pour cette entrée — câbler un opto externe est recommandé en environnement bruité.
2. **Modifier le board map** : retirer la paire `#undef PROBE_ENABLE` / `#define PROBE_ENABLE 0` du bloc HOTWIRE, et ajouter `#define AUXINPUT0_PIN <gpio_choisi>` au-dessus du bloc des AUXINPUT.
3. **Recompiler** : voir section ci-dessous.

Renoncer à l'E-Stop matériel pour récupérer GP22 comme probe est déconseillé en production.

## Recompiler après modification

Pré-requis : extension VS Code « Raspberry Pi Pico » installée (fournit le SDK 2.1.1, la toolchain `arm-none-eabi-gcc 14.2.1`, Ninja, CMake) sous `%USERPROFILE%\.pico-sdk\`.

Depuis le dossier du projet, en CMD :

```cmd
"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" -G Ninja ^
  -DCMAKE_MAKE_PROGRAM=%USERPROFILE%/.pico-sdk/ninja/v1.12.1/ninja.exe ^
  -B build -S .

"%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin\cmake.exe" --build build
```

Sortie : `build\grblHAL.uf2` (~520 ko). Flasher en mode BOOTSEL :

```cmd
copy /B build\grblHAL.uf2 R:\
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
