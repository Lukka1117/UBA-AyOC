extern strcmp
; Definiciones comunes
TRUE  EQU 1
FALSE EQU 0

; Identificador del jugador rojo
JUGADOR_ROJO EQU 1
; Identificador del jugador azul
JUGADOR_AZUL EQU 2

; Ancho y alto del tablero de juego
tablero.ANCHO EQU 10
tablero.ALTO  EQU 5

; Marca un OFFSET o SIZE como no completado
; Esto no lo chequea el ABI enforcer, sirve para saber a simple vista qué cosas
; quedaron sin completar :)
NO_COMPLETADO EQU -1

extern strcmp

;########### ESTOS SON LOS OFFSETS Y TAMAÑO DE LOS STRUCTS
; Completar las definiciones (serán revisadas por ABI enforcer):
carta.en_juego EQU 0
carta.nombre   EQU 1
carta.vida     EQU 14
carta.jugador  EQU 16
carta.SIZE     EQU 18

tablero.mano_jugador_rojo EQU 0
tablero.mano_jugador_azul EQU 8
tablero.campo             EQU 16
tablero.SIZE              EQU tablero.campo + tablero.ANCHO*tablero.ALTO*8

accion.invocar   EQU 0
accion.destino   EQU 8
accion.siguiente EQU 16
accion.SIZE      EQU 24

; Variables globales de sólo lectura
section .rodata

; Marca el ejercicio 1 como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - hay_accion_que_toque
global EJERCICIO_1_HECHO
EJERCICIO_1_HECHO: db TRUE

; Marca el ejercicio 2 como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - invocar_acciones
global EJERCICIO_2_HECHO
EJERCICIO_2_HECHO: db TRUE

; Marca el ejercicio 3 como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - contar_cartas
global EJERCICIO_3_HECHO
EJERCICIO_3_HECHO: db TRUE

section .text

; Dada una secuencia de acciones determinar si hay alguna cuya carta tenga un
; nombre idéntico (mismos contenidos, no mismo puntero) al pasado por
; parámetro.
;
; El resultado es un valor booleano, la representación de los booleanos de C es
; la siguiente:
;   - El valor `0` es `false`
;   - Cualquier otro valor es `true`
;
; ```c
; bool hay_accion_que_toque(accion_t* accion, char* nombre);
; ```
global hay_accion_que_toque
hay_accion_que_toque:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = accion_t*  accion	[rdi]
	; r/m64 = char*      nombre	[rsi]
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	xor r15, r15
	xor r14, r14
	;reservo mis parametros
	mov r15, rdi				;lista de acciones en r15
	mov r14, rsi				;nombre buscado en r14

	.ciclojdjksd:
		cmp r15, 0
		je .noExisteCarta				;si mi actual es null, no hay mas acciones  y no encontre la carta

		;ahora quiero tomar de mi accion_t la carta
		xor r13, r13
		mov r13, qword [r15 + accion.destino]		;untero a mi carta en r13
		;de la carta extraigo su nombre
		xor rdi, rdi
		lea rdi, qword [r13 + carta.nombre]		;guardo la direccion del primer char en rdi
		;preparo ls dos nombres a comparar en rdi, rsi
		mov rsi, r14
		call strcmp							;me devuelve 0 si son iguales
		;si son iguales seteo rax en 1 y salgo porq existe alguna carta con el mismo nombre
		cmp rax, 0
		je .existeCarta
		;si no son iguales, paso a la siguiente accion_t
		mov r15, qword [r15 + accion.siguiente]					;tomo la siguiente accion
		jmp .ciclojdjksd
	
	.existeCarta:
		mov rax, 1
		jmp .finalll

	.noExisteCarta:
		xor rax, rax

	.finalll:
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret

; Invoca las acciones que fueron encoladas en la secuencia proporcionada en el
; primer parámetro.
;
; A la hora de procesar una acción esta sólo se invoca si la carta destino
; sigue en juego.
;
; Luego de invocar una acción, si la carta destino tiene cero puntos de vida,
; se debe marcar ésta como fuera de juego.
;
; Las funciones que implementan acciones de juego tienen la siguiente firma:
; ```c
; void mi_accion(tablero_t* tablero, carta_t* carta);
; ```
; - El tablero a utilizar es el pasado como parámetro
; - La carta a utilizar es la carta destino de la acción (`accion->destino`)
;
; Las acciones se deben invocar en el orden natural de la secuencia (primero la
; primera acción, segundo la segunda acción, etc). Las acciones asumen este
; orden de ejecución.
;
; ```c
; void invocar_acciones(accion_t* accion, tablero_t* tablero);
; ```
global invocar_acciones
invocar_acciones:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = accion_t*  accion			[rdi]
	; r/m64 = tablero_t* tablero		[rsi]
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

	mov r15, rdi 			;guardo lista de accion en r15
	mov r14, rsi			;guardo tablero en rsi

	.cicloInvocaciones:
		cmp r15, 0
		je .noHayMasAcciones	;ssi actual es null, me quede sin acciones

		;xor rbx, rbx
		mov rbx, qword[r15 + accion.destino]		;tomo la carta destino en r8
		xor r9, r9
		mov r9b, byte[rbx + carta.en_juego]		;tomo si esta en juego

		cmp r9, 0
		je .siguienteAccion					;si no esta en juego no la puedo llamar, aca igual no se si
											;pasar al siguiente o cortar porq no puedo invocar a todas

		;nose si mirar si la vida es 0 o no, no entendi si le podia llamar la accion con vida 0
		;pero como en una aclaracion dice algo de (independientemente si vida = 0 antes o no) no la miro
		
		;si la carta esta en juego, tomo accion_fn_t*, preparo el tablero y la carta en los respectivos registros
		;y la invoco
		mov r13, qword[r15 + accion.invocar]
		mov rdi, r14
		mov rsi, rbx
		call r13
		;despues de invocar, verifico la vida de la carta
		xor r10, r10
		mov r10w, word[rbx + carta.vida]
		cmp r10, 0
		jle .sacarDeJuego
		;si la vida no es cero, puedo seguir sin hacer nada
		mov r15, qword[r15 + accion.siguiente]
		jmp .cicloInvocaciones

	.siguienteAccion:
		mov r15, qword[r15 + accion.siguiente]
		jmp .cicloInvocaciones

	.sacarDeJuego:
		mov byte[rbx + carta.en_juego], 0			;si la vida es 0 despues de invocar la saco del juego
		jmp .siguienteAccion						;paso a la ssiguiente accion

	.noHayMasAcciones:
	add rsp, 8
	pop rbx
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret

; Cuenta la cantidad de cartas rojas y azules en el tablero.
;
; Dado un tablero revisa el campo de juego y cuenta la cantidad de cartas
; correspondientes al jugador rojo y al jugador azul. Este conteo incluye tanto
; a las cartas en juego cómo a las fuera de juego (siempre que estén visibles
; en el campo).
;
; Se debe considerar el caso de que el campo contenga cartas que no pertenecen
; a ninguno de los dos jugadores.
;
; Las posiciones libres del campo tienen punteros nulos en lugar de apuntar a
; una carta.
;
; El resultado debe ser escrito en las posiciones de memoria proporcionadas
; como parámetro.
;
; ```c
; void contar_cartas(tablero_t* tablero, uint32_t* cant_rojas, uint32_t* cant_azules);
; ```
global contar_cartas
contar_cartas:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = tablero_t* tablero		[rdi]
	; r/m64 = uint32_t*  cant_rojas		[rsi]
	; r/m64 = uint32_t*  cant_azules	[rdx]
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

	mov r15, rdi
	mov r14, rsi
	mov r13, rdx
	;los puntteros estan inicializados en 0? ver en gdb

	mov dword[r14], 0
	mov dword[r13], 0

	;quiero conseguir el campo del tablero, que despues puedo usar para iterar
	lea r12, qword[r15 + tablero.campo]			;leo la direc del primer puntero del array
	;me reservo un iterador, que tambienn uso de offset para recorrer el tablero
	mov rbx, tablero.ALTO
	imul rbx, tablero.ANCHO			;me queda en rbx 50, que es la cantidad de espacios en el tablero
	
	xor r8d, r8d				;contador de rojos
	xor r9d, r9d				;contador de azules

	.cicloTablero:
		cmp rbx, 0
		je .meQuedeSinTablero			;si rbx llega a 0 revise todo el tablero

		mov r10, qword[r12]		;puntero a carta en r10
		cmp r10, 0
		je .siguienteCarta	;si es null, no hay carta en el tablero
		;si hay carta, obtengo el jugador
		xor r11, r11
		mov r11b, byte[r10 + carta.jugador]
		cmp r11, JUGADOR_ROJO
		je .sumoAlRojo
		cmp r11, JUGADOR_AZUL
		je .sumoAlAzul
		;si no es ninguno de los dos jugadores, no sumo a ninguno y paso a la siguiente carta
	
	.siguienteCarta:
		dec rbx
		add r12, 8		;paso al siguiente puntero del tablero
		jmp .cicloTablero

	.sumoAlRojo:
		add r8d, 1
		jmp .siguienteCarta

	.sumoAlAzul:
		add r9d, 1
		jmp .siguienteCarta

	.meQuedeSinTablero
	mov dword[r14], r8d
	mov dword[r13], r9d

	add rsp, 8
	pop rbx
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret
