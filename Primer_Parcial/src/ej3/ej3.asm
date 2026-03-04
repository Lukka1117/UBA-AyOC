extern malloc
extern calloc
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

; tuit_t **trendingTopic(usuario_t *usuario, uint8_t (*esTuitSobresaliente)(tuit_t *));
global trendingTopic 
trendingTopic:

;;devuelve un array de *tuit_t
;;recorre el feed del usuario y devuelve un arreglo que contiene punteros  a los tuits sobresalientes hechosPOR EL USUARIO
;;el arreglo debe terminar en un elemento null para marcar el final
;;si no hay sobresalientes, se devuelve un puntero a null
;;null = 0
;;armar aux q cuente la cantidad de tuits sobresalientes
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    sub rsp, 8
    ;primero, tenemos que saber la cantidad de tuits spbresalientes del usuario, asi que reservamos los datos de entrada
    ;y llamamos a un aux q los cuente
    xor r15, r15
    xor r14, r14
    xor r13, r13
    xor r12, r12

    mov r15, rdi            ;en r15 el usuario
    mov r14, rsi            ;en r14 la funcion

    ;obtenemos el feed del usuario
    mov r13, qword [r15 + USUARIO_FEED_OFFSET]      ;ya me lo guardo aca oprq despues lo necesito para iterar
    mov r12d, dword [r15 + USUARIO_ID_OFFSET]  ;obtengo el id del usuario para comparar
    mov rdi, r13
    xor rdx, rdx
    mov edx, r12d
    ;ya tengo mi funcion en rsi
    call contar_tuits_sobresalientes_de_usuario

    cmp rax, 0
    je .noHayPublicacioness                          ;si no tiene tuits sobresalientes, se saltea todo y devuelve null(0)

    ;pido memoria para mi array. Necesito cantidad De tuits + 1 para el null final y q se incializen en0, uso calloc
    inc rax
    mov rdi, rax
    mov rsi, 8              ;como son punteros a tuits, necesito 8 bytes por puntero
    call calloc

    mov r13, [r13+FEED_FIRST_OFFSET]        ;ya obtengo tmb la primer publicacion_t*
    ;comom ya no necesito usuario de vuelta, puedo usar r15
    mov r15, rax                            ;me guardo el puntero a mi array en r15
    xor rbx, rbx                            ;limpio rbx de offset

    .loooop:
    ;ahora quiero iterar sobre el feed del usuario.
    ;obtengo el tuit de publicacion_t
        cmp r13, 0
        je .devuelvoArray

        mov rdi, qword[r13+PUBLICACION_VALUE_OFFSET]
        mov esi, dword[rdi+TUIT_ID_AUTOR_OFFSET]         ;obtengo el id del tuit
        cmp esi, r12d                                   ;comparo id del tuit con id del usuario
        jne .siguienteTuit                              ;si no son iguales, no es un tuit del usuario asi q busco otro
        ;si son iguales, llamo a la funcion, ya tengo el tuit el rdi
        call r14                                        
        cmp rax, 0                                      ;comparo si la func me dio 0 o 1
        je .siguienteTuit                               ;si dio 0, no es sobresaliente asi q pasa al siguiente
        ;si es sobresaliente, lo agrega al array 
        mov rdi, qword[r13+PUBLICACION_VALUE_OFFSET]        ;vuelvo a obtener el tuit porq se puede haber sobreescrito
        mov qword[r15 + rbx], rdi                           ;lo ubico en el array
        add rbx, 8                                          ;muevo el offset
    ;voy al siguiente obteniendo pubicacion next
        .siguienteTuit:
        mov r13, qword[r13+PUBLICACION_NEXT_OFFSET]         ;sereto r13 en la siguiente publicacion
                                                ;si next es null, no hay mas publicaciones
        jmp .loooop

    .noHayPublicacioness:
    xor rax, rax
    jmp .fin

    .devuelvoArray:
    mov rax, r15

    .fin:
    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret


;uint32_t contar_tuits_sobresalientes_de_usuario(feed_t *feed_usuario, uint8_t (*esTuitSobresaliente)(tuit_t *), uint32_t usuario_id)
global contar_tuits_sobresalientes_de_usuario
contar_tuits_sobresalientes_de_usuario:
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12

    ;maso lo mismo q antes
    xor r15, r15
    xor r14, r14
    xor r13, r13
    xor r12, r12

    mov r15, rdi            ;guardo el feed
    mov r14, rsi            ;guardo la funcion
    mov r13d, edx           ;guardo el id
    ;preparo un contador
    xor r12, r12

    ;tomo publicacion first
    mov r15, qword[r15+FEED_FIRST_OFFSET]

    .loooooooop:
        cmp r15, 0
        je .finnnnnn
        ;tomo tuit
        mov rdi, qword[r15+PUBLICACION_VALUE_OFFSET]
        ;tomo tuit id
        mov r10d, dword[rdi+TUIT_ID_AUTOR_OFFSET]
        ;comparo ids
        cmp r10d, r13d
        ;si son iguales suma, si no pasa al siguiente tomando publicacion next
        jne .siguienteeeee
        ;si son iguales se fija si es popular, llamo a la funcion q tengo en r14, en rdi tengo el tuit
        call r14
        cmp rax, 0 
        je .siguienteeeee           ;si no es popular pasa al siguiente tuit
        ;si es popular, suma 1 al contador
        inc r12

        .siguienteeeee:
        ;tomo el publicacion next
        mov r15, qword[r15+PUBLICACION_NEXT_OFFSET]
        
        jmp .loooooooop
    ;retorna contador en rax
    .finnnnnn:
    mov rax, r12

    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret