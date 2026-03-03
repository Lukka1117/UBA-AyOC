extern malloc
extern calloc
extern sleep
extern wakeup
extern create_dir_entry

section .rodata
; Acá se pueden poner todas las máscaras y datos que necesiten para el ejercicio
sleep_name: DB "sleep", 0
wakeup_name: DB "wakeup", 0

section .text
; Marca un ejercicio como aún no completado (esto hace que no corran sus tests)
FALSE EQU 0
; Marca un ejercicio como hecho
TRUE  EQU 1

; Marca el ejercicio 1A como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - init_fantastruco_dir
global EJERCICIO_1A_HECHO
EJERCICIO_1A_HECHO: db TRUE ; Cambiar por `TRUE` para correr los tests.

; Marca el ejercicio 1B como hecho (`true`) o pendiente (`false`).
;
; Funciones a implementar:
;   - summon_fantastruco
global EJERCICIO_1B_HECHO
EJERCICIO_1B_HECHO: db TRUE; Cambiar por `TRUE` para correr los tests.

;########### ESTOS SON LOS OFFSETS Y TAMAÑO DE LOS STRUCTS
; Completar las definiciones (serán revisadas por ABI enforcer):
DIRENTRY_NAME_OFFSET EQU 0
DIRENTRY_PTR_OFFSET EQU 16
DIRENTRY_SIZE EQU 24

FANTASTRUCO_DIR_OFFSET EQU 0
FANTASTRUCO_ENTRIES_OFFSET EQU 8
FANTASTRUCO_ARCHETYPE_OFFSET EQU 16
FANTASTRUCO_FACEUP_OFFSET EQU 24
FANTASTRUCO_SIZE EQU 32

; void init_fantastruco_dir(fantastruco_t* card[rdi]);
global init_fantastruco_dir
init_fantastruco_dir:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = fantastruco_t*     card
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12
	sub rsp, 16
    mov qword [rsp], rbx

	xor r15, r15
	mov r15, rdi		;me reservo el puntero a mi carta

	;primero quiero setear en mi carta las dir entries en 2 porq tengo sleep y wakeup
	;no me pide todavia el face up y el archetype
	;entries
	mov word[r15 + FANTASTRUCO_ENTRIES_OFFSET], 2

	;mov qword [r15 + FANTASTRUCO_ENTRIES_OFFSET], 0
	;haciendo calloc ya ne setea todo en 0, por lo que no necesito setear el archetype en null
	;seteo el face up en 1
	mov byte [r15 + FANTASTRUCO_FACEUP_OFFSET], 1
	
	;para inicializar el dir, me pide que incluya el de la habilidad wake y sleep
	;o sea que tiene dos entries
	;_dir es directory_t, que es directory_entry_t**, necesito memoria para un array de punteros a structs
	;puedo hacer malloc(2*8) para obtener un puntero con memoria suficiente para guardar 2 punteros
	xor rdi, rdi
	xor rsi, rsi
	mov rdi, 2
	mov rsi, 8			;2*8
	call calloc

	xor r14, r14
	mov r14, rax			;me guardo en rax el puntero a mi directory_

;directory_entry_t* create_dir_entry(char* ability_name, void* ability_ptr)
;quiero usar esa funcion para crear mis dos entry_t, la de wakeup y la de sleep
	;entonces quiero obtener en rdi el ability_name y en rsi el ptr
	xor r12, r12
	xor r13, r13 

	mov rdi, sleep_name			;me toma la direccion de sleep name en rdi
	mov rsi, sleep				;direccion de funcion sleep en rsi? sino uso lea rsi, [sleep]

	call create_dir_entry
	mov r12, rax
 
	mov rdi, wakeup_name		;nombre de wakuep en rdi
	mov rsi, wakeup				;direccion de funcion sleep en rsi

	call create_dir_entry
	mov r13, rax
	;ahora tengo en r12 el puntero a entry_t de sleep y el puntero a entry_t de wakeup
	;muevo cada uno a su ubicacion en rbx
	mov [r14], r12
	mov [r14 + 8], r13

	;seteoo la di en mi carta
	mov qword [r15 + FANTASTRUCO_DIR_OFFSET], r14

	mov rbx, [rsp]
    add rsp, 16
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
	ret ;No te olvides el ret!

; fantastruco_t* summon_fantastruco();
global summon_fantastruco
summon_fantastruco:
	; Esta función no recibe parámetros
	;devuelve un puntero a fantastruco_t
	;necesito pedir memoria para ubicar un struct fantastruco_t
	push rbp
	mov rbp, rsp
	push r15
	push r14
	push r13
	push r12

	mov rdi, 1
	mov rsi, FANTASTRUCO_SIZE
	call calloc
	mov r15, rax			;me guardo en r15 el puntero a mi carta
	
	mov rdi, r15
	call init_fantastruco_dir 
	;primero quiero setear en mi carta las dir entries en 2 porq tengo sleep y wakeup
	;no me pide todavia el face up y el archetype
	;entries
; 	mov word[r15 + FANTASTRUCO_ENTRIES_OFFSET], 2

; 	;haciendo calloc ya ne setea todo en 0, por lo que no necesito setear el archetype en null
; 	;seteo el face up en 1
; 	mov byte [r15 + FANTASTRUCO_FACEUP_OFFSET], 1
; 	;para inicializar el dir, me pide que incluya el de la habilidad wake y sleep
; 	;o sea que tiene dos entries
; 	;_dir es directory_t, que es directory_entry_t**, necesito memoria para un array de punteros a structs
; 	;puedo hacer malloc(2*8) para obtener un puntero con memoria suficiente para guardar 2 punteros
; 	xor rdi, rdi
; 	xor rsi, rsi
; 	mov rdi, 2
; 	mov rsi, 8			;2*8
; 	call calloc

; 	xor r14, r14
; 	mov r14, rax			;me guardo en rax el puntero a mi directory_
; ;directory_entry_t* create_dir_entry(char* ability_name, void* ability_ptr)
; ;quiero usar esa funcion para crear mis dos entry_t, la de wakeup y la de sleep
; 	;entonces quiero obtener en rdi el ability_name y en rsi el ptr
; 	xor r12, r12
; 	xor r13, r13 

; 	lea rdi, [sleep_name]			;me toma la direccion de sleep name en rdi
; 	lea rsi, [sleep]				;direccion de funcion sleep en rsi? sino uso lea rsi, [sleep]

; 	call create_dir_entry
; 	mov r12, rax
 
; 	lea rdi, [wakeup_name]		;nombre de wakuep en rdi
; 	lea rsi, [wakeup]				;direccion de funcion sleep en rsi

; 	call create_dir_entry
; 	mov r13, rax
; 	;ahora tengo en r12 el puntero a entry_t de sleep y el puntero a entry_t de wakeup
; 	;muevo cada uno a su ubicacion en rbx
; 	mov [r14], r12
; 	mov [r14 + 8], r13

; 	;seteoo la di en mi carta
; 	mov qword [r15 + FANTASTRUCO_DIR_OFFSET], r14

	mov rax, r15

    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
	ret ;No te olvides el ret!
