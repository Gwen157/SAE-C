# Build - Space Invaders

Ce document décrit le fonctionnement du Makefile et les cibles utiles. Les instructions d’installation des dépendances sont dans INSTALL.md.

## Détection automatique
- `pkg-config` cherche ncurses (`ncursesw`) et SDL3 (`sdl3`).
- Variables définies : `HAVE_NCURSES`, `HAVE_SDL3`, `NCURSES_CFLAGS/LIBS`, `SDL_CFLAGS/LIBS`.
- Sources ajoutées selon disponibilité : vues réelles ou stubs (`view_console_stub.c`, `view_sdl_stub.c`).

## Cibles principales
- `make` / `make all` : compile ce qui est disponible et produit `build/space_invaders`.
- `make run-console` / `make run-sdl` : lance la vue console ou SDL3.
- `make check-deps` : affiche l’état des dépendances détectées et la liste des sources compilées.
- `make clean` : nettoie objets et binaire.
- `make valgrind` : exécute la vue SDL & console avec `valgrind.supp` (Linux/WSL).

## Stubs
Si une dépendance manque, la vue correspondante est remplacée par un stub qui affiche un message clair et retourne un code d’erreur, tout en laissant le binaire exécutable.


