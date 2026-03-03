extern malloc

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
;   - es_indice_ordenado
global EJERCICIO_1A_HECHO
EJERCICIO_1A_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

; Marca el ejercicio 1B como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - indice_a_inventario
global EJERCICIO_1B_HECHO
EJERCICIO_1B_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

;########### ESTOS SON LOS OFFSETS Y TAMAÑO DE LOS STRUCTS
; Completar las definiciones (serán revisadas por ABI enforcer):
ITEM_NOMBRE EQU 0
ITEM_FUERZA EQU 20
ITEM_DURABILIDAD EQU 24
ITEM_SIZE EQU 28

;; La funcion debe verificar si una vista del inventario está correctamente 
;; ordenada de acuerdo a un criterio (comparador)

;; bool es_indice_ordenado(item_t** inventario, uint16_t* indice, uint16_t tamanio, comparador_t comparador);

;; Dónde:
;; - `inventario`: Un array de punteros a ítems que representa el inventario a
;;   procesar.
;; - `indice`: El arreglo de índices en el inventario que representa la vista.
;; - `tamanio`: El tamaño del inventario (y de la vista).
;; - `comparador`: La función de comparación que a utilizar para verificar el
;;   orden.
;; 
;; Tenga en consideración:
;; - `tamanio` es un valor de 16 bits. La parte alta del registro en dónde viene
;;   como parámetro podría tener basura.
;; - `comparador` es una dirección de memoria a la que se debe saltar (vía `jmp` o
;;   `call`) para comenzar la ejecución de la subrutina en cuestión.
;; - Los tamaños de los arrays `inventario` e `indice` son ambos `tamanio`.
;; - `false` es el valor `0` y `true` es todo valor distinto de `0`.
;; - Importa que los ítems estén ordenados según el comparador. No hay necesidad
;;   de verificar que el orden sea estable.

global es_indice_ordenado
es_indice_ordenado:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = item_t**     inventario [rdi]
	; r/m64 = uint16_t*    indice	  [rsi]
	; r/m16 = uint16_t     tamanio	   [dx]
	; r/m64 = comparador_t comparador   [rcx]

	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	cmp dx, 1
	je .estaOrdenado
	;limpio los registros
	xor r15, r15
	xor r14, r14
	xor r13, r13
	xor rbx, rbx
	;reservo en no volatiles
	mov r15, rdi
	mov r14, rsi
	mov r13w, dx
	mov rbx, rcx

	;en el ciclo, quiero conseguir el valor de idx[i] y de idx[i+1]
	;luego quiero ver inventario[idx[i]] e inventario[idx[i+1]]
	;comparalos, y si da true continuar al siguiente, si no, no estan ordenados y sale del ciclo
	xor r12, r12 ;limpio r12 para usar de contador

	;como trabajo con i e i-1 quiero que mi ciclo itere i-1 veces, asi q le resto 1 al tam
	dec r13

	.loopyloop:
		cmp r12w, r13w 
		jge .estaOrdenado    ;si el iterador es menor o igual que tam - 1, ya recorrio toda la lista y deberia dar true)

		movzx r8, WORD [r14 + r12*2] ;tomo indice[i] y lo guardo en r8, multiplico por 2 porq cada idx ocupa 2 bytes
		movzx r9, WORD [r14 + r12*2 + 2] ;le sumo 2 y pasa a ñps siguientes 2 bytes, consigo en i+1

		mov r10, [r15 + r8*8]	;tomo invent[idx[i]], mult por 8 por ser un array de punteros, cada uno ocupa 8
		mov r11, [r15 + r9*8]	;lo mismo pero con idx[i+1]

		;ahora los quiero comparar
		;necesito ubicarlos en rdi y rsi para llamar a la funcion correctamente
		mov rdi, r10
		mov rsi, r11

		call rbx		;lamo al comparador, me queda T o F en rax

		cmp rax, 0
		jz .noOrdenado	;como false es 0, si rax = 0 entonces algo no esta bien ordenado
		;si es distinto de 0, sigue al siguiente elemento
		inc r12			; i++
		jmp .loopyloop

	.estaOrdenado:
		xor rax, rax
		mov rax, 1
		jmp .fin

	.noOrdenado:
		xor rax, rax

	.fin:
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret

;; Dado un inventario y una vista, crear un nuevo inventario que mantenga el
;; orden descrito por la misma.

;; La memoria a solicitar para el nuevo inventario debe poder ser liberada
;; utilizando `free(ptr)`.

;; item_t** indice_a_inventario(item_t** inventario, uint16_t* indice, uint16_t tamanio);

;; Donde:
;; - `inventario` un array de punteros a ítems que representa el inventario a
;;   procesar.
;; - `indice` es el arreglo de índices en el inventario que representa la vista
;;   que vamos a usar para reorganizar el inventario.
;; - `tamanio` es el tamaño del inventario.
;; 
;; Tenga en consideración:
;; - Tanto los elementos de `inventario` como los del resultado son punteros a
;;   `ítems`. Se pide *copiar* estos punteros, **no se deben crear ni clonar
;;   ítems**

global indice_a_inventario
indice_a_inventario:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = item_t**  inventario 	[rdi]
	; r/m64 = uint16_t* indice		[rsi]
	; r/m16 = uint16_t  tamanio		[dx]
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	;como el res es un array nuevo, voy a tener que pedirle memoria a malloc
	;va a ser un arrray de punteros que apuntan a item_t, o se que necesito pedirle tamaño*8
	;porq quiero tamaño elemento y cada puntero ocupa 8 bytes
	;preparo registros y memoria para pedir
	xor r15, r15
	xor r14, r14
	xor r13, r13

	mov r15, rdi
	mov r14, rsi
	mov r13w, dx

	;quiero terminar con tamaño*8 en rdi
	shl dx, 3
	xor rdi, rdi	
	mov di, dx		;tengo tamaño*8 en rdi

	call malloc 	;ahora tengo en rax el puntero a mi array nuevo

	;para el loop, quiero conseguir indice[i] para hacer inventario[indice[i]] y eso guardalo en rax
	xor r12, r12	;limpio para usar de iterador
	xor r10, r10    ;lo uso de offset para el res

	.loopyy:
		cmp r12w, r13w	
		jge .fin2			;si iterador >= tamaño, termine de ver todos los elementos

		movzx r8, word [r14 + r12*2]	;aca consigo indice[i]
		mov r9, [r15 + r8*8] 				;inventario[indice[i]]

		mov [rax + r10], r9

		inc r12							;i++
		add r10, 8						;paso a la posicion del siguiente elemento de res

		jmp .loopyy

	.fin2:
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret
