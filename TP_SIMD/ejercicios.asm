; Hace que los accesos a memoria por defecto se compilen como [rip + offset]
; en lugar de [offset_desde_el_0x0].
;
; Ver https://www.nasm.us/doc/nasmdoc7.html#section-7.2.1 para más información
DEFAULT REL

; El valor a poner en los campos `<ejercicio>_hecho` una vez estén completados
TRUE  EQU 1
; El valor a dejar en los campos `<ejercicio>_hecho` hasta que estén completados
FALSE EQU 0

; Offsets a utilizar durante la resolución del ejercicio.
PARTICLES_COUNT_OFFSET    EQU 56 ; ¡COMPLETAR!
PARTICLES_CAPACITY_OFFSET EQU 64 ; ¡COMPLETAR!
PARTICLES_POS_OFFSET      EQU 72 ; ¡COMPLETAR!
PARTICLES_COLOR_OFFSET    EQU 80 ; ¡COMPLETAR!
PARTICLES_SIZE_OFFSET     EQU 88 ; ¡COMPLETAR!
PARTICLES_VEL_OFFSET      EQU 96 ; ¡COMPLETAR!

section .rodata

; La descripción de lo hecho y lo por completar de la implementación en C del
; TP.
global ej_asm
ej_asm:
  .posiciones_hecho: db TRUE
  .tamanios_hecho:   db TRUE
  .colores_hecho:    db TRUE
  .orbitar_hecho:    db FALSE
  ALIGN 8
  .posiciones: dq ej_posiciones_asm
  .tamanios:   dq ej_tamanios_asm
  .colores:    dq ej_colores_asm
  .orbitar:    dq ej_orbitar_asm

; Máscaras y valores que puede ser útil cargar en registros vectoriales.
;
; ¡Agregá otras que veas necesarias!
ALIGN 16
ceros:      dd  0.0,    0.0,     0.0,    0.0
unos:       dd  1.0,    1.0,     1.0,    1.0
inv_mask: dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
ceros_int: dd 0, 0, 0, 0


section .text

; Actualiza las posiciones de las partículas de acuerdo a la fuerza de
; gravedad y la velocidad de cada una.
;
; Una partícula con posición `p` y velocidad `v` que se encuentra sujeta a
; una fuerza de gravedad `g` observa lo siguiente:
; ```
; p := (p.x + v.x, p.y + v.y)
; v := (v.x + g.x, v.y + g.y)
; ```
;
; void ej_posiciones(emitter_t* emitter, vec2_t* gravedad[RSI]);
ej_posiciones_asm:
	mov rcx, [rdi + PARTICLES_COUNT_OFFSET]
	mov rdx, [rdi + PARTICLES_POS_OFFSET]
	mov r8,  [rdi + PARTICLES_VEL_OFFSET]

	;preparo la gravedad
	movq xmm0, [rsi]	;me queda xmm0 = [g.x g.y _ _]
	movlhps xmm0, xmm0  ;queda xmm0 = [g.x g.y g.x g.y] cpia 64 bits bajos a la parte alta

	xor r9, r9

	.loop:
		cmp r9, rcx
		je .finDelLoop			;me gusta mas verificar aca xd
        ; Cuerpo del loop

		movdqu xmm1, [rdx + r9*8]	;xmm1[p.x p.y p.x p.y] cargo posicion de 2 particulas, *8 porq cada particula ocupa 8 bytes
		movdqu xmm2, [r8 + r9*8]	;xmm2[v.x v.y v.x v.y] cargo velocidad de 2 particulas, lo mismo

		;calculo nueva posicion, avanza usando la velocidad actual
		addps xmm1, xmm2		;xmm1 = [p.x+v.x p.y+v.y | p.x+v.x p.y+v.y]	y tengo la nueva posicion
		;calculo nueva velocidad, usando laa gravedad
		addps xmm2, xmm0		;xmm2 = [v.x+g.x v.y+g.y | v.x+g.x v.y+g.y]

		;actualizo en la posicion donde va cada una
		movdqu [rdx + r9*8], xmm1
		movdqu [r8 + r9*8], xmm2

		;paso a las siguientes particulas
		add r9, 2 ; ¿Cantidad de partículas por loop?
		jmp .loop

	.finDelLoop:
	ret

; Actualiza los tamaños de las partículas de acuerdo a la configuración dada.
;
; Una partícula con tamaño `s` y una configuración `(a, b, c)` observa lo
; siguiente:
; ```
; si c <= s:
;   s := s * a - b
; sino:
;   s := s - b
; ```
;
; void ej_tamanios(emitter_t* emitter[rdi], float a[rsi], float b[rdx], float c[rcx]);
ej_tamanios_asm:

	mov r8,  [rdi + PARTICLES_SIZE_OFFSET]
	mov r9,  [rdi + PARTICLES_COUNT_OFFSET]

	;preparo los floats
	;como me los pasan como floats ya se guardan en xmm0, 1 y 2
	shufps xmm0, xmm0, 0x00 ;copio el float al resto de posiciones para tener [a a a a]
	movdqu xmm6, xmm0		;guardo a en otro registro para uasr xmm0 en blend despues
	shufps xmm1, xmm1, 0x00  ;b
	shufps xmm2, xmm2, 0x00  ;c

	movdqu xmm7, [inv_mask]

	xor r10, r10 
	
	.loopyloop: 
		cmp r10, r9 
		je .saleDelLoop

		movdqu xmm15, [r8 + r10*4]	;cargo 4 tamaños de particulas, multiplico por 4 porq floats ocupan 4 bytes
		
		;calculo primero las dos posibilidades
		movdqu xmm14, xmm15
		mulps xmm14, xmm6		;tamañis*a
		subps xmm14, xmm1		;tamaños*a - b

		movdqu xmm13, xmm15
		subps xmm13, xmm1		;tamaños - b

		;ahora tengo en  xmm14 = tamaños*a - b y en xmm13 = tamaños - b

		;me preparo una mascara para ver cuales tamaños son menores a c
		movdqu xmm3, xmm15
		movdqu xmm0, xmm2

		cmpleps xmm0, xmm3		;comparo si c <= tamaños,  
								;me hace xmm0[0] <= xmm3[0], si se cumple me da todo en 1's, sino todo en 0's
								;me queda una mascara en xmm0, tengo 1's en los tamaños >= c	

		;blendvps xmm_dest, xmm_src, xmm_mask
		blendvps xmm13, xmm14, xmm0
		;si xmm0 esta en 1 toma de xmm14, sino toma de xmm13

		;lo ubico donde va
		movdqu [r8 + r10*4], xmm13

		add r10, 4					;ahora opero de a 4 particulas 
		jmp .loopyloop

	.saleDelLoop:
	ret

; Actualiza los colores de las partículas de acuerdo al delta de color
; proporcionado.
;
; Una partícula con color `(R, G, B, A)` ante un delta `(dR, dG, dB, dA)`
; observa el siguiente cambio:
; ```
; R = R - dR
; G = G - dG
; B = B - dB
; A = A - dA
; si R < 0:
;   R = 0
; si G < 0:
;   G = 0
; si B < 0:
;   B = 0
; si A < 0:
;   A = 0
; ```
;
; void ej_colores(emitter_t* emitter, SDL_Color a_restar);
ej_colores_asm:
	;rgba
	;128 abgr|abgr|abgr|abgr 

	mov rcx, [rdi + PARTICLES_COUNT_OFFSET]
	mov rdx, [rdi + PARTICLES_COLOR_OFFSET]

	movd xmm0, esi
	pshufd xmm0, xmm0, 0x00		;me reservo en xmm0 = [color | color | color | color]

	;ahora puedo separar todos los colores para hacer las operaciones como me gusta a mi :) 
	movdqu xmm1, xmm0
	pslld xmm1, 24
	psrld xmm1, 24				;me quedo en xmm1 con [c-rojo | c-rojo | c-rojo | c-rojo]

	movdqu xmm2, xmm0
	pslld xmm2, 16
	psrld xmm2, 24				;me quedo en xmm2 con [c-verfe | c-verde | c-verde | c-verde]
	
	movdqu xmm3, xmm0
	pslld xmm3, 8
	psrld xmm3, 24				;me quedo en xmm3 con [c-azul | c-azul | c-azul | c-azul]

	movdqu xmm4, xmm0
	psrld xmm4, 24				;me quedo en xmm4 con [c-alf | c-alf | c-alf | c-alf]
	
	movdqu xmm7, [ceros_int]

	xor r9, r9

	.looppp:
		cmp r9, rcx
		je .final

		movdqu xmm15, [rdx + r9 * 4]		;cargo colores de 4 particulas tengo abgr|abgr|abgr|abgr

		;separo rojos
		movdqu xmm14, xmm15
		pslld xmm14, 24
		psrld xmm14, 24
		;separo verdes
		movdqu xmm13, xmm15
		pslld xmm13, 16
		psrld xmm13, 24
		;separo azules
		movdqu xmm12, xmm15
		pslld xmm12, 8
		psrld xmm12, 24
		;separo alfa
		movdqu xmm11, xmm15
		psrld xmm11, 24

		;ahora quiero hacer rojo - c_rojo y asi con todos los colores
		;y de cada uno quedarme con el max entre esa resta y 0
		;hago max(rojo - c_rojo, 0)
		psubd xmm14, xmm1
		pmaxsd xmm14, xmm7
		;hago max(verde - c_verde, 0)
		psubd xmm13, xmm2
		pmaxsd xmm13, xmm7
		;hago max(axul - c_azul, 0)
		psubd xmm12, xmm3
		pmaxsd xmm12, xmm7
		;hago max(alfa - c_alfa, 0)
		psubd xmm11, xmm4
		pmaxsd xmm11, xmm7

		;despues devuelvo cada color a su lugar y los junto
		;xmm14 que es el rojo ya esta en su lugar
		pslld xmm13, 8				;00g0
		pslld xmm12, 16				;0b00
		pslld xmm11, 24				;a000

		;los juntos
		por xmm14, xmm13			;00gr
		por xmm14, xmm12			;0bgr
		por xmm14, xmm11			;abgr

		;ubico los colores donde van
		movdqu [rdx + r9 * 4], xmm14

		;paso a las siguientes particulas
		add r9, 4 
		jmp .looppp

	.final:
	ret

; Calcula un campo de fuerza y lo aplica a cada una de las partículas,
; haciendo que tracen órbitas.
;
; La implementación ya está dada y se tiene en el enunciado una versión más
; "matemática" en caso de que sea de ayuda.
;
; El ejercicio es implementar una versión del código de ejemplo que utilice
; SIMD en lugar de operaciones escalares.
;
; void ej_orbitar(emitter_t* emitter, vec2_t* start, vec2_t* end, float r);
ej_orbitar_asm:
	ret
