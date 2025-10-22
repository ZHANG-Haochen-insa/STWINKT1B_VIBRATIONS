# TP : Implémentation d'une solution de « Machine Learning » dédiée à la maintenance prédictive

## Objectif

Ce TP vise à établir une « solution d'intelligence artificielle » permettant la détection d'anomalies lors des phases de fonctionnement d'un moteur électrique.

### Les principaux points abordés

- ➢ l'analyse de la chaîne d'instrumentation
- ➢ l'établissement d'un « dataset » de données
- ➢ la génération et le choix d'une « fonction d'inférence »
- ➢ l'implémentation et le test de la solution établie sur cible

### Description du projet

Le TP va consister à monitorer la vitesse du moteur électrique d'un ventilateur. La solution d'IA devra être en mesure de détecter en temps réels si le moteur est dans un régime de fonctionnement « normal » ou « anormal », tout en l'indiquant à l'utilisateur.

**Remarque** : en fonction du temps, il pourra être envisagé la mise en œuvre de solutions plus complexe intégrant par exemple de la classification permettant de détecter plusieurs régimes de fonctionnement.

**IMPORTANT** : Pour que le TP fonctionne, il est nécessaire de travailler uniquement sur `C:\TEMP` (projet et fichiers de données)

---

## A) Présentation de l'équipement

### a) Partie « hardware »

La maquette de travail est constituée d'un ventilateur d'ordinateur (contenant un moteur électrique) et d'une plateforme d'instrumentation.

#### Plateforme d'instrumentation
- **Référence** : STEVAL-STWINKT1B
- **Fabricant** : STMicroelectronics
- **Composants** :
  - Capteurs multiples
  - MCU de type STM32 (cœur du système d'instrumentation)
  - Carte fille STLINK-V3MINI (débogueur/programmateur)

#### Configuration du moteur

Le moteur peut être configuré par l'utilisateur avec 3 valeurs de vitesse possibles. Ses caractéristiques lui permettent de fonctionner entre 700 tr/min et 1600 tr/min :

- ✓ **O** : le moteur est éteint
- ✓ **L** : le moteur tourne en faible vitesse (1000 tr/min)
- ✓ **M** : le moteur tourne en moyenne vitesse (1250 tr/min)
- ✓ **H** : le moteur tourne en forte vitesse (1500 tr/min)

La plateforme d'instrumentation a été collée sur le haut du ventilateur de façon à faciliter l'instrumentation du moteur.

### b) Partie « software »

Ce TP va impliquer l'utilisation de plusieurs logiciels :

#### STM32Cube IDE
Il s'agit de l'interface fournie par STMicroelectronics pour programmer/déboguer les MCU de type STM32.

#### NanoEdge AI Studio
Ce logiciel dédié à l'IA pour MCU de type STM32 est édité par STMicroelectronics. Il sera utilisé par les parties du TP liés au machine learning. Son rôle principal est d'évaluer l'algorithme le mieux approprié à la problématique et d'établir les « bibliothèques d'IA » à implémenter sur la cible.

#### Putty
Il permettra de monitorer les échanges de données d'une liaison série entre des terminaux.

**Remarque** : la grande force de NanoEdge AI Studio est de pouvoir proposer des fonctions « IA » d'implémentation sur un MCU de type STM32. Cela signifie qu'il est possible d'ajouter facilement des fonctions intelligentes à applications/dispositifs courants basés sur un MCU.

---

## B) Mise en œuvre de la solution

### Partie 1 : Configuration de la plateforme

Comme vous l'avez appris, l'utilisation d'outils dépendants du « machine learning » nécessite l'emploi d'une certaine quantité de « données » pour obtenir des résultats viables.

#### Questions préliminaires

➢ Selon vous quelle(s) grandeur(s) allons-nous chercher à monitorer pour suivre l'état de santé du moteur ?

#### Vérification des connexions

➢ Vérifiez la bonne connexion de la maquette. Il doit y avoir 3 liaisons USB :
- 1 liaison USB entre le PC et la carte STLINK (face arrière du PC)
- 1 liaison USB entre le PC et la carte STWIN (face arrière du PC)
- 1 liaison USB entre le PC et le ventilateur

**Remarque** : Au cours des premières séances de TP, il est apparu des problèmes dans la lecture des données (trop de bruit). En connectant les fils USB du STLINK et du STWIN sur la face arrière du PC, nous avons pu observer une amélioration de la qualité des signaux reçus.

#### Configuration du projet

Pour établir la fonction d'inférence visée dans le cadre de notre application, il va être nécessaire d'établir un jeu de données via des mesures expérimentales sur la maquette.

➢ Télécharger sous « moodle », le dossier portant le nom « TP_5GE_Nano_edge »

➢ Après avoir extrait le contenu de ce dossier, ouvrez le dossier « STWINKT1B_VIBRATIONS », puis ouvrez avec le logiciel STM32Cube IDE, le fichier portant l'extension « .project ».

Le wiki : https://wiki.st.com/stm32mpu/wiki/STM32CubeIDE facilitera la prise en main de ce logiciel.

➢ Une fois le projet ouvert, vous pouvez naviguer dans les différents dossiers. Le code principal se trouve dans le fichier « main.c ».

Vous pourrez le trouver dans l'arborescence via : **Core > SRC > main.c**

#### Configuration de la communication

La plateforme intègre une émulation d'une liaison série via le port USB. Il est ainsi possible de collecter des données via le Port COM de l'ordinateur et d'un logiciel spécifique (Putty pour ce TP).

L'échange de données entre le port COM de l'ordinateur et la plateforme peut s'effectuer au moyen de l'HUART (STLINK) ou de l'USB_CDC (USB on STWIN). Dans le cadre de ce TP, cela sera réalisé uniquement au moyen de l'HUART.

➢ Repérez la ligne de code associée à cette opération dans le fichier « main.c ». Procédez à une modification si cela s'avère nécessaire.

➢ Vérifiez que la variable `NEAI_MODE` soit imposée à 0.

#### Compilation et implémentation

Avant de passer à la partie « collecte », vous devez compiler le projet et l'implémenter sur le STM32 de la carte.

**(privilégier le « RUN AS »)**

En cas de difficultés, vous pouvez vous aider du manuel utilisateur (section 3) présent dans le dossier « Documentations » du fichier zip téléchargé sous moodle dans le cadre du TP.

**Si STM32CubeIDE reflash l'ancien .elf, sans rebuilder, faire un « Project > Build All » puis Run AS.**

---

### Partie 2 : Constitution du « Dataset »

#### Rappel

L'objectif principalement visé est de pouvoir détecter et indiquer la présence d'une anomalie lors du fonctionnement d'un moteur.

#### Collecte des données

Dans le cadre de la solution utilisée, il va falloir procéder à la collecte de deux jeux de données :

- **Le premier jeu de données** devra contenir uniquement des données liées à un fonctionnement normal du moteur
- **Le second jeu de données** devra contenir uniquement des données liées à un fonctionnement anormal (ex : perturbations des pâles lors de la rotation, mise en place d'un trombone sur une pâle…)

**Les 2 jeux de données devront être sauvegardés dans deux fichiers distincts (un fichier par type de fonctionnement).**

#### Méthodes de collecte

Pour la collecte des données, vous avez la possibilité d'utiliser une des deux méthodes présentées ci-dessous :

**Remarque importante** : Lors des premières séances de TPs, nous avons identifié un problème sur la qualité de signaux collectés par le PC. Pour information, ce problème n'apparaissait pas sur le PC où a été développé le TP. Il semblerait que le PC engendre des parasites venant perturber la qualité des signaux.

Il est recommandé d'utiliser au moins une ou deux fois la méthode 1 pour définir le dataset. Si la qualité des signaux s'avère être médiocre lors de la phase de traitement, il faudra alors procéder à une collecte de données via la méthode 2.

#### ▪ Méthode 1 : importation de données par un fichier

Cette méthode vise à collecter des données et à les sauvegarder dans un ou plusieurs fichiers selon l'application visée. Les données seront par la suite importées au niveau de l'application métier. Le logiciel « Putty » est un outil permettant de réaliser cette opération de collecte.

➢ Allez dans le gestionnaire des périphériques (par la barre de recherche windows) et cherchez le port COM indiquant « STMicroelectronics STLINK Virtual COM Port » (dans l'exemple -> Port COM 18)

➢ Ouvrez « Putty » et procédez à la configuration de la liaison série comme cela est indiqué ci-dessous, puis ouvrez la transmission en cliquant sur « open ».

**Configuration Putty (onglet Session)** :
- Serial line : [Port COM correspondant]
- Speed : 115200
- Connection type : Serial

**Configuration Putty (onglet Connection/Serial)** :
- Serial line to connect to : COM18 (adapter selon votre configuration)
- Speed (baud) : 115200
- Data bits : 8
- Stop bits : 1
- Parity : None
- Flow control : XON/XOFF

**Remarque** : pensez bien à adapter la valeur du Port COM avec celle indiquée par le gestionnaire de périphériques.

Vous devez normalement voir des données s'afficher en permanence sur la fenêtre de transmission. Le logiciel « Putty » permet de sauvegarder les données reçues dans un fichier.

➢ Procédez à la collecte de données expérimentales du moteur. N'oubliez pas qu'il faut deux fichiers distincts pour les données (utilisez la méthode que vous souhaitez).

Les fichiers seront par la suite importés sous Nanoedge en vue d'établir l'algorithme IA (Partie 3 du TP : phase 2 et 3).

#### ▪ Méthode 2 : collecte de données en live par USB

Cette méthode va consister à réaliser la collecte de données directement via le logiciel « Nanoedge ». Contrairement à la « méthode 1 », qui doit être réalisée en amont de l'utilisation du studio Nanoedge. Cette méthode vise à collecter les données au cours du process visant la création de la fonction d'inférence, qui est présenté à la partie 3 du TP.

La collecte permettra de définir le « dataset » de données et elle s'effectuera en deux étapes (2 et 3) :
- ✓ la phase 2 permettra de collecter les données pour le fonctionnement normal
- ✓ la phase 3 permettra de collecter les données pour le fonctionnement anormal

➢ Passez à la partie 3 du TP et suivez la procédure liée à l'utilisation de « Nanoedge ».

Aux étapes 2 et 3 de cette procédure, une fenêtre apparaitra. Sélectionnez **« From Serial (USB) »** pour pouvoir effectuer la collecte par USB.

Une fois le choix effectué, une fenêtre de configuration apparaîtra. Il vous suffira configurer la connexion avec les bons paramètres (Port COM et vitesse à 115200), puis de procéder à la collecte des données.

**Remarque** : lors des différentes expérimentations durant les premières séances de TP, il est apparu que la méthode 2 donnait de meilleurs résultats que la méthode 1.

---

### Partie 3 : Génération de la fonction d'inférence

Une fois le jeu de données constitué, il va pouvoir être utilisé pour établir la fonction d'inférence.

➢ Ouvrez le logiciel « NanoEdge AI Studio »

Ce logiciel offre la possibilité de travailler selon différents types de projets : détection d'anomalie, classification, extrapolation…

➢ Dans le cadre de ce TP, nous prendrons le type « détection d'anomalie ». Suivez la procédure proposée par le logiciel.

#### Les 5 phases de NanoEdge AI Studio

**Phase 1 : Project Settings** - Configuration du projet
**Phase 2 : Signals** - Acquisition des signaux
**Phase 3 : Benchmark** - Évaluation des algorithmes
**Phase 4 : Validation** - Test et validation
**Phase 5 : Deployment** - Déploiement

#### • Phase 1 : Configuration

La phase 1 est une phase de configuration. Pour le type de capteur, il est primordial de choisir **« Accelerometer 3 axes »**.

#### • Phase 2 : Acquisition de données

La phase 2 concerne l'acquisition de données. Comme cela a été évoqué précédemment, vous aurez le choix entre plusieurs « méthodes » tels que l'importation de données via des fichiers ou alors d'effectuer une collecte en temps réels par USB (voir : la partie 2 du document de TP).

**Format des données** :
- ✓ Les signaux doivent présenter un format particulier. Le buffer de chaque capteur doit contenir 256 valeurs par lignes (si 3 capteurs : 768 valeurs par ligne).
- ✓ Il peut arriver que certaines valeurs ne soient pas conformes (ex : présence de 2 « . » pour une même donnée). Nous vous conseillons soit de les supprimer si cela engendre peu d'impact, soit de corriger le problème à la main dans le fichier au moyen d'un logiciel comme « NotePad ». Le formatage des données est une étape importante en « machine learning ».
- ✓ Il est possible d'intégrer N lignes de données. Il est recommandé d'avoir un minimum de 50 lignes. Pour assurer une bonne robustesse de la solution proposée, il est important d'avoir une certaine quantité de données (et de qualité) à disposition.
- ✓ La taille du buffer doit être une puissance de 2.
- ✓ Si vous utilisez un log de Putty, il faudra convertir les données en CSV via Excel, en respectant 768 colonnes par ligne.

**Format d'exemple** :
```
Data format example:
n buffers of 256 values x 3 axes (x,y,z) with space separator

line 1:  X₀ Y₀ Z₀ X₁ Y₁ Z₁ (...) X₂₅₅ Y₂₅₅ Z₂₅₅
line 2:  X₀ Y₀ Z₀ X₁ Y₁ Z₁ (...) X₂₅₅ Y₂₅₅ Z₂₅₅
(...)
line n:  X₀ Y₀ Z₀ X₁ Y₁ Z₁ (...) X₂₅₅ Y₂₅₅ Z₂₅₅
```

**Remarque** : Les méthodes de collecte donnent normalement le même résultat. Comme nous l'avons indiqué précédemment, il apparait un problème de bruit sur l'installation actuelle du TP venant perturber la qualité des données collectées par la méthode 1. Si cela s'avère trop complexe, procéder à une collecte de données par voie USB (méthode 2).

**⚠️ Ne pas toucher à la table pendant l'acquisition des données ! (Vibrations)**

#### • Phase 3 : Benchmark

La phase 3 correspond au « benchmark ». Il s'agit d'une étape où le logiciel procède à la détermination de l'algorithme le mieux adapté à la problématique (jeux de données). Cette étape étant longue (estimation 15min). Vous pouvez stopper le benchmark quand **« Quality index » dépasse 98**.

➢ Expliquez pourquoi il est inutile d'atteindre le score de 100 sur l'index.

Une fois le « benchmark » terminé ou stoppé :

➢ Regardez le tableau de résultat et commentez les différents paramètres.

➢ Sur la fenêtre de droite, vous avez une ligne nommée « Result » indiquant « N » libraries.

#### • Phase 4 : Validation et émulation

La phase 4 est une phase d'émulation et du choix de la librairie. Cela signifie que l'algorithme peut être testé sans être implémenté sur cible. Il s'agit d'un bon moyen pour évaluer sa robustesse.

✓ Cliquez sur les librairies et commentez le tableau ouvert.

Vous pouvez le tester : en utilisant des fichiers de données ou directement via la transmission des données en temps réels de la plateforme (sélectionner « From Serial (USB) »).

Lors de la mise en œuvre (émulation et implémentation), vous observerez la présence de deux phases :
- ✓ la première sera une phase d'apprentissage (N mesures lors du démarrage)
- ✓ la seconde une phase de mise en service (opérationnelle)

La phase d'apprentissage est une sorte de « calibration ». La solution apprend en fonction des données reçues.

**Expérimentations à réaliser** :

➢ Lors de cette phase d'apprentissage, faites-en sorte de fortement varier les données appliquées (données normales, données d'anomalies…), puis testez la robustesse de l'algorithme en opérationnelle.

➢ Faites la même chose uniquement avec des données d'anomalies lors de la phase d'apprentissage, puis uniquement avec des données de fonctionnement normal.

➢ Comparez les résultats de robustesse lors des phases opérationnelles, puis concluez.

**⚠️ IMPORTANT** : Lors de la phase d'apprentissage, il est impératif que les données soumises au STM32 soient uniquement du type normal (pas de données d'anomalies). Dans le cas contraire, cela altèrerait fortement la robustesse de la fonction.

Nous vous invitons à justement tester ces différents cas lors de la phase d'implémentation, puis à comparer sa robustesse.

#### • Phase 5 : Déploiement

La phase 5 est l'étape ultime. Elle va générer une bibliothèque. Ces fichiers devront être collés dans le projet ouvert sur STM32Cube IDE. Il faudra remplacer les fichiers déjà présents par ces nouveaux fichiers.

Il sera aussi nécessaire d'apporter une modification dans le fichier « main.c ». Lors de cette phase, le dispositif ne sera plus en collecte de données, mais fonctionnera en « smart-monitoring » (NEAI). Le paramètre de `NEAI_MODE` devra être passé de 0 à 1.

```c
#define NEAI_MODE 1  /* 0: Datalogging mode | 1: NEAI functions mode */
```

➢ Recompilez et implémentez le nouveau code sur le STM32.

➢ Si STM32CubeIDE reflash l'ancien .elf, sans rebuilder, faire un « Project > Build All » puis Run AS.

➢ Testez le fonctionnement en réel sur la maquette (vous aurez deux phases comme lors de l'étape 5).

En fonction du temps, nous vous invitons à tester d'autres librairies proposées lors de l'étape 4 (Benchmark) et à comparer la robustesse de ces différentes fonctions.

Vous pouvez aussi apporter des évolutions dans le fichier « main.c » (ex : états de LEDs en fonction de la présence ou d'une anomalie).

---

### Partie 4 : Analyse de la chaîne d'instrumentation

En vous aidant de la « datasheet » du « STWIN », ainsi que par l'analyse du fichier « main.c »

#### Questions d'analyse

➢ Faites un schéma « simplifié » de la chaîne d'instrumentation

➢ Quelle est la référence du capteur ?

➢ Comment les données sont-elles transmises entre le capteur et le STM32 ?

➢ Quel composant procède à l'échantillonnage du signal monitoré ?

➢ Quelle est la valeur de la fréquence d'échantillonnage configurée ?

➢ La valeur de cette fréquence est-elle adaptée à l'application actuelle ?

➢ Quelle est la valeur de la sensibilité du capteur ?

➢ Ce paramètre est-il adapté ? Quelles valeurs est-il possible de configurer ? Que se passerait-il par rapport à la configuration actuelle, si une valeur plus grande est choisie (ex : 8G ?)

➢ Quelle(s) sont les valeur(s) de la fréquence d'échantillonnage que nous pouvons appliquer ? Que devons-nous respecter ?

#### Importance de la fréquence d'échantillonnage

Le choix de la fréquence d'échantillonnage est primordial dans ce type d'application. La fonction d'inférence est élaborée à partir de données collectées. Si les données ne sont pas « fiables », cela ne peut conduire qu'à un algorithme inadapté au problème.

Il est intéressant de pouvoir observer l'impact de la fréquence d'échantillonnage sur la collecte des données, mais aussi sur la qualité de la librairie IA générée par le logiciel NanoEdge.

#### Expérimentation sur la fréquence d'échantillonnage

➢ Conservez le jeu de données précédemment collecté

➢ Essayez d'obtenir un jeu de données relativement proche (normal et anormal), mais pour une fréquence d'échantillonnage de 16 Hz

➢ À partir de ce nouveau dataset, élaborez à nouveau une librairie IA via Nanoedge, puis testez-la sur la cible.

➢ Comparez la performance des résultats notamment la robustesse entre les deux librairies générées, puis concluez.

---

**Maj 09/2025**
