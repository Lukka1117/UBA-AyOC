extern malloc
extern free

section .rodata
; Acá se pueden poner todas las máscaras y datos que necesiten para el ejercicio
section .text
; Marca un ejercicio como aún no completado (esto hace que no corran sus tests)
FALSE EQU 0
; Marca un ejercicio como hecho
TRUE  EQU 1

; Marca el ejercicio 1A como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - optimizar
global EJERCICIO_2A_HECHO
EJERCICIO_2A_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

; Marca el ejercicio 1B como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - contarCombustibleAsignado
global EJERCICIO_2B_HECHO
EJERCICIO_2B_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

; Marca el ejercicio 1C como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - modificarUnidad
global EJERCICIO_2C_HECHO
EJERCICIO_2C_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

;########### ESTOS SON LOS OFFSETS Y TAMAÑO DE LOS STRUCTS
; Completar las definiciones (serán revisadas por ABI enforcer):
ATTACKUNIT_CLASE EQU 0
ATTACKUNIT_COMBUSTIBLE EQU 12
ATTACKUNIT_REFERENCES EQU 14
ATTACKUNIT_SIZE EQU 16

global optimizar
optimizar:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = mapa_t           mapa						[rdi]
	; r/m64 = attackunit_t*    compartida				[rsi]
	; r/m64 = uint32_t*        fun_hash(attackunit_t*)	[rdx]

	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	sub rsp, 16		;;con 16 queda alineada
	xor r9, r9
	mov [rsp], r9	;reservo r9 en la pila

	;primero preparo registros y reservo en no volatiles
	xor r15, r15
	xor r14, r14
	xor r13, r13
	xor r12, r12
	xor rbx, rbx

	mov r15, rdi		;me gusrdo la matriz en r15
	mov r14, rsi		;la unit pasada por param en r14
	mov rbx, rdx		;y la funcion hash en rbx

	;primero calculo el hash de la unit pasada y lo reservo par comparar despues
	xor rdi, rdi
	mov rdi, rsi		;la muevo a rdi para hacer el call
	call rbx			;me queda en rax el hash
	mov r13, rax 		;ahora tengo el hash en r13


	mov r12, 255
	imul r12, r12		;en r12 tengo 255x255, para usar de limite del ciclo

	.ciclooo:
		mov r9, [rsp]			;cargo el iterador guardado en r9
		xor rdi, rdi
		mov rdi, [r15 + r9*8]		;rdi tiene el primer puntero a struct
		cmp rdi, 0
		je .siguienteUnit
		;ahora quiero conseguir la unit actual, y calcular su hash
		
		call rbx					;me quedo en rax el hash de la unit actual

		cmp rax, r13
		jne .siguienteUnit 			;si no son iguales, paso al siguiente elemento

		mov r9, [rsp]				;vuelvo a cargar el iterador por si me lo cambio el call
		;si son iguales, 
		;sumo uno a las refs de la unit nueva
		inc byte [r14 + ATTACKUNIT_REFERENCES]		;referencias++
		;resto 1 a las refs de la unit actual
		mov rdi, [r15 + r9*8]						;ahora tengo la anterior en rdi
		dec byte [rdi + ATTACKUNIT_REFERENCES]		;referencias de la anterior--
		;redirecciono el puntero a la pasada por parametros
		mov [r15 + r9*8], r14

		;si las referencias de la actual son 0, llamo a free, sino sigo a la siguiente
		cmp byte [rdi + ATTACKUNIT_REFERENCES], 0
		jne .siguienteUnit

		call free

	.siguienteUnit:
		inc qword [rsp]			;iterador++ en la pila
		dec r12
		cmp r12, 0
		jne .ciclooo			;si el contador es mayor a 0 sigue

	add rsp, 16					;libera la pila
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret

global contarCombustibleAsignado
contarCombustibleAsignado:
	; r/m64 = mapa_t           mapa						[rdi]
	; r/m64 = uint16_t*        fun_combustible(char*)	[rsi]

	;recorro el mapa, por cada unit consigno su clase y se le paso a la funcion
	;me devuelve el combustible base de la unidad
	;consulto el combustible total de la unidad y le resto el base
	;lo que queda es lo que le asigno el jugador
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	sub rsp, 16
	mov qword [rsp], 0			;reservo r9 para un acumulador de combustible asignado
	;limpio registros
	xor r15, r15
	xor rbx, rbx
	;reasigno a no volatiles
	mov r15, rdi
	mov rbx, rsi

	xor r14, r14
	mov r14, 255
	imul r14, r14			;en r14 tengo el contador

	xor r13, r13			;uso r13 de iterador/offset

	.cyclops:
		mov r12, [r15 + r13*8]		;tomo la primera unit en r12
		cmp r12, 0					;si es 0, paso a la siguiente
		je .siguientee

		;si no, obtengo su compustible base y su combustible total
		lea rdi, [r12]		;obtengo la clase de la unit en rdi 
		call rbx								;tengo en rax el combustible base de la unit
		
		;ahora obtengo el combustible de la unit actual
		xor r8, r8										;lo tengo q limpiar si o si
		mov r8w, word [r12 + ATTACKUNIT_COMBUSTIBLE]	
		;resto combustible total - combustible base
		sub r8, rax
		;lo sumo al acumulado
		add [rsp], r8

	.siguientee:
		inc r13
		cmp r13, r14							;si iterador >= r14, sale
		jl .cyclops
	
	mov rax, [rsp]

	add rsp, 16
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret

global modificarUnidad
modificarUnidad:
	; r/m64 = mapa_t           mapa								[rdi]
	; r/m8  = uint8_t          x								[rsi] sil
	; r/m8  = uint8_t          y								[rdx] dl
	; r/m64 = void*            fun_modificar(attackunit_t*)		[rcx]
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	xor rbx, rbx

	mov rbx, rcx		;funcion en rbx
	;necesito tomar mapa[x][y]
	;mapa[x][y] = mapa_base + (x * NUM_COLS + y)*8 porq cada elem de map ocupa 8 bytes (puntero)
	movzx rsi, sil				;guardo en rdi los 8 bits de x extendidos en 0 en rsi
	movzx rdx, dl				;guardo y en rdx

	imul rsi, 255			; x * columnas
	add rsi, rdx			;(x * xolumnas) + y
	shl rsi, 3				;lo anterior * 8

	add rdi, rsi		;en rdi tengo mapa[x][y]
	
	mov r15, rdi		;mapa[x][y], aca quiero guardar la unidad modificada o la copia modificada
	xor r14, r14
	mov r14, [r15]		;obtengo la unidad a modificar

	cmp r14, 0
	je .finn 

	;Si se modifica una unidad que está compartiendo instancia por una optimización,
	;se debe crear una nueva instancia individual para esta 
	;o sea que si las referencias > 1 tengo q copiar la unidad
	xor r12, r12
	mov r12b, byte [r14 + ATTACKUNIT_REFERENCES]
	cmp r12, 1
	jle .noCopiar

	;si la tengo q copiar, tomo la unidad actual y le resto una referencia
	dec byte [r14 + ATTACKUNIT_REFERENCES]
	;para hacer una copia necesito pedir memoria suficiente para crear una unidad
	mov rdi, ATTACKUNIT_SIZE
	call malloc					;me queda en rax el puntero a la uniad nueva
	;ahora voy moviendo los campos de la unidad
	mov rdi, [r14]		;copio los primeros 8 bytes
	mov [rax], rdi
	mov rdi, [r14 + 8]	;copio del byte 8 al 16
	mov [rax + 8], rdi
	;tengo q setear la ref en 1
	mov byte [rax + ATTACKUNIT_REFERENCES], 1

	;la ubico en el mapa
	mov [r15], rax

	;ahora la modifico
	.noCopiar:
	mov rdi, [r15]			;preparo la unidad en rdi
	call rbx				;llamo a modificar unit

	;no hayq hacer mas nada porq devuelve void
	.finn:
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret
