;########### SECCION DE DATOS
extern strncmp
extern memcpy
section .data

str_CLT db 'C', 'L', 'T', 0
str_RBO db 'R', 'B', 'O', 0

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

global resolver_automaticamente

;void resolver_automaticamente(funcionCierraCasos* funcion(devuelve uint16)[rdi],
; caso_t* arreglo_casos[rsi], caso_t* casos_a_revisar[rdx], int largo[rcx])
resolver_automaticamente:
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
    xor r13, r13
    xor r12, r12

    mov r15, rsi        ;arreglo de casos en r15
    mov r14, rdx        ;casos a revisar en r14
    mov r13, rcx        ;largo en r13
    mov rbx, rdi        ;funcion en rbx

    xor r12, r12        ;offset de la lista de casos a revisar
    
    .ciclolpmadreqlopario:
        cmp r13, 0
        je .salgo               ;si la long de mi lista de casos llega a 0, termine de analizar

        ;en r15 tengo mi arreglo de asos, despues moviendo el puntero voy a tener los otros casos
        mov r11, [r15 + CASO_USUARIO_OFFSET]    ;obtengo el ususario en r11
        xor r10, r10
        mov r10d, dword [r11 + USUARIO_NIVEL_OFFSET]   ;obtengo el nivel del usuario

        cmp r10d, 0
        je .noSeCierra          ;si el nivel del usuario es 0 no se puede cerrar
        ;si es 1 o 2, llamo a la funcion
        mov rdi, r15            ;preparo mi caso_t en rdi
        call rbx
        ;me queda en rax 0 o 1
        cmp rax, 0
        je .compararCategorias          ;si da 0 tengo q ver las categorias
        ;si la funcion dio 1, solo hay q cambiar el estado a 1
        mov word [r15 + CASO_ESTADO_OFFSET], 1
        jmp .siguienteCasoeaea

    .compararCategorias:
        ;como la categoria no tiene el caracter nulo, si uso strncmp y solo leo 3 bytes me puede decir que
        ;CLTX es igual q CLT, asi que leo 4
        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_CLT        ;cargo CLT
        mov rdx, 4              ;leo 4 bytes por si el estado tiene un char de mas
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .estado2             ;si categoria coincide con CLT, seteamos el estado en 2
        ;si el estado no coincide con CLT probamos con RBO
        mov rdi, r15            ;cargo el puntero a la categoria en rdi
        mov rsi, str_RBO        ;cargo RBO
        mov rdx, 4              ;leo 4 bytes por si el estado tiene un char de mas
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .estado2             ;si ahora coincide con RBO, seteamos estado en 2
        ;si no coincidio con niguno, no se puede cerrar
        jmp .noSeCierra

    .estado2:
        ;cambio el estado de caso_t a 2 y paso al siguiente caso
        mov word [r15 + CASO_ESTADO_OFFSET], 2
        jmp .siguienteCasoeaea

    .noSeCierra:
        ;tomo el caso_t actual y lo copio en los a revisar, paso al siguiente elem de a revisar
        mov rdi, r14            ;puntero a destino en r14
        mov rsi, r15            ;puntero a lo que quiero copiar
        mov rdx, CASO_SIZE      ;cuantos bytes quiero copiar
        call memcpy
        ;el memcpy da menos lugar a error
        ; mov r8, qword [r15]		;copio los primeros 8 bytes
	    ; mov [r14 + r12], r8
	    ; mov r8, qword [r15 + 8]	;copio del byte 8 al 16
	    ; mov [r14 + r12 + 8], r8
        ;y paso al sguiente caso_t de casos a revisar
        add r14, CASO_SIZE
    ;y ahora paso al siguiente elem de los casos generales
    .siguienteCasoeaea:
        add r15, CASO_SIZE              ;muevo el puntero al siguiente caso
        dec r13                         ;resto 1 a la longitud
        jmp .ciclolpmadreqlopario

    .salgo:
    mov rbx, [rsp]

    add rsp, 16
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret
