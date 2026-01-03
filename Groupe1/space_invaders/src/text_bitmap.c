#include "text_bitmap.h"

#include <ctype.h>

/* Rectangle utilitaire pour dessiner des glyphes */
static void draw_rect(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color) {
    SDL_FRect r = { (float)x, (float)y, (float)w, (float)h };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &r);
}

/* Motifs 5x7 (# = plein, espace = vide) */
static const char* glyph_pattern(char c) {
    switch (c) {
        case 'A': return " ### ""#   #""#   #""#####""#   #""#   #""#   #";
        case 'B': return "#### ""#   #""#   #""#### ""#   #""#   #""#### ";
        case 'C': return " ### ""#   #""#    ""#    ""#    ""#   #"" ### ";
        case 'D': return "#### ""#   #""#   #""#   #""#   #""#   #""#### ";
        case 'E': return "#####""#    ""#    ""#### ""#    ""#    ""#####";
        case 'F': return "#####""#    ""#    ""#### ""#    ""#    ""#    ";
        case 'G': return " ### ""#   #""#    ""# ###""#   #""#   #"" ### ";
        case 'H': return "#   #""#   #""#   #""#####""#   #""#   #""#   #";
        case 'I': return " ### ""  #  ""  #  ""  #  ""  #  ""  #  "" ### ";
        case 'J': return "  ###""   # ""   # ""   # ""#  # ""#  # "" ##  ";
        case 'K': return "#   #""#  # ""# #  ""##   ""# #  ""#  # ""#   #";
        case 'L': return "#    ""#    ""#    ""#    ""#    ""#    ""#####";
        case 'M': return "#   #""## ##""# # #""# # #""#   #""#   #""#   #";
        case 'N': return "#   #""##  #""# # #""#  ##""#   #""#   #""#   #";
        case 'O': return " ### ""#   #""#   #""#   #""#   #""#   #"" ### ";
        case 'P': return "#### ""#   #""#   #""#### ""#    ""#    ""#    ";
        case 'Q': return " ### ""#   #""#   #""#   #""# # #""#  ##"" ####";
        case 'R': return "#### ""#   #""#   #""#### ""# #  ""#  # ""#   #";
        case 'S': return " ### ""#   #""#    "" ### ""    #""#   #"" ### ";
        case 'T': return "#####""  #  ""  #  ""  #  ""  #  ""  #  ""  #  ";
        case 'U': return "#   #""#   #""#   #""#   #""#   #""#   #"" ### ";
        case 'V': return "#   #""#   #""#   #""#   #""#   #"" # # ""  #  ";
        case 'W': return "#   #""#   #""#   #""# # #""# # #""## ##""#   #";
        case 'X': return "#   #""#   #"" # # ""  #  "" # # ""#   #""#   #";
        case 'Y': return "#   #""#   #"" # # ""  #  ""  #  ""  #  ""  #  ";
        case 'Z': return "#####""    #""   # ""  #  "" #   ""#    ""#####";
        case '0': return " ### ""#   #""#  ##""# # #""##  #""#   #"" ### ";
        case '1': return "  #  "" ##  ""# #  ""  #  ""  #  ""  #  ""#####";
        case '2': return " ### ""#   #""    #""   # ""  #  "" #   ""#####";
        case '3': return " ### ""#   #""    #"" ### ""    #""#   #"" ### ";
        case '4': return "#   #""#   #""#   #""#####""    #""    #""    #";
        case '5': return "#####""#    ""#    ""#### ""    #""#   #"" ### ";
        case '6': return " ### ""#   #""#    ""#### ""#   #""#   #"" ### ";
        case '7': return "#####""    #""   # ""  #  "" #   "" #   "" #   ";
        case '8': return " ### ""#   #""#   #"" ### ""#   #""#   #"" ### ";
        case '9': return " ### ""#   #""#   #"" ### ""    #""#   #"" ### ";
        case '<': return "  #  "" #   ""#    ""#    ""#    "" #   ""  #  ";
        case '>': return "  #  ""   # ""    #""    #""    #""   # ""  #  ";
        case '#': return " # # "" # # ""#####"" # # ""#####"" # # "" # # ";
        case '.': return "     ""     ""     ""     ""     ""  #  ""     ";
        case ' ': return NULL; /* espace géré par l'avance */
        case ':': return "     ""  #  ""     ""     ""     ""  #  ""     ";
        case '-': return "     ""     ""     "" ### ""     ""     ""     ";
        default:  return NULL;
    }
}

static int draw_glyph(SDL_Renderer* renderer, int x, int y, char ch, SDL_Color color, int pixel, int spacing) {
    char c = (char)toupper((unsigned char)ch);
    const char* pat = glyph_pattern(c);

    int advance = pixel * 5 + spacing; /* largeur fixe 5 colonnes */
    if (!pat) return advance;

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (pat[row * 5 + col] == '#') {
                draw_rect(renderer, x + col * pixel, y + row * pixel, pixel, pixel, color);
            }
        }
    }

    return advance;
}

void bitmap_draw_text_custom(SDL_Renderer* renderer, int x, int y, const char* text, SDL_Color color, int pixel_size, int spacing) {
    if (!renderer || !text) return;
    int cursor_x = x;
    for (const char* ch = text; *ch; ++ch) {
        cursor_x += draw_glyph(renderer, cursor_x, y, *ch, color, pixel_size, spacing);
    }
}

void bitmap_draw_text(SDL_Renderer* renderer, int x, int y, const char* text, SDL_Color color) {
    bitmap_draw_text_custom(renderer, x, y, text, color, BITMAP_FONT_DEFAULT_SIZE, BITMAP_FONT_DEFAULT_SPACING);
}
