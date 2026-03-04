# System Programming: Pasaje a Modo Protegido

## Primera parte

1\) Con modo real y modo protegido nos referimos a los modos que definen como opera el procesador.
El modo real opera en 16 bits y puede direccionar hasta 1 MB de memoria con direcciones fisicas de 20 bits. No soporta protección de memoria, multitarea, paginación ni distintos niveles de privilegios. Simula el ambiente de programacion del Intel 8086.
El modo protegido opera en 32 bits y puede direccionar hasta 4GB de memoria. Pero este, a diferencia del modo real, si permite protección de memoria, multitarea, paginación y define niveles de privilegio. Ofrece mas flexibilidad, compatibilidad y buen rendimiento, es el que se usa para sistemas operativos mdoernos.

2\) Debemos hacer el pasaje a modo protegido porque el modo real es muy limitado. Como el modo real solo puede direccionar hasta 1MB, no alcanza para ninguna aplicación o programa actual. No proteje la memoria al no soportar paginación y segmentación, lo que permite que programas puedan sobreescribir sus datos sobre otros y puede generar errores. Además no distingue niveles de privilegio por lo que cualquier programa puede ejecutar cualquier cosa y afecta a la seguridad. Y no soporta multitarea por lo que es poco eficiente.

3\) La GDT es una tabla que contiene descriptores de segmento. Cada descriptor de segmento ocupa 64 bits y tiene una dirección base dividida en tres campos, la parte baja ocupando los bits 16 al 31, la media del 32 al 39 y la alta del 56 al 63. Un limite de tamaño dividio en dos campos, parte baja del bit 0 al 15 y la alta del bit 48 al 51.Y también tiene atributos ocupando los bits restantes. 

El campo Limit define el tamaño del segmento, que es afectado por el campo G(granularidad), cuando G es 0 el limite puede ir hasta 1 MB en incrementos de un byte, y cuando es 1 el limite puede ir hasta 4GB en incrementos de 4 KB.
El campo Base define donde está el byte 0 del segmento (la dirección de inicio), en total forma un valor de 32 bits distribuido por el selector de segmmento.
El campo G es la granularidad, ocupa 1 bit que determina como se calcula el limite del segmento (lo dicho en el campo limit).
El campo P es el bit present, ocupa 1 bit que indica si el segmento está o no presente en memoria.
El campo DPL representa el nivel de privilegios, son 2 bits y puede ir de 0 a 3, donde 0 es el nivel de kernel y el mas prvilegiado. 
El campo S es el tipo de descriptor, un bit que si está en 1 indica que el descriptor es de codigo/datos, y si está en 0 indica que el descriptor es de sistema.

4\) Si queremos especificar un segmento para ejecución y lectura de código, necesitamos setear los bits de type en 1010. El bit 11 en 1 me indica que es un segmento de codigo y el bit 9 en 1 que es de lectura. El bit 10 en 0 me dice que solo es ejecutable desde el mismo nivel de privilegio y el 8 en 0 que todavia no fue accedido. 

5\)
[Planilla GDT completa](https://docs.google.com/spreadsheets/d/1qlnL1uwsvfSm1BFSVRyfebLECcjVtn5FW_1xYhVHT3U/edit?usp=sharing)

6\) En gdt.h, extern gdt_entry_t gdt es un arreglo de entradas GDT que contiene estructuras gdt_entry, donde cada una es un segment descriptor. Y extern gdt_descriptor_t GDT_DESC es una variable que tiene el tamaño del arreglo gdt[] y la direccion de ese arreglo.

10\) LGDT carga los valores de la GDT en un registro Global Descriptor Table Register (GDTR) desde una dirección de memoria. Toma 6 bytes y carga la dirección base donde empieza la GDT y el numero maximo de bytes que ocupa. 
En el código, la estructura que indica estos datos de la GDT es gdt_descriptor_t, que contiene el campo gdt_length y gdt_addr. Y está representada por la variable extern gdt_descriptor_t GDT_DESC. Luego, se inicializa en gdt.c con esta linea "gdt_descriptor_t GDT_DESC = {sizeof(gdt) - 1, (uint32_t)&gdt}"

## Segunda parte

13\) El registro CR0 debe modificarse para pasar de modo real a modo protegido, se establece el bit PE a 1.

15\) Far Jump cambia el CS(selector de segmento) y la direccion de instrucción (IP), y en el código usamos CS_RING_0_SEL de selector de segmento, que apunta al descriptor de codigo nivel 0 en la GDT.

## Tercera parte

21\) El segmento debería tener tamaño 50*80*2. Ese va a ser su límite y la base seria 0x000B8000. Su tipo seria datos de lectura/escritura, la granularidad en 0 (el limite es menor a 1MB por lo que se puede calcular el limite en bytes), DPL en 0, P en 1 y el resto igual a los otros segmentos.

22\)  screen_draw_box dibuja un rectangulo con los mismo caracteres y color, se le pasa por parametro la posicion inicial, la cantidad de filas y columnas, un caracter y sus atributos. Para acceder a la pantalla obtiene un puntero a la direccion de VIDEO y lo usa como una matriz de 50x80, donde cada posicion es una estructura ca.
Cada caracter de la pantalla se representa con la estructura "ca" que contiene el campo caracter y el campo atributo, cada uno un dato de 8bits, por lo que en total ocupa 2 bytes. 

# System Programming: Interrupciones

## Primera parte 

1\)

a. En la macro IDT_ENTRY0, los dos primeros campos representan el offset (donde empieza la rutina de interrupción), y toman de valor de entrada la direccion de la interrupción _isrX para poder manejarla en la ISR.
El campo segsel representa qué selector de segmento usar para ejecutar la rutina.
Type representa el tipo de la compuerta de interrupcion, y el bit D indica si la compuerta es de 16 o 32 bits. En este caso va a ser de 32 por que estamos en modo protegido.
DPL indica el nivel de privilegio y P indica si la rutina está presente en memoria o no.

b. El selector de segmento apropiado seria GDT_CODE_0_SEL porq es codigo de pivilegio 0 y siempre corre a nivel de kernel. Y los atributos usando gate size de 32 bits queda 1110.

c. El selector de segmento va a ser igual GDT_CODE_0_SEL porq por más que puedan dispararse por código no privilegiado, siempre se ejecuta a nivel de kernel. Básicamente se define todo igual menos el DPL que hay que cambiarlo a nivel 3 para que tenga nivel de privilegios de user..

## Segunda parte

En estas rutinas, el prólogo es la instrucción PUSHAD, guarda los registros (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI para poder usarlos sin perder el contexto de ejecución. En el epílogo se usa el POPAD que restaura el contexto de ejecución para continuar con el programa.
El iret marca que finalizó la atención de la interrupción, y se usa iret en vez de ret porque necesitamos que se restauren tambien las flags y privilegios, no solo los registros, de lo contrario podría fallar.

# System Programming: Paginación

## Primera parte

a\) En las estructuras de paginación podemos definir 2 niveles de privilegio, kernel y usuario

b\) Una dirección lógica se traduce a física de la siguiente forma, siendo virt una direccion de 32 bits:

    **pd** := CR3&0xFFFFF000 Dirección de PD 
    **pd index** := (virt >> 22)&0x3FF Índice de PD 
    **pt** := pd[pd index]&0xFFFFF000 Dirección de PT 
    **pt index** := (virt >> 12)&0x3FF Índice de PT 
    **page addr** := pt[pt index]&0xFFFFF000 Dirección de la página 
    **offset** := virt&0xFFF Offset desde el inicio de la página
    **phys** := page addr | offset Dirección fı́sica 

Se limpia el registro CR3 que contiene la direccion base del page directory actual. 
Con los 10 bits mas altos de la direccion lógica se consigue pd index, y se usa para acceder a la page table del page directory ubicada en ese índice.
Con los 10 bits intermedios de la dir lógica se consigue pt index, con el que se accede a una entrada(una página) de la page table.
Cada una de esas entradas contienen la direccion  base física de la página, y sumando el offset (los ultimos 12 bits de la dir lógica) se consigue la dirección física final. 

c\) Las entradas de la tabla de páginas tienen los siguientes atributos:

    D: indica si una página fue escrita.
    A: indica si una página fue leída o escrita.
    PCD: indica si está permitido el uso de cache o no.
    PWT: controla el cache en la escritura, normalmente se escribe en cache y en memoria (bit en 0)
    U/S: indica el nivel de privilegios, bit en 0 indica que la página solo puede ser accedida por el kernel/supervisor, si está en 1 puede ser accedida por el usuario
    R/W: bit en 0 indica solo lectura, si está en 1 indica lectura/escritura
    P: bit en 1 indica que la tabla está presente en memoria, y en 0 que no está presente.

d\) Si los atributos U/S y R/W del directorio y de la tabla de páginas difieren, en general el efecto combinado se queda con el mas dominante o más restrictivo. Si alguno de los dos tiene permisos de supervisor, el combinado va a ser supervisor, y si alguno de los dos es read only, el combinado va a ser read only. Pero si está desctivada la protección de supervisor, siempre va a resultar en permisos read/wirite cuando alguno tenga permisos de supervisor. 

e\) Para el directorio, tablas de páginas y la memoria de una tarea, haría falta pedir una página oara el directorio, una para la tabla y 3 para la memoria de la tarea (una para el stack y dos para el código de la tarea según lo que se supone de la consigna). O sea, pedimos 5 en total. 

g\) El buffer auxiliar de traducción (TLB) es un cache que guarda traducciones a direcciones fisicas recientes para acelerar el proceso de traducción. Es necesario purgarlo para que no queden traducciones cacheadas que puedan generar errores.
Cada traducción en la TLB contiene el numero de la dirección física asociada, el número de la dirección virtual correspondiende, y el bit de validez que indica si tiene una traducción válida.
Al desalojar una entrada de la TLB la homóloga en la tabla original no se ve afectada, no se modifica la page table en memoria.

## Tercera parte

b\) Es necesario mapear y desmapear las paginas de destino y fuente porque el procesador no puede acceder directamente a memorias físicas, entonces se mapean esas direcciones físicas a direcciónes virtuales temporalmente para poder copiarlas, y despues se desmapea para que vuelvan a quedar libres en el kernel y evitar conflictos o complejidad innecesaria.
SRC_VIRT_PAGE y DST_VIRT_PAGE son direcciones virtuales elegidas del espacio de dir virtuales, para mapear las direcciones físicas temporalmente.
Necesitamos obtener el CR3 con rcr3() porque me devuelve la dirección fisica del page directory actual, que necesito para mapear y desmapear correctamente. 

# System Programming: Tareas

## Primera parte

1\) Para que un sistema use dos tareas necesitamos definir dos TSS(task state segment), TSS descriptor y un scheduler, ademas necesitamos una pila de nivel 0 para cada tarea.
Hay que modificar la GDT y agregar 2 entradas, un TSS descriptor para cada tarea. También es necesario que la interrupción del reloj llame al scheduler y configurar los registros:
 - TR(task register), para guardar el selector del descriptor de la tarea actual
 - CR3, que debe apuntar al directorio de cada tarea (cada una tiene un directorio propio)
Cada descriptor de TSS tiene la direccion base donde se encuentra la TSS, su límite y el tipo de segmento.Cada TSS contiene los registros EIP, ESP, EBP, ESP0, selectores de segmento CS, DS, ES, FS, GS, SS, SS0, el registro CR3 y las EFLAGS, información necesaria para poder retomar las tareas despupes de un cambio de contexto.
Las TSS están almacenadas en memoria y los descriptores de TSS para acceder a ellas están almacenados en la GDT, que tiene su direccion base cargada en GDTR.

2\) LLamamos cambio de contexto a interrumpir la ejecucion de una tarea, guardando sus registros en la TSS, para empezar la ejecución de otra. Cuando termina esa tarea, restura los registros y vuelve a retomar la que fué interrumpida. Se suele producir por una interrupción del reloj o por llamados a syscalls.
El TR(task register) almacena un selector que apunta a la TSS de la tarea actual en la GDT, por lo que se puede acceder a los registros guardados para ejecutar tareas despues de un cambio de contexto. 

3\) Antes del primer cambio de contexto, necesitamos inicializar correctamente la TSS de la primera tarea antes de hacer el salto a esta, y debemos setear el registro TR para que apunte al descriptor de TSS de la primera tarea. Este descriptor debe estar como una entrada en la GDT correctamente configurada como tipo TSS con los datos correspondientes de su TSS. Cuando no tenemos tareas para ejecutar, debe haber una tarea idle para que el procesador siempre tenga una tarea valida para ejecutar y que no ejecute tareas que no existen, hasta que encuentre otra tarea para hacer.

4\) El scheduler de un Sistema Operativo decide qué tareas se van a ejecutar. Que use una politica significa que usa reglas para decidir cual es la siguiente tarea que se ejecuta y su duración.

5\) Para que los programas parezcan ejecutarse en simultaneo se usa el scheduler, que va intercalando las tareas con intervalos de tiempo muy cortos y hace parecer que se ejecutan en al mismo tiempo.

9\) Hace falta tener definida la pila de nivel 0 en la tss porque cuando se ejecuta una tarea de usuario y ocurre una interrupción, el procesador cambia a nivel 0 por seguridad, y necesita su propio stack 

## Segunda parte

11\)

a. 
```global _isr32
  
_isr32:
  pushad                ; reserva los registros para guardar contexto de ejecucion
  call pic_finish1      ; le avisa al pic que estoy manejando otra interrupción
  
  call sched_next_task  ; llama al scheduler y devuelve el selector de la proxima tarea
  
  str cx                ; guarda el segmento de tarea actual en CX
  cmp ax, cx            ; lo compara con el selector obtenido en la llamada del scheduler
  je .fin               ; si son iguales, va al final y no hay cambio de tarea
  
  mov word [sched_task_selector], ax    ; si no, actualiza el TR a la tarea nueva
  jmp far [sched_task_offset]           ; salta a la nueva tarea
  
  .fin:
  popad                                 ; recupera los registros y el contexto de ejecucion
  iret                                  ; retorna y finaliza la interrupción
  ```               

b. En jmp far [sched_task_offset] se están leyendo 6 bytes en memoria, 4 corresponden al offset y 2 al selector de la tarea. El offset no tiene ningun efecto ya que es ignorado cuando el selector apunta a un descriptor de TSS.

c. Cuando una tarea vuelve a ser puesta en ejecución, el EIP vuelve al valor de EIP que se habia guardado antes del cambio de tarea.

12\) El scheduler busca de manera circular con la politica round-robin la siguiente tarea a ejecutar. Primero empieza a buscar desde la siguiente a la tarea actual, usando un bucle que eventualmente vuelve a la tarea inicial. Si encuentra alguna tarea runnable sale del bucle y apunta a esa posible siguiente tarea. Despues se obtiene el indice real de esa tarea y si es ejecutable la corre y devuelve su selector, si no encuentra ninguna tarea viva usa la tarea idle. 

## Tercera parte

14\)

a. La funcion tss_gdt_entry_for_task crea un TSS descriptor y lo guarda en la GDT.

b. El desplazamiento a izquierda de gdt_id lo hace porque solo quiere la dirección, no necesita los atributos, entonces lo shiftea para quedarse solo con los bits de la dirección

15\)

a. Las syscalls
b. Porque .data esta en codigo y la tarea no tiene permisos para acceder

16\) 

a. La tarea termina en un loop infinito para asegurar que la tarea se ejecute infinitamente

## Cuarta parte
### Análisis:

18\) Se definen 2 tipos de tarea, la tarea idle para que cuando no hay tareas para hacer el sistema se queda en espera y luego estan las tareas de usuario con su respectivo coódigo.

19\) Escribe en SHARED_SCORE_BASE_VADDR. En esa dirección hay un arary donde se guardan los scores de los 3 pongs, cada uno es un bloque de 64 bits que cada uno dividido en 2 es el score de jugador 1 y 2.