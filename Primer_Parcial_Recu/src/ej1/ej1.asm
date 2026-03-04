; ------------------------
; Offsets para los structs
; Plataforma: x86_64 (LP64)
; ------------------------

section .data
acciones_ejecutadas dq 0
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

;bool encontrarTesoroEnMapa(Mapa *mapa, 
;Recorrido *rec, uint64_t *acciones_ejecutadas)
global  encontrarTesoroEnMapa
encontrarTesoroEnMapa:
    push rbp
    mov rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    sub rsp, 8

    ;reseteo mi global
    mov qword [acciones_ejecutadas], 0
    
    xor r15, r15
    xor r14, r14
    xor r13, r13
    xor r12, r12
    xor rbx, rbx

    ;quiero entrar a mapa 
    ;de mapa obtengo el array a habitaciones
    mov r12, qword[rdi+MAP_HABITACIONES_OFFSET]
    ;n_habitaciones
    mov r14, qword[rdi+MAP_N_HABITACIONES_OFFSET]
    ;id_entrada
    mov r13d, dword[rdi+MAP_ID_ENTRADA_OFFSET]

    mov rbx, [acciones_ejecutadas]  
    
    ;tengo q obtener la lista de acciones de recorrido,
    mov rcx, [rsi+REC_ACCIONES_OFFSET]
    mov r9, [rsi+REC_CANT_ACCIONES_OFFSET]      ;cantidad de acciones

    cmp r14, 0
    je .noHayTesoro ;si la cant de habitaciones es 0, no hay tesos

    ;obtengo la primera habitacion con el id
    imul r13, HAB_SIZE
    mov r15, r12
    add r15, r13 ;tengo mi habitacion en r15

    .loop:
        cmp r15, 0
        je .noHayTesoro     ;si no hay habitacion no hay tesoro

        ;sumo 1 a las visitas de la habitacion
        xor r11, r11
        mov r11d, dword[r15+HAB_VISITAS_OFFSET]
        add r11, 1
        mov dword[r15+HAB_VISITAS_OFFSET], r11d

        ;ahora quiero entrar a los contenidos y ver si tiene un tesoro o no
        mov r11, r15
        add r11, HAB_CONTENIDO_OFFSET

        mov r10b, byte[r11+CONT_ES_TESORO_OFFSET]
        cmp r10b, byte 0
        ;si no es tesoro, voy a la siguiente habitacion
        je .siguienteHabitacion
        ;si es tesoro, devuelvo true y termina la ejecucion
        mov rax, 1
        jmp .fin

    .siguienteHabitacion:
        ;agarro acciones[acciones ejecutadas]
        ;si acciones ejecutadas es mayor o igual a cant_acciones, no hay tesoro
        cmp rbx, r9
        jge .noHayTesoro    ;no me quedan mas acciones

        xor r11, r11
        mov r11d, dword[rcx+rbx*4]    ;indexo
        ;se supone q me daria la accion a tomar
        
        ;ahora, r11 es el numero de id de mi habitacion siguiente que me mando a ir la accion, 
        ;entro a habitacion.vecinos[r11]
        mov r10, r15
        add r10, HAB_VECINOS_OFFSET
        ;por ahi tengo q hacer
        ;imul r11, 4
        ;add r10, r11
        mov r10d, dword[r10+r11*4]    ;*4 porq cada elem es de 4 bytes 
        ;si es 99, nohaytesoro
        cmp r10, 99
        je .noHayTesoro
        ;sino, uso ese numero para indexar en el array de hab
        imul r10, HAB_SIZE
        mov r15, r12
        add r15, r10
        add rbx, 1                    ;sumo 1 a la cantidad de acciones
        ;jmp loop
        jmp .loop

    .noHayTesoro:
    xor rax, rax

    .fin:
    ;actualizo el puntero de las ejecutadas
    mov [rdx], rbx
    
    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret