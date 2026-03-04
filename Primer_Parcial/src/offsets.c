#include "ejs.h" // Asumiendo que has guardado tus definiciones en un archivo llamado "tu_header.h"
#include <stddef.h> 
#include <stdio.h>

void imprimir_offsets() {
    printf("--- Offsets y Tamaños de las Estructuras ---\n");
    printf("\n");

    // Tuit
    printf("Estructura: tuit_t (Tamaño: %zu bytes)\n", sizeof(tuit_t));
    printf("  Offset mensaje:    %zu\n", offsetof(tuit_t, mensaje));
    printf("  Offset favoritos:  %zu\n", offsetof(tuit_t, favoritos));
    printf("  Offset retuits:    %zu\n", offsetof(tuit_t, retuits));
    printf("  Offset id_autor:   %zu\n", offsetof(tuit_t, id_autor));
    printf("\n");

    // Publicacion
    printf("Estructura: publicacion_t (Tamaño: %zu bytes)\n", sizeof(publicacion_t));
    printf("  Offset next:       %zu\n", offsetof(publicacion_t, next));
    printf("  Offset value:      %zu\n", offsetof(publicacion_t, value));
    printf("\n");

    // Feed
    printf("Estructura: feed_t (Tamaño: %zu bytes)\n", sizeof(feed_t));
    printf("  Offset first:      %zu\n", offsetof(feed_t, first));
    printf("\n");

    // Usuario
    printf("Estructura: usuario_t (Tamaño: %zu bytes)\n", sizeof(usuario_t));
    printf("  Offset feed:             %zu\n", offsetof(usuario_t, feed));
    printf("  Offset seguidores:       %zu\n", offsetof(usuario_t, seguidores));
    printf("  Offset cantSeguidores:   %zu\n", offsetof(usuario_t, cantSeguidores));
    printf("  Offset seguidos:         %zu\n", offsetof(usuario_t, seguidos));
    printf("  Offset cantSeguidos:     %zu\n", offsetof(usuario_t, cantSeguidos));
    printf("  Offset bloqueados:       %zu\n", offsetof(usuario_t, bloqueados));
    printf("  Offset cantBloqueados:   %zu\n", offsetof(usuario_t, cantBloqueados));
    printf("  Offset id:               %zu\n", offsetof(usuario_t, id));
    printf("\n");
}

int main() {
    imprimir_offsets();
    return 0;
}