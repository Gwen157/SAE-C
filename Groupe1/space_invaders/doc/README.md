# Space Invaders (C) — Vue console & SDL3

Space Invaders en C avec architecture MVC, deux vues interchangeables (console ncurses et SDL3) et sauvegarde des meilleurs scores en JSON.

## Ce que vous obtenez
- Deux interfaces : console (ncurses) et SDL3 (graphique).
- Commandes reconfigurables dans les menus Options (console et SDL3).
- Tableau des high-scores persistant (`data/highscores.json`).
- Build automatique qui active les vues disponibles (stubs sinon).

## Installer
Voir les instructions détaillées par plateforme dans [INSTALL.md](INSTALL.md).

## Compiler
- `make` : build complet, détecte SDL3/ncurses via pkg-config.
- `make check-deps` : affiche quelles vues seront compilées.
- `make clean` : nettoyage.

## Lancer
- Console : `make run-console` ou `./build/space_invaders --view=console`
- SDL3 : `make run-sdl` ou `./build/space_invaders --view=sdl`

## Contrôles (par défaut)
- Gauche/Droite : `A` / `D` ou flèches.
- Tirer : `Espace` (console : espace/entrée acceptés).
- Pause : `P` (console uniquement).
- Quitter : `Q`.
Les bindings sont modifiables dans le menu Options de chaque vue.

## Mémoire
Valgrind (Linux/WSL) : `make valgrind` — utilise `valgrind.supp` pour masquer les fuites reachables des libs.
                       `make valgrind-console` Uniquement pour Valgrind la version console
                       `make valgrind-sdl` Uniquement pour Valgrind la version SDL

## Où contribuer ensuite
- Ajouter des assets/sons en SDL3.
- Ajouter Boss
- Étendre les tests (voir [TEST.md](TEST.md)).
- Explorer l’architecture détaillée dans [ARCHITECTURE.md](ARCHITECTURE.md).

