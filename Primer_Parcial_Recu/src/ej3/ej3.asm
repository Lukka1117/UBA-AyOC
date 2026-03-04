; ------------------------
; Offsets para los structs
; Plataforma: x86_64 (LP64)
; ------------------------

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

;uint32_t sumarTesoros(Mapa *mapa[rdi], uint32_t actual[esi], bool *visitado[rdx])
global  sumarTesoros
sumarTesoros
;saber cuanto valen los tesoros en total
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

    mov r12, rdi        ;mapa en r12
    mov r13d, esi       ;habitacion actual en r13
    mov r14, rdx        ;visitados en r14
    
    xor rbx, rbx        ;uso rbx de acumulador de valores

    ;veo si el id es valido, el id es invalido cuando es 99 porq significa q no hay vecino
    cmp r13d, 99
    je .finalll    ;si es 99, sale y retorna 0, rbx ya es 0 aca asi q no hace falta setarlo despues

    ;me fijo si la habitacion ya fue visitada entrando al array de visitados
    mov al, byte [r14 + r13]
    cmp al, 1
    je .finalll   ;si ya fue visitado, retornamos porq ya se manejaron sus vecinos tmb
    ;marcar como visitada
    mov byte [r14 + r13], 1

    ;quiero la habitacion actual
    ;entro al mapa, array de habitaciones, indexo con el id
    mov r15, qword [r12 + MAP_HABITACIONES_OFFSET] ;tengo en r15 el array de habitaciones
    
    mov rax, r13        ;me paso el id a otro regsitro para multiplicarlo asi puedo indexar bien (cada habitacion es de tamaño hab_size)
    mov rcx, HAB_SIZE   
    mul rcx             ;ahora en rax tengo el depalzamiento q necesio para llegar a la habitacion actual
    
    add r15, rax        ;r15 apunta a mi habitacion actual

    ;incrementar contador de visitas de la habitación, por ahi no es necesario pero por las dudas
    inc dword [r15 + HAB_VISITAS_OFFSET]

    ;entro al contenido, miro el valor de es_tesoro
    cmp byte [r15 + HAB_CONTENIDO_OFFSET + CONT_ES_TESORO_OFFSET], 1    
    jne .buscar_en_vecinos                                               ;si no es tesoro,miro sus vecinos
    ;si tesoro, tomo el valor y lo sumo a rbx
    mov eax, dword [r15 + HAB_CONTENIDO_OFFSET + CONT_VALOR_OFFSET]
    add rbx, rax

;la parte fea
;recorrer recursivamente los vecinos
.buscar_en_vecinos:
    ;cada habitacion puede tener hasta 4 vecinos, tenemo el array vecinos[ACC_CANT]. 
    ;aprovehco para iterar del 0 al 3

    xor r10, r10        ;limpio para el bucle

    .vecinos_looooop:
        cmp r10, 4          
        je .finalll   ;si es = 4, ya miro todos los vecinos y sale 

        ;para volver a llamar sumar_tesoros, tengo que preparar mis valores en cada registro
        ;necesito el puntero al mapa, el id del vecino, y el array de visitados en ese orden
        
        mov rdi, r12        ;guardamos en rdi el puntero al mapa
        ;en la habitacion actual entramos a vecinos, y vamos al correspndiente con el iterador, multiplica por 4 porq cada elem ocupa 4 bytes
        ;asi guardo el id de la habitacion vecina en rsi
        mov esi, dword [r15 + HAB_VECINOS_OFFSET + r10*4]
        mov rdx, r14        ;y guardamos el array de visitados en rdx

        ;como r10 es volatil, necesito guardarlo antes de un call porq puedo perder mi iterador
        push r10
        sub rsp, 8          ;realineo la pila a 16 q sino llora el abi xd
        call sumarTesoros   ;llamo a la funcion con el vecino obtenido
        add rsp, 8          
        pop r10             ;recupero el iterador

        add rbx, rax        ;sumamos el resultado a rbx

        inc r10             ;iterador++
        jmp .vecinos_looooop

.finalll:
    mov rax, rbx        ;ponemos el total en rax para retornar

    add rsp, 8
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    mov rsp, rbp
    pop rbp
    ret
    
