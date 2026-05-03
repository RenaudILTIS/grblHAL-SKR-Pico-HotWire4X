# grblHAL Hotwire SERVO — BTT SKR Pico v1.0

Variante SERVO du board map `BOARD_BTT_SKR_PICO_10` de grblHAL/RP2040 pour la découpe par fil chaud (foam cutter), 4 axes, drivers TMC2209 en UART, sortie de chauffe sur le connecteur SERVO via module MOSFET externe.

> **Alimentation** : pour la mise au point complète (test des drivers TMC2209 via UART, `M122`, persistance des microsteps, etc.) il est **impératif** d'alimenter la carte par le bornier 12–24 V (`VMOT`/`GND`, à côté des fusibles), **pas par le jumper USB seul**. Sans VM, les TMC2209 ne sont pas alimentés et ne répondent pas en UART : `[NEWOPT:...]` ne contient pas `TMC=15`, et toute écriture `$15x=...` est silencieusement ignorée (la valeur lue par `$$` reste à 16).

## Vérification du flash

Après flash de `build/grblHAL.uf2` (mode BOOTSEL puis copie sur le lecteur RPI-RP2), la commande grblHAL `$I` doit renvoyer une ligne contenant :

```
[BOARD:BTT SKR Pico 1.0 SERVO]
```

Si vous voyez `[BOARD:BTT SKR Pico 1.0]` (sans le suffixe « SERVO »), c'est le firmware standard — re-flasher.

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

### Entrée sécurité

| Fonction | Connecteur | Broche signal | GPIO | Caractéristique          |
| -------- | ---------- | ------------- | ----:| ------------------------ |
| E-Stop   | **WD-DET** | `P_S`         | GP22 | Opto-couplé sur la carte |

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

L'entrée E-Stop est sur GP22, broche `P_S` du connecteur WD-DET (opto-couplée au niveau de la carte). **Câblage fail-safe obligatoire** : un bouton coup-de-poing à contact **NF (normalement fermé)** entre `P_S` et `GND`.

Brochage du connecteur WD-DET (3 broches, sérigraphie BTT SKR Pico v1.0, vérifié sur le pinout officiel `BTT SKR Pico V1.0-PIN.pdf` du dépôt BigTreeTech) :

| Broche silkscreen | Rôle                                                    |
| ----------------- | ------------------------------------------------------- |
| `12/24V`          | Alim active (capteur), non utilisée ici — laisser libre |
| `GND`             | Masse                                                   |
| `P_S`             | Signal (Probe Signal), routé en interne vers GP22       |

Câblage :

```
SKR Pico WD-DET           Bouton coup-de-poing
+-----------+             (contact NF — normalement fermé)
| 12/24V    |  (libre)
| GND   ----+-------------+
| P_S   ----+--------+    |
+-----------+        |    |
                    [bouton NF]
                  fermé au repos,
                  ouvert sur appui
```

Comportement attendu :

- **Au repos** (bouton relâché, contact **fermé**) : `P_S` est tiré à GND par le bouton. grblHAL voit l'entrée comme « pas de halt » et passe en `<Idle|...>` après `$X`.
- **Sur appui** (contact ouvert) : la ligne flotte haute (pull-up interne du Pico). grblHAL déclenche `Alarm:1` (E-Stop), coupe la chauffe et arrête tout mouvement.
- **Câble coupé ou débranché** : ligne flotte haute → comportement identique à un appui sur le bouton → halt automatique. C'est précisément ce qu'apporte le fail-safe.

`CONTROL_HALT` est lié à `RESET_PIN = AUXINPUT1_PIN = GP22` dans le board map ; l'entrée E-Stop est donc active dès le boot, sans configuration logicielle additionnelle.

### Test de validation (avant câblage du bouton définitif)

Court-circuiter `P_S` à `GND` avec un fil jumper, faire un soft reset (`Ctrl+X`), puis :

```
$X       → doit retourner [MSG:Caution: Unlocked] (et non error:79)
M122     → rapport TMC complet
```

Retirer le pont → l'alarme E-Stop doit revenir au prochain reset. Si c'est conforme, câbler le bouton NF définitif et tu peux ignorer toute configuration de polarité.

### Bouton NO ou polarité inversée

Si tu utilises malgré tout un bouton NO (normalement ouvert), ou si la chaîne est inversée pour une raison quelconque, la polarité du signal E-Stop peut être inversée via `$14=64` (bit 6 = E-Stop). Cette option sera exposée dans l'interface logicielle (case « NO/NC ») — il n'est en principe pas nécessaire de la régler manuellement en CLI.

> **Avertissement** : un câblage NO n'est **pas fail-safe**. Un câble coupé ou débranché ne sera pas détecté (la ligne reste à son état "OK"). À éviter en production, surtout sur une machine industrielle ou éloignée de l'opérateur.

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
$33=5000         ; fréquence PWM hotwire en Hz (5 kHz = bon compromis)
$34=0            ; duty cycle quand la chauffe est OFF (impératif : 0 %)
```

Le `$34=0` est critique en hotwire : il garantit que `M5` coupe effectivement la chauffe. Une valeur non nulle laisserait un courant résiduel dans le fil.

Test de la chauffe : `M3 S500` (S valeur sur l'échelle qui va de la valeur de `$31` à la valeur de `$30`), attendre, mesurer le courant, `M5` pour couper. Vérifier que `M5` éteint réellement le module MOSFET.

### Sauvegarder

Toutes les modifications `$xxx=` sont persistées en flash automatiquement. Vérifier l'état complet avec `$$`.

## Réactiver le probe plus tard

Dans cette variante, `PROBE_ENABLE 0` est forcé en haut de `boards/btt_skr_pico_10_map.h` car GP22 est utilisé comme E-Stop. Pour rétablir un probe :

1. **Choisir un autre GPIO libre** comme entrée probe (par exemple GP21 ex-HB). La carte ne fournit pas d'opto pour cette entrée — câbler un opto externe est recommandé en environnement bruité. (GP29 SERVO n'est plus disponible : pris par la sortie hotwire PWM.)
2. **Modifier le board map** : retirer la paire `#undef PROBE_ENABLE` / `#define PROBE_ENABLE 0` (en haut du fichier), et ajouter `#define AUXINPUT0_PIN <gpio_choisi>` au-dessus du bloc des AUXINPUT.
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
