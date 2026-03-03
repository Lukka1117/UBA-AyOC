extern strcmp
global invocar_habilidad

; Completar las definiciones o borrarlas (en este ejercicio NO serán revisadas por el ABI enforcer)
DIRENTRY_NAME_OFFSET EQU 0
DIRENTRY_PTR_OFFSET EQU 16
DIRENTRY_SIZE EQU 24

FANTASTRUCO_DIR_OFFSET EQU 0
FANTASTRUCO_ENTRIES_OFFSET EQU 8
FANTASTRUCO_ARCHETYPE_OFFSET EQU 16
FANTASTRUCO_FACEUP_OFFSET EQU 24
FANTASTRUCO_SIZE EQU 32

section .rodata
; Acá se pueden poner todas las máscaras y datos que necesiten para el ejercicio

section .text

; void invocar_habilidad(void* carta[rdi], char* habilidad[rsi]);
invocar_habilidad:
	; Te recomendamos llenar una tablita acá con cada parámetro y su
	; ubicación según la convención de llamada. Prestá atención a qué
	; valores son de 64 bits y qué valores son de 32 bits o 8 bits.
	;
	; r/m64 = void*    card ; Vale asumir que card siempre es al menos un card_t*
	; r/m64 = char*    habilidad

; Si la habilidad está implementada por la carta actual (se encuentra una con el nombre pasado por parámetro en el directorio), se llama a la implementación correspondiente.
; Si la habilidad no está implementada por la carta actual, se revisa si está implementada en su arquetipo
; Si la habilidad está implementada en su arquetipo, llama a dicha implementación
; De no ser el caso, revisará en el arquetipo de dicha carta (el arquetipo de una carta podría tener a su vez un arquetipo, y así) hasta llegar a una carta que:
; tenga la implementación (en cuyo caso se llama) o
; no tenga arquetipo asociado a quien consultar (en cuyo caso se termina la ejecución sin realizar nada).
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
	
	mov r15, rdi		;reservo mi carta en r15
	mov r14, rsi		;reservo habilidad en r14

	.buscar:
		;quiero ver primero si la habilidad esta adentro de el directory
		;obtengo dir_entries
		xor r13, r13
		mov r13w, word [r15 + FANTASTRUCO_ENTRIES_OFFSET]
		;obtengo directory_t de mi carta
		xor r12, r12
		mov r12, qword [r15 + FANTASTRUCO_DIR_OFFSET]

		xor rbx, rbx				;indice

		.buscarEnEntries:
			cmp r13, 0
			je .buscoEnArchetype		;si termine de recorrer las entries, no estaba la hab y la busco en arche
		
		;por cada entrie tomo habilidad_name y la comparo con habilidad
		;si tengo directory_t en r12, tengo directory_entries**
			mov r9, qword [r12 + rbx]							;obtengo directory_entrie_t*
			mov rdi, r9											;cargo en rdi la direccion al nombre de habilidad
			mov rsi, r14										;cargo la q busco en rsi
			call strcmp
			cmp rax, 0
		;si coincide llamar a funcion
			je .llamarHabilidad
		;sino ir a la siguiente entrie
			dec r13
			add rbx, 8
			jmp .buscarEnEntries

	.buscoEnArchetype:
		mov r10, qword [r15 + FANTASTRUCO_ARCHETYPE_OFFSET]
		cmp r10, 0											;si archetype es null, no hace nada y termuna
		jz .termino
	;si es null no hago nada
	;si no vuelvo a llamar a invocar habilidad con la nueva carta y misma habilidad
		mov r15, qword [r15 + FANTASTRUCO_ARCHETYPE_OFFSET]			;cargo la carta nueva en rdi
		;mov rsi, r14												;la habilidad en r14
		;lo quise hacer recursivo y me gano hkjsdhfs volvemos a la iteracion normal de siempre
		;call invocar_habilidad
		jmp .buscar
	
	.llamarHabilidad:
		mov rcx, qword [r12 + rbx]			;obtengo directory_entrie_t* de vuelta
		mov rcx, qword [rcx + DIRENTRY_PTR_OFFSET]	;obtengo la funcion en r8
		mov rdi, r15						;pongo mi carta en rdi
		call rcx

		dec r13
		add rbx, 8
		jmp .buscarEnEntries

	.termino:
	mov rbx, [rsp]
    add rsp, 16
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp
	ret ;No te olvides el ret!
