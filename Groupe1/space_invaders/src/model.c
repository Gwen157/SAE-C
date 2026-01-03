/*
 * model.c
 * -------
 * Implémentation du modèle (EtatJeu) pour le petit jeu Space Invaders.
 * Contient la représentation interne des ennemis et projectiles,
 * la logique de mise à jour (déplacements, collisions, tirs ennemis),
 * et des accesseurs en lecture seule utilisés par les vues.
 *
 * L'API publique est exposée dans `include/model.h` et ce fichier
 * ne dépend d'aucune bibliothèque d'interface utilisateur.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "model.h"

/* Structure de base : entité avec propriétés communes */
typedef struct {
    int x, y;
    int vivant;
    int sante; /* points de vie */
    int dmg;   /* points de dégâts */
    int type;  /* 0=joueur, 1=ennemi faible, 2=ennemi fort, 3=bouclier */
} Entite;

/* Particule d'explosion */
typedef struct {
    int x, y;
    int vx, vy; /* vélocité */
    int ttl;    /* time to live en frames */
    int type;   /* type d'entité qui a explosé */
} Particule;

/* Constantes pour les types d'entités */
#define TYPE_JOUEUR 0
#define TYPE_ENNEMI_FAIBLE 1
#define TYPE_ENNEMI_FORT 2
#define TYPE_BOUCLIER 3

#define NB_MAX_PARTICULES 256

/* Structures internes spécialisées */
typedef struct {
    Entite entite;
} Ennemi;

typedef struct {
    int x, y;
    int dy; /* -1 vers le haut, +1 vers le bas */
    int proprietaire; /* 0 = joueur, 1 = ennemi */
    int actif;
} Projectile;

typedef struct {
    Entite entite;
} Bouclier;

typedef struct {
    Entite entite;
} Joueur;

/* Etat interne du jeu */
struct EtatJeu {
    int largeur;
    int hauteur;
    Joueur joueur;
    int vies;
    int score;
    int niveau;
    double temps_acc;
    int quitter;
    int game_over; /* 1 si le joueur est mort */

    /* ennemis / projectiles */
    Ennemi ennemis[NB_MAX_ENNEMIS];
    int nombre_ennemis;
    int direction_ennemis; /* +1 droite, -1 gauche */
    double acc_deplacement_ennemis;
    double intervalle_deplacement_ennemis;

    Projectile projectiles[NB_MAX_PROJECTILES];
    int nombre_projectiles;
    
    /* boucliers */
    Bouclier boucliers[NB_MAX_BOUCLIERS];
    int nombre_boucliers;
    
    /* particules d'explosion */
    Particule particules[NB_MAX_PARTICULES];
    int nombre_particules;
};

/* Ligne logique du vaisseau (utilisée pour collisions et condition de défaite) */
static int ligne_vaisseau(const EtatJeu* e) {
    return e ? e->hauteur - 2 : 0;
}

/* Ajoute un projectile au tableau interne si possible. */
static void ajouter_projectile(EtatJeu* e, int x, int y, int dy, int proprietaire) {
    if (!e) return;
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) {
        if (!e->projectiles[i].actif) {
            e->projectiles[i].actif = 1;
            e->projectiles[i].x = x;
            e->projectiles[i].y = y;
            e->projectiles[i].dy = dy;
            e->projectiles[i].proprietaire = proprietaire;
            if (i >= e->nombre_projectiles) e->nombre_projectiles = i+1;
            return;
        }
    }
}

/* Crée une explosion à une position avec des particules */
static void creer_explosion(EtatJeu* e, int x, int y, int type) {
    if (!e) return;
    /* Créer 8 particules en étoile */
    int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    for (int i = 0; i < 8 && e->nombre_particules < NB_MAX_PARTICULES; ++i) {
        int idx = -1;
        for (int j = 0; j < NB_MAX_PARTICULES; ++j) {
            if (e->particules[j].ttl <= 0) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;
        e->particules[idx].x = x;
        e->particules[idx].y = y;
        e->particules[idx].vx = directions[i][0] * 2;
        e->particules[idx].vy = directions[i][1] * 2;
        e->particules[idx].ttl = 20; /* 20 frames de vie */
        e->particules[idx].type = type;
        if (idx >= e->nombre_particules) e->nombre_particules = idx + 1;
    }
}

EtatJeu* etatjeu_creer(int largeur, int hauteur) {
    EtatJeu* e = (EtatJeu*)malloc(sizeof(EtatJeu));
    if (!e) return NULL;
    e->largeur = largeur;
    e->hauteur = hauteur;
    e->joueur.entite.x = largeur / 2;
    e->joueur.entite.y = hauteur - 1;
    e->joueur.entite.vivant = 1;
    e->joueur.entite.sante = 1;
    e->joueur.entite.dmg = 1;
    e->joueur.entite.type = TYPE_JOUEUR;
    e->vies = 3;
    e->score = 0;
    e->niveau = 1;
    e->temps_acc = 0.0;
    e->quitter = 0;
    e->game_over = 0;

    /* initialisation des ennemis en grille simple */
    e->nombre_ennemis = 0;
    e->direction_ennemis = 1;
    e->acc_deplacement_ennemis = 0.0;
    e->intervalle_deplacement_ennemis = 0.6; /* secondes */
    int lignes = 3;
    int colonnes = 8;
    int start_y = 2;
    int espacement_x = (largeur - 4) / colonnes;
    if (espacement_x < 2) espacement_x = 2;
    for (int r = 0; r < lignes; ++r) {
        for (int c = 0; c < colonnes; ++c) {
            int idx = e->nombre_ennemis++;
            if (idx >= NB_MAX_ENNEMIS) break;
            e->ennemis[idx].entite.vivant = 1;
            e->ennemis[idx].entite.x = 2 + c * espacement_x;
            e->ennemis[idx].entite.y = start_y + r*2;
            e->ennemis[idx].entite.dmg = 1;
            /* Santé basée sur le niveau : niveau 1=1, niveau 2+=1 ou 2 */
            e->ennemis[idx].entite.sante = 1;
            e->ennemis[idx].entite.type = TYPE_ENNEMI_FAIBLE;
            if (e->niveau >= 2) {
                /* 25% des ennemis ont 2 de santé au niveau 2, plus au niveau 3+ */
                int pourcentage = 25 + (e->niveau - 2) * 15; /* 25, 40, 55... % */
                if (rand() % 100 < pourcentage) {
                    e->ennemis[idx].entite.sante = 2;
                    e->ennemis[idx].entite.type = TYPE_ENNEMI_FORT;
                }
            }
        }
    }

    /* initialisation des projectiles */
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) e->projectiles[i].actif = 0;
    e->nombre_projectiles = 0;

    /* initialisation des boucliers (4 positions) */
    e->nombre_boucliers = 4;
    int espacement_boucliers = largeur / 5;
    for (int i = 0; i < NB_MAX_BOUCLIERS; ++i) {
        e->boucliers[i].entite.vivant = 1;
        e->boucliers[i].entite.x = espacement_boucliers * (i + 1);
        e->boucliers[i].entite.y = hauteur / 2;
        e->boucliers[i].entite.sante = 3; /* 3 coups pour détruire */
        e->boucliers[i].entite.dmg = 0; /* les boucliers ne font pas de dégâts */
        e->boucliers[i].entite.type = TYPE_BOUCLIER;
    }

    /* initialisation des particules */
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) e->particules[i].ttl = 0;
    e->nombre_particules = 0;

    /* initialisation RNG pour tirs ennemis */
    srand((unsigned int)time(NULL));

    return e;
}

void etatjeu_detruire(EtatJeu* e) {
    if (!e) return;
    free(e);
}

static int nombre_ennemis_vivants(EtatJeu* e) {
    int c = 0;
    for (int i = 0; i < e->nombre_ennemis; ++i) if (e->ennemis[i].entite.vivant) ++c;
    return c;
}

void etatjeu_mettre_a_jour(EtatJeu* e, double dt) {
    if (!e) return;
    e->temps_acc += dt;

    /* Mise à jour des particules d'explosion */
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) {
        if (e->particules[i].ttl <= 0) continue;
        e->particules[i].x += e->particules[i].vx;
        e->particules[i].y += e->particules[i].vy;
        e->particules[i].ttl -= 1;
    }

    /* Déplacement des projectiles */
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) {
        if (!e->projectiles[i].actif) continue;
        e->projectiles[i].y += e->projectiles[i].dy;
        /* hors limites */
        if (e->projectiles[i].y < 0 || e->projectiles[i].y >= e->hauteur) {
            e->projectiles[i].actif = 0;
            continue;
        }

        if (e->projectiles[i].proprietaire == 0) {
            /* projectile joueur : collision avec ennemis */
            for (int enn = 0; enn < e->nombre_ennemis; ++enn) {
                if (!e->ennemis[enn].entite.vivant) continue;
                if (e->ennemis[enn].entite.x == e->projectiles[i].x && e->ennemis[enn].entite.y == e->projectiles[i].y) {
                    e->projectiles[i].actif = 0;
                    e->ennemis[enn].entite.sante -= 1;
                    if (e->ennemis[enn].entite.sante <= 0) {
                        creer_explosion(e, e->ennemis[enn].entite.x, e->ennemis[enn].entite.y, e->ennemis[enn].entite.type);
                        e->ennemis[enn].entite.vivant = 0;
                        e->score += 10; /* ou 20 si sante était 2 ? */
                    }
                    break;
                }
            }
            
            /* projectile joueur : collision avec boucliers */
            for (int b = 0; b < e->nombre_boucliers; ++b) {
                if (!e->boucliers[b].entite.vivant) continue;
                if (e->boucliers[b].entite.x == e->projectiles[i].x && e->boucliers[b].entite.y == e->projectiles[i].y) {
                    e->projectiles[i].actif = 0;
                    e->boucliers[b].entite.sante -= 1;
                    if (e->boucliers[b].entite.sante <= 0) {
                        creer_explosion(e, e->boucliers[b].entite.x, e->boucliers[b].entite.y, e->boucliers[b].entite.type);
                        e->boucliers[b].entite.vivant = 0;
                    }
                    break;
                }
            }
        } else {
            /* projectile ennemi : collision avec vaisseau */
            int y_vaisseau = ligne_vaisseau(e);
            if (e->projectiles[i].x == e->joueur.entite.x && e->projectiles[i].y == y_vaisseau) {
                e->projectiles[i].actif = 0;
                e->vies -= 1;
                if (e->vies <= 0) e->game_over = 1;
            }
            
            /* projectile ennemi : collision avec boucliers */
            for (int b = 0; b < e->nombre_boucliers; ++b) {
                if (!e->boucliers[b].entite.vivant) continue;
                if (e->boucliers[b].entite.x == e->projectiles[i].x && e->boucliers[b].entite.y == e->projectiles[i].y) {
                    e->projectiles[i].actif = 0;
                    e->boucliers[b].entite.sante -= 1;
                    if (e->boucliers[b].entite.sante <= 0) {
                        creer_explosion(e, e->boucliers[b].entite.x, e->boucliers[b].entite.y, e->boucliers[b].entite.type);
                        e->boucliers[b].entite.vivant = 0;
                    }
                    break;
                }
            }
        }
    }

    /* Déplacement des ennemis selon un intervalle */
    e->acc_deplacement_ennemis += dt;
    int vivants = nombre_ennemis_vivants(e);
    if (vivants == 0) {
        /* niveau vidé : réapparition un peu plus rapide et augmenter la difficulté */
        e->niveau += 1;
        e->intervalle_deplacement_ennemis *= 0.9; /* accélère un peu */
        
        int lignes = 3;
        int colonnes = 8;
        int start_y = 2;
        int espacement_x = (e->largeur - 4) / colonnes;
        if (espacement_x < 2) espacement_x = 2;
        e->nombre_ennemis = 0;
        for (int r = 0; r < lignes; ++r) {
            for (int c = 0; c < colonnes; ++c) {
                int idx = e->nombre_ennemis++;
                if (idx >= NB_MAX_ENNEMIS) break;
                e->ennemis[idx].entite.vivant = 1;
                e->ennemis[idx].entite.x = 2 + c * espacement_x;
                e->ennemis[idx].entite.y = start_y + r*2;
                e->ennemis[idx].entite.dmg = 1;
                /* Santé basée sur le niveau */
                e->ennemis[idx].entite.sante = 1;
                if (e->niveau >= 2) {
                    int pourcentage = 25 + (e->niveau - 2) * 15;
                    if (rand() % 100 < pourcentage) e->ennemis[idx].entite.sante = 2;
                }
            }
        }
    }

    if (e->acc_deplacement_ennemis >= e->intervalle_deplacement_ennemis) {
        e->acc_deplacement_ennemis = 0.0;
        /* tentative de déplacement horizontal */
        int touche_bord = 0;
        for (int i = 0; i < e->nombre_ennemis; ++i) {
            if (!e->ennemis[i].entite.vivant) continue;
            int nx = e->ennemis[i].entite.x + e->direction_ennemis;
            if (nx < 0 || nx >= e->largeur) { touche_bord = 1; break; }
        }
        if (touche_bord) {
            /* change de direction et descend */
            e->direction_ennemis = -e->direction_ennemis;
            for (int i = 0; i < e->nombre_ennemis; ++i) if (e->ennemis[i].entite.vivant) e->ennemis[i].entite.y += 1;
        } else {
            for (int i = 0; i < e->nombre_ennemis; ++i) if (e->ennemis[i].entite.vivant) e->ennemis[i].entite.x += e->direction_ennemis;
        }
    }

    /* Tir ennemi : petite probabilité aléatoire */
    if (rand() % 100 < 4) { /* ~4% par tick */
        int idxs[NB_MAX_ENNEMIS]; int n = 0;
        for (int i = 0; i < e->nombre_ennemis; ++i) if (e->ennemis[i].entite.vivant) idxs[n++] = i;
        if (n > 0) {
            int pick = idxs[rand() % n];
            ajouter_projectile(e, e->ennemis[pick].entite.x, e->ennemis[pick].entite.y + 1, +1, 1);
        }
    }

    /* Défaite immédiate si un ennemi atteint la ligne du vaisseau */
    int ligne_joueur = ligne_vaisseau(e);
    for (int i = 0; i < e->nombre_ennemis; ++i) {
        if (!e->ennemis[i].entite.vivant) continue;
        if (e->ennemis[i].entite.y >= ligne_joueur) {
            e->game_over = 1;
            e->vies = 0;
            break;
        }
    }
}

void etatjeu_deplacer_vaisseau(EtatJeu* e, int dir) {
    if (!e) return;
    e->joueur.entite.x += dir;
    if (e->joueur.entite.x < 0) e->joueur.entite.x = 0;
    if (e->joueur.entite.x >= e->largeur) e->joueur.entite.x = e->largeur - 1;
}

void etatjeu_vaisseau_tirer(EtatJeu* e) {
    if (!e) return;
    int y_vaisseau = ligne_vaisseau(e);
    ajouter_projectile(e, e->joueur.entite.x, y_vaisseau - 1, -1, 0);
}

int etatjeu_obtenir_vaisseau_x(const EtatJeu* e) { return e ? e->joueur.entite.x : 0; }
int etatjeu_obtenir_largeur(const EtatJeu* e) { return e ? e->largeur : 0; }
int etatjeu_obtenir_vies(const EtatJeu* e) { return e ? e->vies : 0; }
int etatjeu_obtenir_score(const EtatJeu* e) { return e ? e->score : 0; }
int etatjeu_devrait_quitter(const EtatJeu* e) { return e ? e->quitter : 1; }

/* API ennemis / projectiles (getters en lecture seule pour les vues) */
int etatjeu_obtenir_nombre_ennemis(const EtatJeu* e) { return e ? e->nombre_ennemis : 0; }
int etatjeu_obtenir_ennemi_x(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_ennemis) ? e->ennemis[idx].entite.x : 0; }
int etatjeu_obtenir_ennemi_y(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_ennemis) ? e->ennemis[idx].entite.y : 0; }
int etatjeu_ennemi_vivant(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_ennemis) ? e->ennemis[idx].entite.vivant : 0; }
int etatjeu_obtenir_ennemi_sante(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_ennemis) ? e->ennemis[idx].entite.sante : 0; }

int etatjeu_obtenir_nombre_projectiles(const EtatJeu* e) {
    if (!e) return 0;
    int c = 0;
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) if (e->projectiles[i].actif) ++c;
    return c;
}

int etatjeu_obtenir_projectile_x(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) {
        if (!e->projectiles[i].actif) continue;
        if (seen++ == idx) return e->projectiles[i].x;
    }
    return 0;
}

int etatjeu_obtenir_projectile_y(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) {
        if (!e->projectiles[i].actif) continue;
        if (seen++ == idx) return e->projectiles[i].y;
    }
    return 0;
}

int etatjeu_obtenir_projectile_proprietaire(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) {
        if (!e->projectiles[i].actif) continue;
        if (seen++ == idx) return e->projectiles[i].proprietaire;
    }
    return 0;
}

/* API boucliers */
int etatjeu_obtenir_nombre_boucliers(const EtatJeu* e) { return e ? e->nombre_boucliers : 0; }
int etatjeu_obtenir_bouclier_x(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_boucliers) ? e->boucliers[idx].entite.x : 0; }
int etatjeu_obtenir_bouclier_y(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_boucliers) ? e->boucliers[idx].entite.y : 0; }
int etatjeu_bouclier_vivant(const EtatJeu* e, int idx) { return (e && idx >= 0 && idx < e->nombre_boucliers) ? e->boucliers[idx].entite.vivant : 0; }

/* Accesseur pour le niveau */
int etatjeu_obtenir_niveau(const EtatJeu* e) { return e ? e->niveau : 1; }

/* Accesseur pour game over */
int etatjeu_est_game_over(const EtatJeu* e) { return e ? e->game_over : 0; }

/* Réinitialise le jeu (recommencer) */
void etatjeu_reinitialiser(EtatJeu* e) {
    if (!e) return;
    e->joueur.entite.x = e->largeur / 2;
    e->joueur.entite.y = e->hauteur - 1;
    e->joueur.entite.vivant = 1;
    e->joueur.entite.sante = 1;
    e->joueur.entite.dmg = 1;
    e->vies = 3;
    e->score = 0;
    e->niveau = 1;
    e->temps_acc = 0.0;
    e->quitter = 0;
    e->game_over = 0;
    
    /* réinitialiser les ennemis */
    e->nombre_ennemis = 0;
    e->direction_ennemis = 1;
    e->acc_deplacement_ennemis = 0.0;
    e->intervalle_deplacement_ennemis = 0.6;
    int lignes = 3;
    int colonnes = 8;
    int start_y = 2;
    int espacement_x = (e->largeur - 4) / colonnes;
    if (espacement_x < 2) espacement_x = 2;
    for (int r = 0; r < lignes; ++r) {
        for (int c = 0; c < colonnes; ++c) {
            int idx = e->nombre_ennemis++;
            if (idx >= NB_MAX_ENNEMIS) break;
            e->ennemis[idx].entite.vivant = 1;
            e->ennemis[idx].entite.x = 2 + c * espacement_x;
            e->ennemis[idx].entite.y = start_y + r*2;
            e->ennemis[idx].entite.dmg = 1;
        }
    }
    
    /* réinitialiser projectiles */
    for (int i = 0; i < NB_MAX_PROJECTILES; ++i) e->projectiles[i].actif = 0;
    e->nombre_projectiles = 0;
    
    /* réinitialiser boucliers */
    int espacement_boucliers = e->largeur / 5;
    for (int i = 0; i < NB_MAX_BOUCLIERS; ++i) {
        e->boucliers[i].entite.vivant = 1;
        e->boucliers[i].entite.x = espacement_boucliers * (i + 1);
        e->boucliers[i].entite.y = e->hauteur / 2;
        e->boucliers[i].entite.sante = 3;
        e->boucliers[i].entite.dmg = 0;
        e->boucliers[i].entite.type = TYPE_BOUCLIER;
    }
    
    /* réinitialiser particules */
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) e->particules[i].ttl = 0;
    e->nombre_particules = 0;
}

/* Aide interne pour le contrôleur : définit le drapeau quitter */
void _etatjeu_definir_quitter(EtatJeu* e, int q) { if (e) e->quitter = q; }
/* API Particules */
int etatjeu_obtenir_nombre_particules(const EtatJeu* e) {
    if (!e) return 0;
    int c = 0;
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) if (e->particules[i].ttl > 0) ++c;
    return c;
}

int etatjeu_obtenir_particule_x(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) {
        if (e->particules[i].ttl <= 0) continue;
        if (seen == idx) return e->particules[i].x;
        ++seen;
    }
    return 0;
}

int etatjeu_obtenir_particule_y(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) {
        if (e->particules[i].ttl <= 0) continue;
        if (seen == idx) return e->particules[i].y;
        ++seen;
    }
    return 0;
}

int etatjeu_obtenir_particule_type(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) {
        if (e->particules[i].ttl <= 0) continue;
        if (seen == idx) return e->particules[i].type;
        ++seen;
    }
    return 0;
}

int etatjeu_obtenir_particule_ttl(const EtatJeu* e, int idx) {
    if (!e) return 0;
    int seen = 0;
    for (int i = 0; i < NB_MAX_PARTICULES; ++i) {
        if (e->particules[i].ttl <= 0) continue;
        if (seen == idx) return e->particules[i].ttl;
        ++seen;
    }
    return 0;
}
