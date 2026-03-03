;########### SECCION DE DATOS
extern malloc
extern strncmp
section .data
str_CLT db 'C', 'L', 'T', 0
str_RBO db 'R', 'B', 'O', 0
str_KSC db 'K', 'S', 'C', 0
str_KDT db 'K', 'D', 'T', 0

contador_estado0 db 0
contador_estado1 db 0
contador_estado2 db 0

contador_CLT     db 0
contador_RBO     db 0
contador_KSC     db 0
contador_KDT     db 0
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


global calcular_estadisticas

;void calcular_estadisticas(caso_t* arreglo_casos[rdi], int largo[rsi], uint32_t usuario_id[rdx])
calcular_estadisticas:
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

    mov r15, rdi
    mov r14, rsi
    mov r13, rdx

    ;las variables globales no se resetean entre un test y el otro
    mov byte [contador_estado0], 0
    mov byte [contador_estado1], 0
    mov byte [contador_estado2], 0
    mov byte [contador_CLT], 0
    mov byte [contador_RBO], 0
    mov byte [contador_KSC], 0
    mov byte [contador_KDT], 0

    ;como devuelve un estadisticas_t tengo q pedir espacio para generar ese struct
    mov rdi, ESTADISTICAS_SIZE
    call malloc  
    mov rbx, rax                    ;me guardo en rbx la re mierda esta de puntero a  struct 
    ;me tiene las bolas por el piso este ejercicio es espantoso

    cmp r13, 0
    je .contarDeTodosLosCasos

    mov rdi, r15
    mov rsi, r14
    mov rdx, r13
    call calcular_cantidades_ID
    jmp .acomodarEnEstadisticas

    .contarDeTodosLosCasos:
    mov rdi, r15
    mov rsi, r14
    call calcular_cantidades_totales

    .acomodarEnEstadisticas:
    mov al, [contador_CLT] 
    mov byte [rbx + ESTADISTICAS_CLT_OFFSET], al
    mov al, [contador_RBO] 
    mov byte [rbx + ESTADISTICAS_RBO_OFFSET], al
    mov al, [contador_KSC] 
    mov byte [rbx + ESTADISTICAS_KSC_OFFSET], al
    mov al, [contador_KDT] 
    mov byte [rbx + ESTADISTICAS_KDT_OFFSET], al
    mov al, [contador_estado0] 
    mov byte [rbx + ESTADISTICAS_ESTADO0_OFFSET], al
    mov al, [contador_estado1]     
    mov byte [rbx + ESTADISTICAS_ESTADO1_OFFSET], al
    mov al, [contador_estado2] 
    mov byte [rbx + ESTADISTICAS_ESTADO2_OFFSET], al
    
    mov rax, rbx

    mov rbx, [rsp]

    add rsp, 16
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret

;void calcular_cantidades_ID(caso_t* arreglo_casos[rdi], int largo[rsi], uint32_t usuario_id[rdx])
calcular_cantidades_ID:
    push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

    xor r15, r15
    xor r14, r14
    xor r13, r13

    mov r15, rdi
    mov r14, rsi
    mov r13d, edx

    .contarSoloIDTmbMeMato:
        cmp r14, 0
        je .terminoLaTortura            ;si long es 0 termino el array

        mov r8, [r15 + CASO_USUARIO_OFFSET]
        xor r9, r9
        mov r9d, dword [r8 + USUARIO_ID_OFFSET]        ;consigo el id
        cmp r9, r13
        jne .siguienteElem                      ;si el id no es el pedido paso al siguiente
        ;ahora es donde me quiero cortar las bolas
        mov r9w, word[r15 + CASO_ESTADO_OFFSET]
        cmp r9w, 0
        je .sumarEstado0
        cmp r9w, 1
        je .sumarEstado1
        cmp r9w, 2
        je .sumarEstado2
        jmp .siguienteElem

    .seguimosConLosChar:
        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_CLT        ;cargo CLT
        mov rdx, 4              ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatCLT

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_RBO        ;cargo CLT
        mov rdx, 4               ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatRBO

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_KDT        ;cargo CLT
        mov rdx, 4              ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatKDT

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_KSC        ;cargo CLT
        mov rdx, 4               ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatKSC
        jmp .siguienteElem

    .sumarEstado0:
        inc byte [contador_estado0]
        jmp .seguimosConLosChar

    .sumarEstado1:
        inc byte [contador_estado1]
        jmp .seguimosConLosChar

    .sumarEstado2:
        inc byte [contador_estado2]
        jmp .seguimosConLosChar

    .sumarCatCLT:
        inc byte [contador_CLT]
        jmp .siguienteElem

    .sumarCatRBO:
        inc byte [contador_RBO]
        jmp .siguienteElem

    .sumarCatKDT:
        inc byte [contador_KDT]
        jmp .siguienteElem

    .sumarCatKSC:
        inc byte [contador_KSC]
        ;el codigo mas horrible q hice en mi vida
    .siguienteElem:
        dec r14
        add r15, CASO_SIZE
        jmp .contarSoloIDTmbMeMato

    .terminoLaTortura:
   
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret

;void calcular_cantidades_totales(caso_t* arreglo_casos[rdi], int largo[rsi])
calcular_cantidades_totales:
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

    mov r15, rdi
    mov r14, rsi
    mov r13, rdx

    .contarTodos:
        cmp r14, 0
        je .terminoLaTorturatotal            ;si long es 0 termino el array

        ;ahora es donde me quiero cortar las bolas
        xor r9, r9
        ;ahora es donde me quiero cortar las bolas
        mov r9w, word[r15 + CASO_ESTADO_OFFSET]
        cmp r9w, 0
        je .sumarEstado0total
        cmp r9w, 1
        je .sumarEstado1total
        cmp r9w, 2
        je .sumarEstado2total
        jmp .siguienteElemtotal

    .seguimosConLosChartotal:
        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_CLT        ;cargo CLT
        mov rdx, 4               ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatCLTtotal

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_RBO        ;cargo CLT
        mov rdx, 4               ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatRBOtotal

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_KDT        ;cargo CLT
        mov rdx, 4              ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatKDTtotal

        mov rdi, r15          ;cargo el puntero a la categoria en rdi
        mov rsi, str_KSC        ;cargo CLT
        mov rdx, 4               ;leo solo 4 bytes por si sobra alguno
        call strncmp
        cmp rax, 0              ;si rax es 0 son iguales
        je .sumarCatKSCtotal
        jmp .siguienteElemtotal

    .sumarEstado0total:
        inc byte [contador_estado0]
        jmp .seguimosConLosChartotal

    .sumarEstado1total:
        inc byte [contador_estado1]
        jmp .seguimosConLosChartotal

    .sumarEstado2total:
        inc byte [contador_estado2]
        jmp .seguimosConLosChartotal
    
    .sumarCatCLTtotal:
        inc byte [contador_CLT]
        jmp .siguienteElemtotal

    .sumarCatRBOtotal:
        inc byte [contador_RBO]
        jmp .siguienteElemtotal

    .sumarCatKDTtotal:
        inc byte [contador_KDT]
        jmp .siguienteElemtotal

    .sumarCatKSCtotal:
        inc byte [contador_KSC]
        ;el codigo mas horrible q hice en mi vida
    .siguienteElemtotal:
        dec r14
        add r15, CASO_SIZE
        jmp .contarTodos

    .terminoLaTorturatotal:
    mov rbx, [rsp]
    add rsp, 16
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    ret