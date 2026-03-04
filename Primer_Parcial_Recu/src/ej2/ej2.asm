; ------------------------
; Offsets para los structs
; Plataforma: x86_64 (LP64)
; ------------------------
extern calloc
extern malloc
section .data

section .text

; COMPLETAR las definiciones (serán revisadas por ABI enforcer):
; ------------------------
; Contenido
; ------------------------
CONT_NOMBRE_OFFSET      EQU 0        ; char nombre[64]
CONT_VALOR_OFFSET       EQU 64       ; uint32_t valor
CONT_COLOR_OFFSET       EQU 68       ; char color[32]
CONT_ES_TESORO_OFFSET   EQU 100      ; bool es_tesoro
CONT_PESO_OFFSET        EQU 104      ; float peso
CONT_SIZE               EQU 108      ; sizeof(Contenido) (rounded)

; ------------------------
; Habitacion
; ------------------------
HAB_ID_OFFSET          EQU 0         ; uint32_t id
HAB_VECINOS_OFFSET     EQU 4        ; uint32_t vecinos[ACC_CANT] (4 entradas)
HAB_CONTENIDO_OFFSET   EQU 20        ; Contenido contenido (aligned to 4)
HAB_VISITAS_OFFSET     EQU 128       ; uint32_t visitas
HAB_SIZE               EQU 132       ; sizeof(Habitacion)

; ------------------------
; Mapa
; ------------------------
MAP_HABITACIONES_OFFSET    EQU 0     ; Habitacion *habitaciones  (pointer, 8 bytes)
MAP_N_HABITACIONES_OFFSET  EQU 8     ; uint64_t n_habitaciones       (8 bytes)
MAP_ID_ENTRADA_OFFSET      EQU 16    ; uint32_t id_entrada         (4 bytes)
MAP_SIZE                   EQU 24    ; sizeof(Mapa) (padded to 8)

; ------------------------
; Recorrido
; ------------------------
REC_ACCIONES_OFFSET        EQU 0     ; Accion *acciones  (pointer, 8 bytes)
REC_CANT_ACCIONES_OFFSET   EQU 8     ; uint64_t cant_acciones (8 bytes)
REC_SIZE                  EQU 16     ; sizeof(Recorrido)

; Notar que el enum aparece como puntero, entonces no afecta los offsets

;Recorrido *invertirRecorridoConDirecciones(
;const Recorrido *rec, uint64_t len);
global  invertirRecorridoConDirecciones
invertirRecorridoConDirecciones:
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

    mov r15, rdi    ;puntero a recorrido
    mov r14, rsi    ;len
    ;me devuelve un puntero a recorrido
    ;necesito pedir memoria para un struct recorrido nuevo
    ;llamo a malloc con size_recorrido
    ;me va a devolver un puntero a ese espacio de memoria
    cmp r14, 0              ;si len es 0 sale
    je .final
    
    mov rdi, 1
    mov rsi, REC_SIZE
    call calloc
    mov r13, rax    ;puntero a recorrido inicializado en 0
    
    cmp r14, 0              ;si len es 0 sale
    je .final
    ;tmb necesito un array de acciones inversas
    ;tomo recorrido cant acciones
    mov rdi, r14 ;cant acciones
    mov rsi, 4
    ;cada accion es de 4 bytees asi q malloc(4*cant_acciones)
    call calloc
    mov r12, rax    ;en r12 el puntero a mi array de acciones

    ;loopeo en acciones de recorrido, por cada accion,
    mov r15, qword[r15+REC_ACCIONES_OFFSET]  ;obtengo array acciones
    xor rbx, rbx                        ;limpio iterador
    mov rbx, r14
    dec rbx                             ;en todo esto, puse en rbx la cantidad de acciones, le reste 1, para q itere cant_acciones - 1

    .looooop:
        cmp rbx, 0
        jl .noHayMasAcciones   ;si cant_acciones-1 es >= len sale

        xor rdi, rdi
        mov edi, dword[r15]      ;indexo mi accion, tengo accion[i]
    ;llamar a conseguir inverso
        call obtener_inverso
        cmp rax, 4
        je .noHayMasAcciones
    ;lo ubico en la posicion del nuevo array
    ;rbx es, ademas, el lugar donde debria ubicarlo en el nuevo array
        mov dword[r12+rbx*4], eax
        ;paso al siguiente elemento
        dec rbx
        add r15, 4
        jmp .looooop
    
    .noHayMasAcciones:
    ;cuando salgo del loop, ubico el puntero de acciones en recorrido nuevo
        ;r13 = *recorrido
        ;r12 = *acciones
    mov qword[r13+REC_ACCIONES_OFFSET], r12
    ;seteo cant de acciones con el ya conocido
    mov qword[r13+REC_CANT_ACCIONES_OFFSET], r14

    .final:
    mov rax, r13

    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret

;int obtener_inverso(accion accion_a_invertir)
global obtener_inverso
obtener_inverso:
    push rbp
    mov rbp, rsp

    ;tengo mi accion, la voy comparando y tengo los casos
    cmp rdi, 0
    je .inverso_de_norte

    cmp rdi, 1
    je .inverso_de_sur

    cmp rdi, 2
    je .inverso_de_este

    cmp rdi, 3
    je .inverso_de_oeste

    cmp rdi, 4
    mov rax, 4
    jmp .fin
    
    .inverso_de_norte:
        mov rax, 1
        jmp .fin

    .inverso_de_sur:
        mov rax, 0
        jmp .fin

    .inverso_de_este:
        mov rax, 3
        jmp .fin

    .inverso_de_oeste:
        mov rax, 2
        jmp .fin

    .fin:
    mov rsp, rbp
    pop rbp
    ret