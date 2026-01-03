# Tests - Space Invaders

Objectif : vérifier rapidement les fonctionnalités principales sur les deux vues sans dupliquer les guides d’installation ou de build.

## Lancer les vues
- Console : `make run-console` ou `./build/space_invaders --view=console`
- SDL3 : `make run-sdl` ou `./build/space_invaders --view=sdl`

## Scénarios recommandés
- **Déplacements & tir** : Gauche/Droite puis Tir ; l’ennemi touché disparaît, le score augmente.
- **Tirs ennemis** : Attendre les projectiles adverses, vérifier la perte de vie à l’impact.
- **Pause (console)** : Taper `P`, vérifier le gel puis la reprise.
- **Game over / restart** : Perdre toutes les vies, choisir `r` pour relancer ou `q` pour quitter.
- **High-scores** : Finir avec un score top 5, saisir un nom, relancer le jeu et ouvrir “Meilleurs scores” pour voir la persistance (JSON).
- **Options** : Dans chaque vue, remapper une touche (ex. Tir) et vérifier en jeu.

## Points d’affichage à contrôler
- Console : couleurs si disponibles, rafraîchissement uniquement sur changement, HUD réduit (Score/Vies/Level).
- SDL3 : fenêtre 800×600 redimensionnable, police bitmap lisible, pause en overlay.

## Valgrind (Linux/WSL)
`make valgrind` — utilise `valgrind.supp` pour ignorer les fuites « reachable » de SDL/ncurses/Mesa. Investiguer seulement les « definitely » ou « indirectly lost ».

## Dépannage rapide
- Vue absente → `make check-deps` et installer la lib manquante (voir [INSTALL.md](INSTALL.md)).
- Entrée clavier qui ne répond pas → vérifier les bindings dans le menu Options, inclure Entrée/Espace pour le tir en console.


