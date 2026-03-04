extern malloc
extern calloc
extern strcpy
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

; tuit_t *publicar(char *mensaje, usuario_t *usuario);
global publicar
publicar:
;;tomando un mensaje y un usuario, crea un tuit y lo agrega como publicacion al principio de su propio feed
;; y del feed de los demas.
;;el mesnsaje debe ser clonado
;;armar funcion auxiliar que dado un tuit y un feed, cree una publicacion y la agregue al principio del feed
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    sub rsp, 8
    ;primero, como quiero crear un tuit, pido memoria suficiente para ubicar sus campos
    ;un tuit es de tamaño 148 bytes, necesito poner ese valor en rdi para llamar a malloc
    ;para hacer eso, me reservo mis valores de rdi, rsi en registros no volatiles para q no se sobreescriban
    xor r15, r15
    xor r14, r14
    mov r15, rdi    ;guardo en r15 el puntero a mi mensaje
    mov r14, rsi    ;guardo en r14 el puntero a mi usuario_t

    mov rdi, 1
    mov rsi, TUIT_SIZE
    call calloc             ;ahora tengo en rax un puntero a mi nuevo tuit, lo voy a mover a otro reg
    mov r13, rax            ;me quedo mi puntero del nuevo tuit en en r13 con todos los campos en 0

    ;ahora voy a copiar el mensaje, strcpy *dst, *src. Paso mi puntero amensaje q quiero copiar a rsi
    mov rsi, r15
    mov rdi, r13
    ;add rdi, tuit_nombre_autor_offset          esto seria en caso de q no sea el primer campo
    call strcpy     ;el string queda copiado a aprtir de la direccion base de la estructura (r13), se copia en la memoria a la q 
                    ;apunta rdi antes del strcpy, entonces el mensaje ya esta en r13, que es lo que quiero

    ;ahora, a partir de mi usuario, consigo su id y lo agrego al campo correspondiente del tuit
    mov edi, dword [r14 + USUARIO_ID_OFFSET]
    mov dword [r13 + TUIT_ID_AUTOR_OFFSET], edi     ;ya tengo mi tuit seteado con el mensaje y el id, elresto esta en 0 por calloc

    ;me falta obtener el feed del usuario, tomo el feed y el tuit y llamo a la aux
    mov rdi, qword [r14 + USUARIO_FEED_OFFSET]  ;obtengo el puntero al feed de usuario
    mov rsi, r13                            ;preparo el tuit
    call crear_publicacion_y_agregar_a_feed

    ;despues tomo los seguidores y cantidad de seguidores, y voy iterando
    ;por cada seguidor, tomo su feed y llamo a la funcion aux con el tuit nuevo.
    ;tengo libre r15, r12 y rbx
    xor r12, r12
    xor rbx, rbx

    xor r15, r15        ;limpio r15 para usar de iterador
    mov r12, qword [r14 + USUARIO_SEGUIDORES_OFFSET]    ;guardo en r12 el array de usuarios q siguen a usuario
    mov ebx, dword [r14 + USUARIO_CANT_SEGUIDORES_OFFSET]   ;cantidad de seguidores en ebx

    .loop:
        cmp r15d, ebx
        je .chauloop        ;si iterador es igual a cant de seguidores, ya recorrio todo y sale del loop

        ;en r12 tengo un puntero a *usuario_t, quiero agarrar eso, y en ese entrar a su campo *feed_t
        mov r10, [r12 + r15*8]      ;obtengo *usuario_t, tengo q hacer +r15*8 para sumarle un  offset
                                    ;*8 porq cada puntero ocupa 8 bytes
        mov rdi, qword [r10 + USUARIO_FEED_OFFSET]    ;obtengo feed de uno de los seguidores de usuario
        mov rsi, r13                            ;preparo el tuit
        call crear_publicacion_y_agregar_a_feed

        ;ahora q ya agregue el tuit al feed, paso al siguiente usuario
        inc r15
        jmp .loop

    .chauloop:
    
    mov rax, r13        ;devuelvo el tuit

    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret


;void crear_publicacion_y_agregar_a_feed(feed_t *feed, tuit_t *tuit)
global crear_publicacion_y_agregar_a_feed
crear_publicacion_y_agregar_a_feed:
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12
    ;aca necesito perimero crear una publicacion y despues agregarlo al feed pasado
    ;para crear el feed, pido memoria necesria, o sea un malloc de publicacion size
    xor r15, r15
    xor r14, r14
    mov r15, rdi        ;preservo *feed_t en r15
    mov r14, rsi        ;preservo tuit_t* en r14

    mov rdi, PUBLICACION_SIZE
    call malloc
    mov r13, rax        ;tengo en r13 el puntero a mi nueva publicacion.

    ;ahora, para setear esa publicacion necesito poner los datos en suscampos, que hago? muevo el tuit al campo value
    ;y para el campo next, necesito un puntero a la siguiente publicacion. esa publi tiene q ser pubicacion_t* first de mi feed_t
    ;asi q obtengo primero el first del feed_t
    mov rdi, [r15 + FEED_FIRST_OFFSET]      ;aca ya entre a mi struct feed_t que su direccion apunta a *publicacion_t first
                                            ;podria no tener el offset porq es 0
    mov qword[r13 + PUBLICACION_NEXT_OFFSET], rdi
    mov qword[r13 + PUBLICACION_VALUE_OFFSET], r14

    ;me falta setear publicacion_t* first de mi feed_t para q apunte a mi nueva publicacion
    mov qword[r15 + FEED_FIRST_OFFSET], r13
    ;NO TE OLVIDES DE SETEAR BIEN ESTAS COSAS CONCHUDA

    ;se supone q deberia alcanzar con eso 

    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret