/*
 * view_sdl.c
 * ----------
 * Implémentation de la vue graphique SDL3 pour Space Invaders.
 * Gère le rendu des entités (ennemis, projectiles, vaisseau),
 * la boucle d'événements SDL, et l'entrée utilisateur.
 */

#include "view_sdl.h"
#include "controller.h"
#include "text_bitmap.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Configuration de la fenêtre et du rendu */
#define LARGEUR_FENETRE  800
#define HAUTEUR_FENETRE  600
#define TAILLE_CELLULE   10

/* Structure pour gérer l'état SDL */
typedef struct {
    SDL_Window* fenetre;
    SDL_Renderer* rendu;
    int en_cours;
} ContexteSDL;

static KeyBindings g_bindings = { SDLK_LEFT, SDLK_RIGHT, SDLK_SPACE, SDLK_P, SDLK_Q };

void vue_sdl_get_bindings(KeyBindings* out) {
    if (out) *out = g_bindings;
}

void vue_sdl_set_bindings(const KeyBindings* in) {
    if (in) g_bindings = *in;
}

/* Couleurs prédéfinies */
static SDL_Color couleur_rouge = {255, 0, 0, 255};
static SDL_Color couleur_orange = {255, 165, 0, 255};
static SDL_Color couleur_vert = {0, 255, 0, 255};
static SDL_Color couleur_cyan = {0, 120, 220, 255};
static SDL_Color couleur_jaune = {255, 255, 0, 255};
static SDL_Color couleur_magenta = {255, 0, 255, 255};

static void keycode_label(SDL_Keycode code, char* buf, size_t sz) {
    const char* name = SDL_GetKeyName(code);
    if (!name) name = "?";
    snprintf(buf, sz, "%s", name);
}

/* Initialise SDL et crée la fenêtre/rendu */
static ContexteSDL* sdl_initialiser(void) {
    ContexteSDL* contexte = (ContexteSDL*)malloc(sizeof(ContexteSDL));
    if (!contexte) return NULL;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        free(contexte);
        return NULL;
    }

    contexte->fenetre = SDL_CreateWindow("Space Invaders - SDL3",
                                         LARGEUR_FENETRE, HAUTEUR_FENETRE,
                                         SDL_WINDOW_RESIZABLE);
    if (!contexte->fenetre) {
        fprintf(stderr, "Erreur SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        free(contexte);
        return NULL;
    }

    contexte->rendu = SDL_CreateRenderer(contexte->fenetre, NULL);
    if (!contexte->rendu) {
        fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(contexte->fenetre);
        SDL_Quit();
        free(contexte);
        return NULL;
    }

    contexte->en_cours = 1;
    SDL_SetRenderDrawColor(contexte->rendu, 0, 0, 0, 255);

    return contexte;
}

/* Affiche un menu pause simple et attend une action */
static int afficher_pause(ContexteSDL* contexte, EtatJeu* e) {
    int continuer = 1;
    while (continuer) {
        int largeur_fenetre, hauteur_fenetre;
        SDL_GetRenderOutputSize(contexte->rendu, &largeur_fenetre, &hauteur_fenetre);

        SDL_SetRenderDrawColor(contexte->rendu, 0, 0, 0, 180);
        SDL_FRect fond = {0, 0, (float)largeur_fenetre, (float)hauteur_fenetre};
        SDL_RenderFillRect(contexte->rendu, &fond);

        SDL_Color couleur_blanche = {255, 255, 255, 255};
        SDL_Color couleur_cyan_local = {0, 200, 255, 255};
        SDL_Color couleur_magenta_local = {255, 0, 200, 255};
        char keyname[32];
        char quit_label[64];
        keycode_label(g_bindings.quitter, keyname, sizeof(keyname));

        bitmap_draw_text(contexte->rendu, largeur_fenetre / 2 - 40, hauteur_fenetre / 2 - 80, "PAUSE", couleur_blanche);
        bitmap_draw_text(contexte->rendu, largeur_fenetre / 2 - 140, hauteur_fenetre / 2 - 20, "ENTREE POUR REPRENDRE", couleur_cyan_local);
        snprintf(quit_label, sizeof(quit_label), "%s POUR QUITTER", keyname);
        bitmap_draw_text(contexte->rendu, largeur_fenetre / 2 - 110, hauteur_fenetre / 2 + 20, quit_label, couleur_magenta_local);

        SDL_RenderPresent(contexte->rendu);

        SDL_Event evt;
        if (SDL_WaitEvent(&evt)) {
            if (evt.type == SDL_EVENT_QUIT) {
                return 0; /* quitter le jeu */
            }
            if (evt.type == SDL_EVENT_KEY_DOWN) {
                if (evt.key.key == g_bindings.quitter) {
                    controleur_appliquer_commande(e, CMD_QUITTER);
                    return 0;
                }
                if (evt.key.key == g_bindings.pause || evt.key.key == SDLK_RETURN || evt.key.key == SDLK_KP_ENTER || evt.key.key == SDLK_ESCAPE) {
                    continuer = 0; /* reprendre */
                }
            }
        }
    }
    return 1;
}

/* Libère les ressources SDL */
static void sdl_quitter(ContexteSDL* contexte) {
    if (!contexte) return;
    if (contexte->rendu) {
        SDL_DestroyRenderer(contexte->rendu);
        contexte->rendu = NULL;
    }
    if (contexte->fenetre) {
        SDL_DestroyWindow(contexte->fenetre);
        contexte->fenetre = NULL;
    }
    SDL_Quit();
    free(contexte);
}

/* Dessine un rectangle rempli */
static void dessiner_rectangle(SDL_Renderer* rendu, int x, int y, int largeur, int hauteur,
                               SDL_Color couleur) {
    SDL_FRect rect = {(float)x, (float)y, (float)largeur, (float)hauteur};
    SDL_SetRenderDrawColor(rendu, couleur.r, couleur.g, couleur.b, couleur.a);
    SDL_RenderFillRect(rendu, &rect);
}

/* Traite les événements SDL et retourne 0 si quitter, 1 sinon */
static int traiter_evenements(ContexteSDL* contexte, EtatJeu* e) {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_EVENT_QUIT) return 0;
        if (evt.type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode k = evt.key.key;

            if (k == g_bindings.gauche || k == SDLK_LEFT || k == SDLK_A) {
                controleur_appliquer_commande(e, CMD_GAUCHE);
            } else if (k == g_bindings.droite || k == SDLK_RIGHT || k == SDLK_D) {
                controleur_appliquer_commande(e, CMD_DROITE);
            } else if (k == g_bindings.tirer || k == SDLK_SPACE) {
                controleur_appliquer_commande(e, CMD_TIRER);
            } else if (k == g_bindings.pause) {
                if (!afficher_pause(contexte, e)) return 0;
            } else if (k == g_bindings.quitter) {
                controleur_appliquer_commande(e, CMD_QUITTER);
                return 0;
            }
        }
    }
    return 1;
}

/* Affichage des éléments du jeu */
static void afficher_jeu(SDL_Renderer* rendu, EtatJeu* e,
                         int largeur_jeu, int hauteur_jeu) {
    /* Fond noir */
    SDL_SetRenderDrawColor(rendu, 0, 0, 0, 255);
    SDL_RenderClear(rendu);

    /* Calculer les dimensions d'une cellule en fonction de la fenêtre */
    int largeur_fenetre, hauteur_fenetre;
    SDL_GetRenderOutputSize(rendu, &largeur_fenetre, &hauteur_fenetre);
    float largeur_cellule = (float)largeur_fenetre / largeur_jeu;
    float hauteur_cellule = (float)hauteur_fenetre / hauteur_jeu;

    /* Dessiner les ennemis */
    int nombre_ennemis = etatjeu_obtenir_nombre_ennemis(e);
    for (int idx = 0; idx < nombre_ennemis; ++idx) {
        if (!etatjeu_ennemi_vivant(e, idx)) continue;
        int ennemi_x = etatjeu_obtenir_ennemi_x(e, idx);
        int ennemi_y = etatjeu_obtenir_ennemi_y(e, idx);
        int sante = etatjeu_obtenir_ennemi_sante(e, idx);
        int x_pixel = (int)(ennemi_x * largeur_cellule);
        int y_pixel = (int)(ennemi_y * hauteur_cellule);
        int l = (int)largeur_cellule;
        int h = (int)hauteur_cellule;
        if (l < 1) l = 1;
        if (h < 1) h = 1;
        /* Ennemis forts (sante >= 2) en rouge, normaux en orange */
        SDL_Color couleur_ennemi = (sante >= 2) ? couleur_rouge : couleur_orange;
        dessiner_rectangle(rendu, x_pixel, y_pixel, l, h, couleur_ennemi);
    }

    /* Dessiner les boucliers */
    int nombre_boucliers = etatjeu_obtenir_nombre_boucliers(e);
    for (int idx = 0; idx < nombre_boucliers; ++idx) {
        if (!etatjeu_bouclier_vivant(e, idx)) continue;
        int bouclier_x = etatjeu_obtenir_bouclier_x(e, idx);
        int bouclier_y = etatjeu_obtenir_bouclier_y(e, idx);
        int x_pixel = (int)(bouclier_x * largeur_cellule);
        int y_pixel = (int)(bouclier_y * hauteur_cellule);
        int l = (int)largeur_cellule;
        int h = (int)hauteur_cellule;
        if (l < 1) l = 1;
        if (h < 1) h = 1;
        dessiner_rectangle(rendu, x_pixel, y_pixel, l, h, couleur_vert);
    }

    /* Dessiner les projectiles */
    int nombre_projectiles = etatjeu_obtenir_nombre_projectiles(e);
    for (int idx = 0; idx < nombre_projectiles; ++idx) {
        int proj_x = etatjeu_obtenir_projectile_x(e, idx);
        int proj_y = etatjeu_obtenir_projectile_y(e, idx);
        int proprietaire = etatjeu_obtenir_projectile_proprietaire(e, idx);
        int x_pixel = (int)(proj_x * largeur_cellule);
        int y_pixel = (int)(proj_y * hauteur_cellule);
        int l = (int)(largeur_cellule * 0.5f); /* projectile plus petit */
        int h = (int)(hauteur_cellule);
        if (l < 1) l = 1;
        if (h < 1) h = 1;
        SDL_Color couleur_proj = (proprietaire == 0) ? couleur_jaune : couleur_magenta;
        dessiner_rectangle(rendu, x_pixel + (int)(largeur_cellule * 0.25f), y_pixel, l, h, couleur_proj);
    }

    /* Dessiner les particules d'explosion */
    int nombre_particules = etatjeu_obtenir_nombre_particules(e);
    for (int idx = 0; idx < nombre_particules; ++idx) {
        int part_x = etatjeu_obtenir_particule_x(e, idx);
        int part_y = etatjeu_obtenir_particule_y(e, idx);
        int part_type = etatjeu_obtenir_particule_type(e, idx);
        int part_ttl = etatjeu_obtenir_particule_ttl(e, idx);
        int x_pixel = (int)(part_x * largeur_cellule);
        int y_pixel = (int)(part_y * hauteur_cellule);
        int taille = (int)(largeur_cellule * 0.3f);
        if (taille < 1) taille = 1;
        
        /* Choisir la couleur selon le type d'entité */
        SDL_Color couleur_particule;
        if (part_type == TYPE_ENNEMI_FAIBLE) couleur_particule = couleur_orange;
        else if (part_type == TYPE_ENNEMI_FORT) couleur_particule = couleur_rouge;
        else if (part_type == TYPE_BOUCLIER) couleur_particule = couleur_vert;
        else couleur_particule = couleur_cyan; /* joueur */
        
        /* Réduire l'opacité avec le temps */
        couleur_particule.a = (Uint8)((part_ttl / 20.0f) * 255);
        dessiner_rectangle(rendu, x_pixel, y_pixel, taille, taille, couleur_particule);
    }

    /* Dessiner le vaisseau */
    int vaisseau_x = etatjeu_obtenir_vaisseau_x(e);
    int vaisseau_y = hauteur_jeu - 2;
    int x_pixel = (int)(vaisseau_x * largeur_cellule);
    int y_pixel = (int)(vaisseau_y * hauteur_cellule);
    int l = (int)largeur_cellule;
    int h = (int)(hauteur_cellule * 1.5f);
    if (l < 1) l = 1;
    if (h < 1) h = 1;
    dessiner_rectangle(rendu, x_pixel, y_pixel, l, h, couleur_cyan);

    /* Afficher les vies (petits carrés en haut à gauche) */
    int vies = etatjeu_obtenir_vies(e);
    for (int i = 0; i < vies; ++i) {
        dessiner_rectangle(rendu, 10 + i * 25, 10, 20, 20, couleur_cyan);
    }
    
    /* Afficher le niveau en haut au centre */
    int niveau = etatjeu_obtenir_niveau(e);
    char niveau_texte[32];
    snprintf(niveau_texte, sizeof(niveau_texte), "LEVEL %d", niveau);
    SDL_Color couleur_blanche = {255, 255, 255, 255};
    bitmap_draw_text(rendu, largeur_fenetre / 2 - 40, 10, niveau_texte, couleur_blanche);
    
    /* Afficher le score en chiffres en haut à droite */
    int score = etatjeu_obtenir_score(e);
    char score_texte[32];
    snprintf(score_texte, sizeof(score_texte), "SCORE %d", score);
    bitmap_draw_text(rendu, largeur_fenetre - 150, 10, score_texte, couleur_blanche);

    SDL_RenderPresent(rendu);
}

/* Boucle principale de la vue SDL */
int vue_sdl_executer(EtatJeu* e) {
    if (!e) return -1;

    ContexteSDL* contexte = sdl_initialiser();
    if (!contexte) return -1;

    const int largeur_jeu = etatjeu_obtenir_largeur(e);
    const int hauteur_jeu = 24; /* hauteur du jeu en cellules */

    const int fps_cible = 60;
    const unsigned int temps_image = 1000 / fps_cible; /* en ms */
    unsigned int temps_precedent = SDL_GetTicks();

    while (contexte->en_cours && !etatjeu_devrait_quitter(e)) {
        unsigned int temps_courant = SDL_GetTicks();
        unsigned int temps_ecoule = temps_courant - temps_precedent;

        /* Si game over, afficher écran de fin et attendre choix */
        if (etatjeu_est_game_over(e)) {
            SDL_SetRenderDrawColor(contexte->rendu, 0, 0, 0, 255);
            SDL_RenderClear(contexte->rendu);
            
            /* Afficher grand rectangle rouge au centre (GAME OVER) */
            int largeur_fenetre, hauteur_fenetre;
            SDL_GetRenderOutputSize(contexte->rendu, &largeur_fenetre, &hauteur_fenetre);
            
            /* Grand rectangle rouge central encore plus large */
            const int zone_w = 620;
            const int zone_h = 440;
            const int zone_x = largeur_fenetre / 2 - zone_w / 2;
            const int zone_y = hauteur_fenetre / 2 - zone_h / 2;
            dessiner_rectangle(contexte->rendu, zone_x, zone_y, zone_w, zone_h, couleur_rouge);
            
            /* Cadre blanc autour */
            SDL_SetRenderDrawColor(contexte->rendu, 255, 255, 255, 255);
            SDL_FRect cadre = {(float)(zone_x - 10), (float)(zone_y - 10), zone_w + 20, zone_h + 20};
            SDL_RenderRect(contexte->rendu, &cadre);
            
            /* Afficher "GAME OVER" agrandi */
            SDL_Color couleur_blanche = {255, 255, 255, 255};
            const int titre_size = 6;
            const int titre_spacing = 7;
            const int titre_glyph_w = titre_size * 4 + titre_spacing;
            const int titre_len = 9; /* "GAME OVER" (avec espace) */
            int titre_w = titre_len * titre_glyph_w;
            int titre_x = largeur_fenetre / 2 - titre_w / 2;
            int titre_y = zone_y + 60;
            bitmap_draw_text_custom(contexte->rendu, titre_x, titre_y, "GAME OVER", couleur_blanche, titre_size, titre_spacing);
            
            /* Afficher le score légèrement plus grand */
            int score_final = etatjeu_obtenir_score(e);
            char score_texte[32];
            snprintf(score_texte, sizeof(score_texte), "SCORE %d", score_final);
            const int score_size = 4;
            const int score_spacing = 5;
            const int score_glyph_w = score_size * 4 + score_spacing;
            int score_w = (int)strlen(score_texte) * score_glyph_w;
            int score_x = largeur_fenetre / 2 - score_w / 2;
            int score_y = zone_y + zone_h / 2 - 15;
            bitmap_draw_text_custom(contexte->rendu, score_x, score_y, score_texte, couleur_jaune, score_size, score_spacing);
            
            /* Instructions visuelles: bouton bleu très large, texte taille standard */
            const int box_largeur = 500;
            const int box_hauteur = 90;
            const int box_x = largeur_fenetre / 2 - box_largeur / 2;
            const int box_y = zone_y + zone_h - box_hauteur - 20;
            dessiner_rectangle(contexte->rendu, box_x, box_y, box_largeur, box_hauteur, couleur_cyan);

            const char* msg_continue = "ENTREE POUR CONTINUER";
            const int btn_size = BITMAP_FONT_DEFAULT_SIZE;
            const int btn_spacing = BITMAP_FONT_DEFAULT_SPACING;
            const int btn_glyph_w = btn_size * 4 + btn_spacing;
            const int btn_glyph_h = btn_size * 5;
            int text_w = (int)strlen(msg_continue) * btn_glyph_w;
            int text_x = largeur_fenetre / 2 - text_w / 2 - 10; 
            int text_y = box_y + (box_hauteur - btn_glyph_h) / 2;
            bitmap_draw_text_custom(contexte->rendu, text_x, text_y, msg_continue, couleur_blanche, btn_size, btn_spacing);
            
            SDL_RenderPresent(contexte->rendu);
            
            /* Attendre événement */
            SDL_Event evt;
            if (SDL_WaitEvent(&evt)) {
                if (evt.type == SDL_EVENT_QUIT || evt.type == SDL_EVENT_KEY_DOWN) {
                    /* Quitter la boucle : main gérera le highscore puis le menu */
                    contexte->en_cours = 0;
                }
            }
            continue;
        }

        if (!traiter_evenements(contexte, e)) {
            break;
        }

        /* Mise à jour du jeu */
        etatjeu_mettre_a_jour(e, temps_ecoule / 1000.0f);

        /* Affichage */
        afficher_jeu(contexte->rendu, e, largeur_jeu, hauteur_jeu);

        /* Limitation du frame rate */
        if (temps_ecoule < temps_image) {
            SDL_Delay(temps_image - temps_ecoule);
        }

        /* Mise à jour du temps pour la prochaine itération */
        temps_precedent = temps_courant;
    }

    sdl_quitter(contexte);
    return 0;
}
