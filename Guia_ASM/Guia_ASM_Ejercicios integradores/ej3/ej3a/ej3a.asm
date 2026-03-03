;########### SECCION DE DATOS
extern malloc
section .data

;########### SECCION DE TEXTO (PROGRAMA)
section .text

; Completar las definiciones (serán revisadas por ABI enforcer):
USUARIO_ID_OFFSET EQU 0
USUARIO_NIVEL_OFFSET EQU 4
USUARIO_SIZE EQU 8

CASO_CATEGORIA_OFFSET EQU 0
CASO_ESTADO_OFFSET EQU 4
CASO_USUARIO_OFFSET EQU 8
CASO_SIZE EQU 16

SEGMENTACION_CASOS0_OFFSET EQU 0
SEGMENTACION_CASOS1_OFFSET EQU 8
SEGMENTACION_CASOS2_OFFSET EQU 16
SEGMENTACION_SIZE EQU 24

ESTADISTICAS_CLT_OFFSET EQU 0
ESTADISTICAS_RBO_OFFSET EQU 1
ESTADISTICAS_KSC_OFFSET EQU 2
ESTADISTICAS_KDT_OFFSET EQU 3
ESTADISTICAS_ESTADO0_OFFSET EQU 4
ESTADISTICAS_ESTADO1_OFFSET EQU 5
ESTADISTICAS_ESTADO2_OFFSET EQU 6
ESTADISTICAS_SIZE EQU 7

;segmentacion_t* segmentar_casos(caso_t* arreglo_casos[rdi], int largo[rsi])
global segmentar_casos
segmentar_casos:
    push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12
    sub rsp, 16

    mov qword [rsp], rbx

    xor r15, r15
    xor r14, r14
    xor rbx, rbx

    mov r15, rdi                ;array de casos en r15
    mov r14, rsi                ;largo en r14

    ;necesito pedir memoria para mi res q es un segmento_t
    ;cada segmento_t ocupa 24 bytes porq son 3 punteros q ocupan 8 cada uno
    mov rdi, SEGMENTACION_SIZE
    call malloc
    mov rbx, rax            ;tengo en rbx el puntero a mi segmento_t

    ;ahora q tengo mi aux quiero llamarla con cada nivel q necesito, voy a tener 3 arrays de casos
    ;los que voy a tener q copiar en mi segmennto_t*
    ;preparo casoos nivel 0
    mov rdi, r15
    mov rsi, r14
    mov rdx, 0
    call array_casos_por_nivel
    mov r13, rax                ;en r13 me guardo el puntero al array de casos nivel 0

    ;preparo casos nivel 1
    mov rdi, r15
    mov rsi, r14
    mov rdx, 1
    call array_casos_por_nivel
    mov r12, rax                ;en r12 me guardo el puntero al array de casos nivel 1

    ;preparo casos nivel 2
    mov rdi, r15
    mov rsi, r14
    mov rdx, 2
    call array_casos_por_nivel
    mov r11, rax                ;en r11 me guardo el puntero al array de casos nivel 2

    ;ahora quiero copiar en aegmentos_t* cada array
    mov [rbx + SEGMENTACION_CASOS0_OFFSET], r13              ;relleno el struct con los punteros
    mov [rbx + SEGMENTACION_CASOS1_OFFSET], r12
    mov [rbx + SEGMENTACION_CASOS2_OFFSET], r11

    mov rax, rbx
    
    mov rbx, [rsp]

    add rsp, 16
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret

;caso_t* array_casos_por_nivel(caso_t* arreglo_casos[rdi], int largo[rsi], int nivel[rdx])
array_casos_por_nivel:
    push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12
    ;limpio registros
    xor r15, r15
    xor r14, r14
    xor r13, r13
    ;reservo parametros
    mov r15, rdi
    mov r14, rsi
    mov r13d, edx
    ;calculo cuantos elementos va a tener mi res
    ;los parametros ya estan donde los necesito pero los paso igual porq nunca se sabe
    mov rdi, r15
    mov rsi, r14
    mov rdx, r13
    call contar_casos_por_nivel
    ;ahora quiero q el array sea un array de n elementos q cada uno ocupa 16 bytes
    cmp rax, 0
    je .noHayCasosNivel         ;hay q definir el caso donde no hay elementos para segmentar
                                ;deberia apuntar a un null
    imul rax, CASO_SIZE
    mov rdi, rax 
    call malloc
    mov r9, rax            ;en r12 guardo el puntero a mi res
    
    xor r10, r10            ;offset del res

    .loop:
        cmp r14, 0
        je .yaRecorriTodo       ;si largo llega a 0 ya recorrio todo el array

        mov r12, r15          ;tomo caso_t actual
        mov rsi, qword [r12 + CASO_USUARIO_OFFSET]    ;tomo el usuario del caso
        xor rdx, rdx
        mov edx, dword [r12 + USUARIO_NIVEL_OFFSET]   ;tomo nivel del usuario

        cmp edx, r13d                        
        ;si nivel de usuario != nivel, paso al siguiente caso
        jne .siguienteCasooo
        ;si es igual, tengo que copiar ese caso_t en mi array res
        mov r8, qword [r12]		;copio los primeros 8 bytes
	    mov [r9 + r10], r8
	    mov r8, qword [r12 + 8]	;copio del byte 8 al 16
	    mov [r9 + r10 + 8], r8
        
        ;otra opcion con memcpy
        ;mov rdi, r9    ;direccion donde escribir el caso
        ;add rdi, r10   
        ;mov rsi, r15   ;direccion del caso a leer
        ;mov rdx, caso_size
        ;call memcpy

        add r10, CASO_SIZE             ;siguiente elemento del res
        ;luego paso al siguiente caso_t
    .siguienteCasooo:
        add r15, CASO_SIZE             ;paso al siguiente elemento
        dec r14                 ;longitud--
        jmp .loop

    .yaRecorriTodo:
        mov rax, r9
        jmp .final

    .noHayCasosNivel:
        mov rax, 0

    .final:
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret
;int contar_casos_por_nivel(caso_t* arreglo_casos[rdi], int largo[rsi], int nivel[rdx])
contar_casos_por_nivel:

    push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

    xor r9, r9            ;acumulador
    ;xor r10, r10            ;offset

    .ciloloop:
        cmp rsi, 0
        je .salgoDelLoop       ;si el largo es 0 ya mire todos y sale del loop

        xor rcx, rcx
        mov rcx, rdi    ;tomo el primer caso_t
        mov r8, qword [rcx + CASO_USUARIO_OFFSET]        ;entro al usuario
        xor r11, r11
        mov r11d, dword [r8 + USUARIO_NIVEL_OFFSET]       ;ahora tengo en r11 el nivel del usuario de mi caso_t

        cmp r11d, edx           
        ;si el nivel de usuario no es el q me pide,, paso al siguiente
        jne .siguienteCaso                             
        ;si el nivel de usuario es igual al pasado por parametro, sumo uno al acumulador
        inc r9
    .siguienteCaso:
        add rdi, CASO_SIZE
        dec rsi
        jmp .ciloloop
    
    .salgoDelLoop:
    mov rax, r9

    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret