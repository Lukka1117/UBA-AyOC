; ** por compatibilidad se omiten tildes **
; ==============================================================================
; TALLER System Programming - Arquitectura y Organizacion de Computadoras - FCEN
; ==============================================================================

%include "print.mac"

global start


; COMPLETAR - Agreguen declaraciones extern según vayan necesitando
extern screen_draw_layout
extern idt_init
extern mmu_init_kernel_dir
extern mmu_init_task_dir
extern tss_init
extern tasks_screen_draw
extern sched_init
extern tasks_init
; Macros
extern GDT_DESC
extern IDT_DESC
extern GDT_IDX_TASK_INITIAL
extern GDT_IDX_TASK_IDLE

; COMPLETAR - Definan correctamente estas constantes cuando las necesiten
%define CS_RING_0_SEL 8   
%define DS_RING_0_SEL 3 << 3  
%define COLOR_DEFAULT 0b01010010
%define INITIAL_TASK_TR 11 << 3
%define IDLE_TASK_TR    12 << 3
%define DIVISOR 0x50 //65536 para 18.206 Hz
BITS 16
;; Saltear seccion de datos
jmp start

;;
;; Seccion de datos.
;; -------------------------------------------------------------------------- ;;
start_rm_msg db     'Iniciando kernel en Modo Real'
start_rm_len equ    $ - start_rm_msg

start_pm_msg db     'Iniciando kernel en Modo Protegido'
start_pm_len equ    $ - start_pm_msg

start_idt_msg db     'Iniciada la IDT'
start_idt_len equ   $ - start_idt_msg

idle_tss_ptr:
    dd 0                 ; offset (ignorado en task switch)
    dw IDLE_TASK_TR      ; selector del TSS Idle
; PIC
extern pic_reset
extern pic_enable

;;
;; Seccion de código.
;; -------------------------------------------------------------------------- ;;

;; Punto de entrada del kernel.
BITS 16
start:
    ; ==============================
    ; ||  Salto a modo protegido  ||
    ; ==============================

    ; COMPLETAR - Deshabilitar interrupciones (Parte 1: Pasaje a modo protegido)
    cli

    ; Cambiar modo de video a 80 X 50
    mov ax, 0003h
    int 10h ; set mode 03h
    xor bx, bx
    mov ax, 1112h
    int 10h ; load 8x8 font

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO REAL (Parte 1: Pasaje a modo protegido)
    ; (revisar las funciones definidas en print.mac y los mensajes se encuentran en la
    ; sección de datos)
    print_text_rm start_rm_msg, start_rm_len, COLOR_DEFAULT, 5, 10

    ; COMPLETAR - Habilitar A20 (Parte 1: Pasaje a modo protegido)
    ; (revisar las funciones definidas en a20.asm)
    call A20_enable

    ; COMPLETAR - los defines para la GDT en defines.h y las entradas de la GDT en gdt.c
    ; COMPLETAR - Cargar la GDT (Parte 1: Pasaje a modo protegido)
    lgdt [GDT_DESC]

    ; COMPLETAR - Setear el bit PE del registro CR0 (Parte 1: Pasaje a modo protegido)
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; COMPLETAR - Saltar a modo protegido (far jump) (Parte 1: Pasaje a modo protegido)
    ; (recuerden que un far jmp se especifica como jmp CS_selector:address)
    ; Pueden usar la constante CS_RING_0_SEL definida en este archivo
    jmp CS_RING_0_SEL:modo_protegido

BITS 32
modo_protegido:
    ; COMPLETAR (Parte 1: Pasaje a modo protegido) - A partir de aca, todo el codigo se va a ejectutar en modo protegido
    ; Establecer selectores de segmentos DS, ES, GS, FS y SS en el segmento de datos de nivel 0
    ; Pueden usar la constante DS_RING_0_SEL definida en este archivo
    mov ax , DS_RING_0_SEL
    mov ds , ax
    mov es , ax
    mov fs , ax
    mov gs , ax
    mov ss , ax         

    ; COMPLETAR - Establecer el tope y la base de la pila (Parte 1: Pasaje a modo protegido)i
    mov ebp, 0x25000
    mov esp, ebp

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO PROTEGIDO (Parte 1: Pasaje a modo protegido)
    print_text_pm start_pm_msg, start_pm_len, COLOR_DEFAULT, 20,40 
    ; COMPLETAR - Inicializar pantalla (Parte 1: Pasaje a modo protegido)
    call screen_draw_layout 
    
    ; ===================================
    ; ||     (Parte 3: Paginación)     ||
    ; ===================================

    ; COMPLETAR - los defines para la MMU en defines.h
    ; COMPLETAR - las funciones en mmu.c
    ; COMPLETAR - reemplazar la implementacion de la interrupcion 88 (ver comentarios en isr.asm)
    ; COMPLETAR - La rutina de atención alineacion de stack 32 bits del page fault en isr.asm
    ; COMPLETAR - Inicializar el directorio de paginas
    call mmu_init_kernel_dir

    ; COMPLETAR - Cargar directorio de paginas 
    mov cr3, eax

    ; COMPLETAR - Habilitar paginacion 
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax


    ; ========================
    ; ||  (Parte 4: Tareas) ||
    ; ========================

    ; COMPLETAR - reemplazar la implementacion de la interrupcion 88 (ver comentarios en isr.asm)
    ; COMPLETAR - las funciones en tss.c
    ; COMPLETAR - Inicializar tss
    call tss_init

    ; COMPLETAR - Inicializar el scheduler
    call sched_init

    ; COMPLETAR - Inicializar las tareas

    call tasks_init

    ; ===================================
    ; ||   (Parte 2: Interrupciones)   ||
    ; ===================================

    ; COMPLETAR - las funciones en idt.c

    ; COMPLETAR - Inicializar y cargar la IDT
    call idt_init
    lidt [IDT_DESC]
    print_text_pm start_idt_msg, start_idt_len, COLOR_DEFAULT, 0, 0 

    ; COMPLETAR - Reiniciar y habilitar el controlador de interrupciones (ver pic.c)
    call pic_reset ; remapear PIC
    call pic_enable ; habilitar PIC
    sti ; habilitar interrupciones

    mov ax, 0x500

    out 0x40, al

    rol ax, 8

    out 0x40, al
    ; COMPLETAR (Parte 4: Tareas)- Cargar tarea inicial

    call tasks_screen_draw
    mov ax, INITIAL_TASK_TR
    ltr ax

    ; COMPLETAR - Habilitar interrupciones (!! en etapas posteriores, evaluar si se debe comentar este código !!)
    
    ; NOTA: Pueden chequear que las interrupciones funcionen forzando a que se
    ;       dispare alguna excepción (lo más sencillo es usar la instrucción
    ;       `int3`)
    ;int3

    ; COMPLETAR - Probar Sys_call (para etapas posteriores, comentar este código)

    ; COMPLETAR - Probar generar una excepción (para etapas posteriores, comentar este código)
    
    ; ========================
    ; ||  (Parte 4: Tareas)  ||
    ; ========================
    
    ; COMPLETAR - Inicializar el directorio de paginas de la tarea de prueba

    ; COMPLETAR - Cargar directorio de paginas de la tarea

    ; COMPLETAR - Restaurar directorio de paginas del kernel

    ; COMPLETAR - Saltar a la primera tarea: Idle
    jmp far [idle_tss_ptr]
    
    ; Ciclar infinitamente 
    mov eax, 0xFFFF
    mov ebx, 0xFFFF
    mov ecx, 0xFFFF
    mov edx, 0xFFFF
    jmp $

;; -------------------------------------------------------------------------- ;;

%include "a20.asm"
