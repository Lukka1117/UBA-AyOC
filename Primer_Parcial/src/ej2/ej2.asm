extern free

;########### SECCION DE DATOS
section .data

;########### SECCION DE TEXTO (PROGRAMA)
section .text

; Completar las definiciones (serán revisadas por ABI enforcer):
TUIT_MENSAJE_OFFSET EQU 0
TUIT_FAVORITOS_OFFSET EQU 140
TUIT_RETUITS_OFFSET EQU 142
TUIT_ID_AUTOR_OFFSET EQU 144
TUIT_SIZE EQU 148

PUBLICACION_NEXT_OFFSET EQU 0
PUBLICACION_VALUE_OFFSET EQU 8
PUBLICACION_SIZE EQU 16

FEED_FIRST_OFFSET EQU 0 
FEED_SIZE EQU 8

USUARIO_FEED_OFFSET EQU 0;
USUARIO_SEGUIDORES_OFFSET EQU 8; 
USUARIO_CANT_SEGUIDORES_OFFSET EQU 16; 
USUARIO_SEGUIDOS_OFFSET EQU 24; 
USUARIO_CANT_SEGUIDOS_OFFSET EQU 32; 
USUARIO_BLOQUEADOS_OFFSET EQU 40; 
USUARIO_CANT_BLOQUEADOS_OFFSET EQU 48; 
USUARIO_ID_OFFSET EQU 52; 
USUARIO_SIZE EQU 56

; void bloquearUsuario(usuario_t *usuario, usuario_t *usuarioABloquear);
global bloquearUsuario 
bloquearUsuario:
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12

    ;toma dos usuarios y bloquea el acceso del segundo a los tuits del primero y viceversa
    ;agregar el usuario bloqueado al final del arreglo debloqueados
    ;borrar todas las publicaciones del feed del usuario bloqueador q contengan tuits del bloqueado
    ;borrar todas las publis del feed del usuario bloqueado q contengan tuits del usuario bloqueador
    ;aux q tome feed y usuario, y borre todas las publicaciones del feed q tengan tuits del usuario pasado
    xor r15, r15
    xor r14, r14
    xor r13, r13
    xor r12, r12

    mov r15, rdi            ;usuario bloqueador
    mov r14, rsi            ;ususario bloqueado
    ;voy a necesitar tomar el bloqueados de usuario1, y agregar al final el usuario2
    mov r13, qword[r15+USUARIO_BLOQUEADOS_OFFSET]
    xor r10, r10
    mov r10d, dword[r15+USUARIO_CANT_BLOQUEADOS_OFFSET]
    ;agrego el usuario a bloquear en la ultima pos del array
    mov qword[r13+r10*8], r14       ;metemos el bloquado
    ;aumentamos la cantidad de usuarios bloqueados
    inc r10
    mov dword[r15+USUARIO_CANT_BLOQUEADOS_OFFSET], r10d      ;actualizo la cantidad en el campo correspondiente
    ;sigo teniendo al bloqueador en r15 y al bloqueado en r14

    ;despues, tomo usuario1 feed, y usario 2 y le paso la aux
    mov rdi, qword[r15+USUARIO_FEED_OFFSET]
    mov rsi, r14                                ;ya estaba en rsi pero por las dudas
    call borrar_publicaciones_de_usuario

    ;despues tomo usuario2 feed, y usuario 1, y le paso la aux
    mov rdi, qword[r14+USUARIO_FEED_OFFSET]
    mov rsi, r15
    call borrar_publicaciones_de_usuario

    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret


;void borrar_publicaciones_de_usuario(feed_t *feed, usuario_t *usuario)
global borrar_publicaciones_de_usuario
borrar_publicaciones_de_usuario:
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    sub rsp, 8

    xor r15, r15
    xor r14, r14
    xor r13, r13
    xor r12, r12

    mov r15, rdi
    mov r14, rsi
    ;tengo un feed y un usuario, quiero borrar todas las publicaciones que aparezcan en el feed q sean delusuario
    ;necesito fijo el id del usuario, lo obtengo y ya me lo guardo
    mov r13d, dword[r14+USUARIO_ID_OFFSET]          ;obtengo el ID de usuario. me queda r14 libre

    mov r14, qword[r15 + FEED_FIRST_OFFSET]         ;primera publicacion
    
    .veoPrimerPublicacion:
        cmp r14, 0
        je .sinPublicaciones
        mov r10, qword[r14+PUBLICACION_VALUE_OFFSET]
        mov r11d, dword[r10+TUIT_ID_AUTOR_OFFSET]
        cmp r11d, r13d                              ;veo si el tuit es de el usuario o no
        je .eliminarPrimeraPubli
        ;si no era el usuario, paso a la siguiente publicacion en el feed
        je .siguientePubli     ;siguiente publi
        
        .verPublicacion:
        cmp r14, 0
        je .sinPublicaciones
        mov r10, qword[r14+PUBLICACION_VALUE_OFFSET]
        mov r11d, dword[r10+TUIT_ID_AUTOR_OFFSET]
        cmp r11d, r13d                              ;veo si el tuit es de el usuario o no
        je .eliminarPubli
        ;sino, vamos a la siguiente publi en el feed
        .siguientePubli:
        mov r14, qword[r14+PUBLICACION_NEXT_OFFSET]     ;me queda en r12 publicacion actual, en r14 publicacion anterior
        jmp .verPublicacion

    .eliminarPrimeraPubli:
        mov rbx, qword[r14+PUBLICACION_NEXT_OFFSET]         ;obtengo la next
        mov qword[r15+FEED_FIRST_OFFSET], rbx               ;muevo la siguiente al lugar de la primera publicacion
        mov rdi, r14                                        ;pongo el puntero de la publi q quiero eliminar en rdi
        call free                                           ;libero la memoria de la publicacion
        mov r14, qword[r15+FEED_FIRST_OFFSET]               ;actualizo r14 con la nueva first
        jmp .veoPrimerPublicacion                           ;salto  a ver si la nueva first es del usuario o no
    
    .eliminarPubli:
        mov rdi, r15                    ;tomo el feed
        mov rsi, r14                    ;tomo la publi q quiero borrar
        call obtener_publicacion_anterior
        ;tengo en rax el puntero a mi publicacion anterior
        mov rbx, rax
        mov r10, qword[r14+PUBLICACION_NEXT_OFFSET]             
        ;tenemos en rbx: publi anterior. r14: publi actual. r10: publi siguiente
        ;quiero ubicar publi siguiente en el campo next de publi anterior
        mov qword[rbx+PUBLICACION_NEXT_OFFSET], r10
        ;liberar el puntero de la actual
        mov rdi, r14
        call free
        ;obtener la nueva actual
        mov r14, qword[rbx+PUBLICACION_NEXT_OFFSET]
        ;volver al loop
        jmp .verPublicacion

    .sinPublicaciones:
    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret

;publicacion_t* obtener_publicacion_anterior(feed_t *feed, publicacion_t *publicacion)
global obtener_publicacion_anterior
obtener_publicacion_anterior:
    push rbp
    mov rbp, rsp
    
    xor rax, rax
    mov r10, qword[rdi+FEED_FIRST_OFFSET]
    
    .loopppp:
        cmp r10, 0              ;ultimo elem    
        je .fin
        mov r11, qword[r10+PUBLICACION_NEXT_OFFSET]
        cmp r11, rsi            ;si la siguiente es la misma publi q estoy buscando, devuelve la actual
        je .return

        mov r10, qword[r10+PUBLICACION_NEXT_OFFSET]
        jmp .loopppp

    .return:
    mov rax, r10

    .fin:
    mov rsp, rbp
    pop rbp
    ret