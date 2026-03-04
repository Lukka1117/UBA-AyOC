uint32_t task_id_to_cr3(int8_t task_id) {
    uint16_t index = sched_tasks[task_id].selector >> 3;
    gdt_entry_t* task_descriptor = &gdt[index];
    tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                               (task_descriptor->base_23_16 << 16) | 
                               (task_descriptor->base_15_0));
    return task_tss->cr3;
}

//dir fisica del pd de la tarea a partir de su selector
uint32_t task_sel_to_cr3(int16_t selector) {
    uint16_t index = selector >> 3;
    gdt_entry_t* task_descriptor = &gdt[index];
    tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                            (task_descriptor->base_23_16 << 16) | 
                            (task_descriptor->base_15_0));
    return task_tss->cr3;
}

//como traducir de virtual a fisico?
//siguiendo el camin ode la traduccion:
//cr3 -> PD -> PD entry -> PT -> PT entry -> phys

paddr_t mmu_virt_to_phys(uint32_t cr3, vaddr_t virt) {
    pd_entry_t *pd = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);
    uint32_t pd_index = VIRT_PAGE_DIR(virt);
   
    pt_entry_t *pt = (pt_entry_t *)(pd_entry.pt << 12);
    uint32_t pt_index = VIRT_PAGE_TABLE(virt);

    paddr_t phys = pt[pt_index].page << 12;
    //uint32_t offset = VIRT_PAGE_OFFSET(virt);

    return phys //+ offset;
}   //la version de la resolucion de ellos es sin lo del offset no se porq

//obtener direccion fisica del page directory que solo mapea area del kernel, en mmu.c
//una tarea del kernel necesita acceder a su propio codigo y estructuras, que tipicamente se carga en un aera 
//de memoria con mapeo identidad
paddr_t create_cr3_for_kernel_task() {
    //crea un pd nuevo y lo configura con un identity mapping del area del kernel
    paddr_t task_page_dir = mmu_next_free_kernel_page();
    zero_page(task_page_dir);
    //inicializar pd en 0 es buena practica porq toda entrada no utilizada queda en 0 (no presente)
    //identity mapping 
    for (uint32_t i = 0; i < IDENTITY_MAPPING_END; i += PAGE_SIZE) {
        mmu_map_page((uint32_t)task_page_dir, i, i, MMU_P | MMU_W); //seteamos en W para que el codigo kernel pueda escribir en su propia memoria
    }
    return task_page_dir;   //devuelve la dir fisica del pd creado para ser almacenado en el cr3 de la TSS de la tarea
} 

//cuando quiero leer el bit accessed o dirty de una pte necesito esto
pt_entry_t* mmu_get_pte_for_task(uint16_t task_id, vaddr_t virt) {
    uint32_t* cr3 task_id_to_cr3(task_id);          // dir fisica d esu pd

    uint32_t pd_index = VIRT_PAGE_DIR(virt);
    pd_entry_t *pd = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);

    uint32_t pt_index = VIRT_PAGE_TABLE(virt);
    pd_entry_t *pt = pd[pd_index].pt << 12; //contiene dir phy de la pt

    return (pt_entry_t*)&pt[pt_index];
}

//obtener tss a partir de un segselector
tss_t* get_tss_from_selector(int16_t selector) {
    uint16_t index = selector >> 3;
    gdt_entry_t* task_descriptor = &gdt[index];
    tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                            (task_descriptor->base_23_16 << 16) | 
                            (task_descriptor->base_15_0));
    return task_tss;
}
/*
Obtener el Índice GDT: El selector de 16 bits no es un índice directo.
Se utiliza selector >> 3 para eliminar los 3 bits inferiores (el RPL y el TI) y obtener el índice de la GDT (index).
Acceder al Descriptor: La línea gdt_entry_t* task_descriptor = &gdt[index]; usa ese índice para encontrar el Descriptor de la 
TSS correspondiente dentro de la tabla GDT (Global Descriptor Table).
Ensamblar la Dirección Base (Física): Un Descriptor TSS de 64 bits almacena la dirección base de la estructura tss_t en memoria, 
pero está dividida en tres campos no contiguos: base_15_0, base_23_16, y base_31_24.
El código utiliza shifts (<< 24, << 16) y la operación OR bit a bit (|) para reconstruir la dirección base física completa de 32 bits de la estructura tss_t.
*/

#define MMU_PWT (1 << 3) //controla write back(0) o write through(1), writeback hace que la cpu use su cache para escribir
#define MMU_PCD (1 << 4) //cache disable, si esta en 1 deshabilita la cache para esa pagina
#define MMU_A   (1 << 5) //accessed bit, se setea automaticamente cuando la pagina es accedida (leida o escrita)
#define MMU_D   (1 << 6) //dirty bit, se setea automaticamente cuando la pagina es escrita
#define MMU_PS  (1 << 7) //page size, si esta en 1 la entrada mapea una pagina de 4MB en vez de 4KB (solo en PDE, en PTE es control avanzado de caching)
#define MMU_G   (1 << 8) //global page, si esta en 1 la pagina no se invalida en un cambio de contexto y no se purga el tlb (cambio de cr3), 
//requiere que el bit PGE del registro CR4 este en 1

// En defines.h o un archivo similar
#define ONE_MB_IN_BYTES (1024 * 1024)
#define MAX_MEM_PER_TASK (4 * ONE_MB_IN_BYTES) // 4,194,304 bytes

4MB en hexadecimal = 0x00400000

Unidad,Cifra en Bytes,Definición C
Kilobyte (1 KB),"1,024", #define ONE_KB_IN_BYTES (1024)
Megabyte (1 MB),"1,048,576", #define ONE_MB_IN_BYTES (1024 * ONE_KB_IN_BYTES)
Gigabyte (1 GB),"1,073,741,824", #define ONE_GB_IN_BYTES (1024 * ONE_MB_IN_BYTES)
Página Estándar (4 KB),"4,096", #define PAGE_SIZE_IN_BYTES (4 * ONE_KB_IN_BYTES)



/*
1. Lazy Allocation / Page Fault
El Page Fault Handler es el único lugar donde se implementa la asignación perezosa.Detección: 
La CPU detecta P=0 en la PTE y llama al handler.Validación: El handler revisa el rango virtual (CR2) con la tabla de reservas 
(esMemoriaReservada).Asignación: Si es válido, llama a mmu_next_free_user_page(), limpia la página (zero_page), y llama a mmu_map_page() 
para mapear el $\text{PADDR}$ al VADDR donde ocurrió el fault.Reanudación: Devuelve true para reanudar la tarea.
2. Sincronización y BloqueoPara una syscall que bloquea (crear_pareja, opendevice):La función C debe setear el estado de la tarea actual 
a TASK_BLOCKED.El handler de ensamblador debe llamar a sched_next_task y realizar un Task Switch (jmp far) a la siguiente tarea elegible.
El desbloqueo siempre ocurre en un evento asíncrono (IRQ 40 o la syscall de otra tarea), que cambia el estado a TASK_RUNNABLE.

La programación de sistemas se centra en la gestión directa de los recursos del hardware mediante el sistema operativo (kernel), 
lo que requiere un profundo conocimiento de la arquitectura x86 y el modelo de Modo Protegido.

El corazón de tu kernel se basa en tres pilares interconectados para la gestión de tareas, memoria y el control de la CPU: 
la Segmentación (GDT/TSS), la Paginación (CR3/PD/PT) y el Manejo de Interrupciones (IDT).

La Segmentación establece una capa de protección inicial y define las unidades lógicas de memoria (segmentos). La GDT (Global Descriptor Table) 
es la tabla principal que almacena descriptores, incluyendo los esenciales descriptores flat (plano) que tu kernel utiliza para que la 
Dirección Virtual sea igual a la Dirección Lineal, delegando toda la complejidad de la gestión de memoria a la Paginación. 
Relacionada directamente con las tareas está la TSS (Task State Segment), que es una estructura vital que almacena el estado completo de la CPU 
para cada tarea (como EAX, EIP, ESP), sus pilas de kernel (ESP0, SS0), y, fundamentalmente, el registro CR3. 
El Selector de la TSS es el identificador que el scheduler utiliza para hacer referencia a una tarea y es clave para el proceso de Task Switch (cambio de tarea).

La Paginación es el mecanismo que implementa la protección y el aislamiento de memoria. El registro CR3 es el ancla de este sistema, 
ya que almacena la dirección física de la Page Directory (PD) de la tarea actual. El PD, a su vez, apunta a las Page Tables (PTs), y cada entrada de la PT 
(PTE) contiene la Dirección Física (PADDR) de una página de 4 KiB, junto con los bits de atributos (como Presente (P), R/W (W), Usuario/Supervisor (U), 
y los bits de estado Accessed (A) y Dirty (D)). Estos bits son cruciales en tu kernel, ya que el Bit Dirty se usa para el garbage_collector 
y el Bit Accessed se usa para la task_killer para monitorizar el uso de memoria. Las funciones como mmu_map_page utilizan la Dirección Virtual (VADDR) 
de la tarea para extraer los índices de las tablas (VIRT_PAGE_DIR, VIRT_PAGE_TABLE) y así navegar desde el CR3 hasta la PTE.

El Manejo de Interrupciones es el sistema de control que permite al hardware (IRQs como el reloj o el teclado) y al software (Syscalls) solicitar servicios del kernel. 
La IDT (Interrupt Descriptor Table) es la tabla que mapea cada interrupción (índice 0 a 255) a un handler de ensamblador. Las Syscalls (llamadas al sistema) 
son interrupciones de software iniciadas por tareas de Nivel 3 y configuradas en la IDT con DPL=3, pero el código del handler se ejecuta siempre en el contexto 
de Nivel 0 (el kernel). El handler en ensamblador es el único lugar donde se puede manipular la pila para pasar parámetros (push EAX) y devolver resultados 
(mov [ESP + offset], EAX).

La relación entre estas estructuras es vital: el Selector TSS (GDT) te da la base de la TSS, la cual contiene el CR3 (Paginación), que define el mapa de memoria 
que es crucial para resolver los Page Faults (IDT) que ocurren durante el Lazy Allocation. Tu kernel utiliza funciones auxiliares como task_selector_to_CR3 para 
navegar esta jerarquía y obtener la dirección física necesaria para manipular la memoria o el estado de una tarea inactiva.*/

Pila al momento del jmp

|            | +                     ┌───> PUSHAD:
| SS_3       | ───┐                   │           | EAX | +0x1C
| ESP_3      |    │                   │           | ECX | +0x18
| EFLAGS     |    ├ Apilado por el    │           | EDX | +0x14 [5]
| CS_3       |    │ cambio de nivel   │           | EBX | +0x10 [4]
| EIP_3      |    │ de privilegio     │           | ESP | +0x0C [3]
| Error Code | ───┘                   │           | EBP | +0x08 [2]
| PUSHAD     | ───────────────────────────┘           | ESI | +0x04 [1]
| ...        |                                   | EDI | +0x00 [0] <─ esp al momento
|            | -                                                       del jmp far

"ejemplo de uso"
/*
Necesitamos revisar el valor de edx que tenía la tarea al momento de ser
interrumpida por el clock. La tarea no está actualmente en ejecución, entonces dónde
está su información? En la TSS.
Qué es la TSS? En qué momento se carga? En qué momento se actualiza?
• La TSS se actualiza cuando hacemos jmp far ( jmp sel:offset ), por lo que en la TSS
se guardan los valores de los registros al momento del jmp.
edx es un registro no volátil, por lo que si recordamos la rutina de reloj de antes,
después de los llamados a funciones de C ( sched_next_task ) es probable que no
tenga el mismo valor que nos había llegado.
Es decir, TSS.edx no tiene el valor que buscamos.
Sin embargo, el valor de edx que nos interesa vive aún. Dónde? en la pila.
Podemos acceder al valor guardado en la pila a través de la TSS? 33
Sí, usando TSS.esp[5]

Contexto General (Apilado por PUSHAD)
Esta sección se apila manualmente por el handler de ensamblador (_isrXX) mediante la instrucción PUSHAD. 
El objetivo es guardar los registros de propósito general de la tarea de usuario antes de que el código C del kernel los utilice y potencialmente los sobrescriba.

PUSHAD guarda 8 registros de 32 bits, empezando por EDI (el offset 0x00) hasta EAX (el offset 0x1C).
Ubicación del ESP: El puntero de pila (ESP) al momento de comenzar el handler (jmp far) apunta al valor del Error Code 
(si existe) o al EIP\_3 (si no existe código de error).
Función Clave: El kernel manipula esta sección, especialmente el slot de EAX (en el offset 0x1C), para inyectar el valor de retorno de una syscall 
antes de restaurar el contexto con POPAD e IRET.
*/

Page fault

/*
Esta región se asignará bajo demanda. Acceder a memoria no asignada dentro de la región reservará sólo las páginas necesarias para cumplir ese acceso.
Es una forma de decir que voy a necesitar memoria en esa región, pero no voy a pedirla toda de una vez, sino que la voy a ir pidiendo a medida que la necesite. O sea q 
cuando una tarea pida acceso, si la página no está mapeada, se genera un page fault y el manejador de page fault se encarga de asignar la página en ese momento.

Mecanismos de Asignación Bajo Demanda (Lazy)
Estos escenarios implican que se concede memoria virtual, pero la asignación de memoria física (RAM) se retrasa hasta el primer uso, lo cual es el trabajo 
exclusivo del Page Fault Handler.
Asignación Perezosa (Lazy Allocation):
"El kernel no reservará la memoria física hasta que realmente se acceda a dicha memoria."
"Acceder a memoria no asignada dentro de la región reservará sólo las páginas necesarias para cumplir ese acceso."
"La asignación es gradual, es decir, solamente se asignará una única página física por cada acceso a la memoria reservada."
"Si la dirección virtual corresponde a las reservadas por la tarea, el kernel deberá asignar memoria física a la dirección virtual que corresponda."

Páginas compartidas:
"Al asignarse memoria, ésta estará limpia (todos ceros)." (Esto se hace en el Page Fault Handler después de asignar el PADDR).
🛡️ Acceso con Permisos Condicionales
Cuando los permisos de una página deben cambiar dinámicamente o son asimétricos, el Page Fault Handler puede ser el mecanismo de implementación:
Copy-on-Write (CoW):
"La tarea hija debe obtener una copia de la página solo al intentar escribir en ella." (El handler detecta el fallo de escritura en una página de sololecturay crea la copia).
Permisos Asimétricos (Su Caso de Parejas):
"Sólo la tarea que crea la pareja puede escribir en el área de memoria especificada." (El handler aplica el permiso R/W al líder y R/O al seguidor).
❌ Detección de Fallos y Seguridad
El Page Fault Handler es necesario para interceptar y gestionar errores que afectan el mapa de memoria de una tarea.
Detección de Acceso Ilegal:
"Si el acceso es incorrecto porque la tarea está leyendo una dirección que no le corresponde, el kernel debe desalojar tarea inmediatamente." (El handler detecta la dirección
virtual no mapeada/no reservada y mata la tarea).
Gestión de Swap (Paginación a Disco):
"Si la página no está presente, debe ser cargada desde el disco." (El handler detecta P=0 y gestiona el swap-in).
📌 Datos Necesarios para el Handle
Para implementar estas lógicas, el Page Fault Handler requiere leer ciertos datos que el hardware le proporciona automáticamente o que debe obtener del contexto:
Dirección Virtual (CR2): La dirección que causó el fault (se pasa a la función como parámetro virt).
Información del Error: El código de error (en la pila, para saber si fue por escritura, permisos, o nivel de privilegio).
Contexto de la Tarea: El CR3 de la tarea actual (rcr3()) y su ID (current_task).
*/

CR2 y page fault

El registro CR2 es fundamental para el manejo de una excepción de Page Fault (ISR 14). El procesador lo utiliza para informar al sistema operativo (kernel) 
sobre la dirección exacta que causó el fallo.
🧭 Función del Registro CR2.
El registro CR2 (Page Fault Linear Address Register) almacena la dirección virtual 
(lineal) a la que el programa intentó acceder cuando ocurrió el Page Fault
.¿Cómo funciona? Cuando el subsistema de paginación detecta que el Bit Presente (P) en una entrada del Page Directory (PDE) o una Page Table (PTE) 
está en cero, detiene la ejecución del programa y genera la excepción. Antes de transferir el control al handler del kernel, la CPU carga el 
valor de la dirección virtual fallida en CR2.
El kernel utiliza el valor de CR2 para saber qué página virtual debe mapear o qué acceso fue ilegal:
Mapeo: En casos de Lazy Allocation o swapping, el kernel lee CR2 para saber qué dirección virtual debe traducir y mapear a una nueva dirección física.
Diagnóstico: El kernel usa CR2 para buscar la entrada de la tabla de paginación (PD[CR2 index]) y analizar por qué ocurrió el fallo 
(permisos, no presente, etc.).
🛡️ Proceso de Atención al Page FaultLa atención al Page Fault es el proceso de software que el kernel ejecuta 
para resolver el fallo de memoria, el cual ocurre inmediatamente después de que el procesador genera la interrupción (INT 14).
Activación de la Excepción: Una tarea intenta acceder a una dirección virtual con Bit Presente (P=0) en sus tablas de paginación.Acción del 
Procesador (Hardware): El procesador:
Carga la dirección virtual fallida en el registro CR2.
Apila el código de error y el contexto de la tarea en la pila del kernel.
Salta al handler definido en la IDT para la interrupción 14 (_isr14).
Llamada al Handler de C: 
El handler en ensamblador guarda los registros y llama a la función de C: page_fault_handler(vaddr_t virt). 
El argumento virt se obtiene leyendo el valor de CR2 (usando la instrucción mov eax, cr2 en ensamblador).
Lógica del Kernel (Software): Dentro de page_fault_handler:El kernel lee CR2 (el parámetro virt) y el código de error 
(de la pila) para diagnosticar la causa.Si es una falla resoluble (Lazy Allocation): El kernel asigna una página física, 
la inicializa, la mapea a la dirección virtual de CR2 con los permisos correctos (mmu_map_page), y devuelve true.
Si es una falla fatal (Acceso Ilegal): El kernel mata a la tarea y devuelve false
.Reanudación: Si el handler retorna true, el Task Switch se evita, y la CPU reanuda la tarea exactamente en la 
instrucción que causó el fallo. Dado que la memoria ya está mapeada, la instrucción se ejecuta con éxito.

Jerarquias y accesos;

/*
Jerarquía de Estructuras y Acceso a Datos
Tu kernel se basa en una jerarquía de acceso a la información:
Identificación de Tarea (task_id y Selector):
Para identificar a la tarea que está corriendo o la que se quiere manipular, se utiliza el task_id (el índice en el array del scheduler).
La variable global current_task siempre contiene el task_id de la tarea actualmente activa, que es tu punto de partida en la mayoría de las syscalls.
El Selector TSS (ej., sched_tasks[task_id].selector) es el identificador de 16 bits que la CPU utiliza para localizar el estado completo de la tarea. 
Para obtener el Selector de una tarea i basta con acceder a sched_tasks[i].selector.
Contexto de la Tarea (TSS):
La función get_tss_from_selector(selector) o su equivalente (task_id_to_cr3) es crucial, ya que permite al kernel localizar la estructura TSS de cualquier 
tarea desde su selector.
Datos Críticos: De la TSS se extraen los registros guardados (EAX, EDX) para lógica de prioridades o syscalls y, más importante, el CR3.
Base de la Memoria (CR3):
El CR3 es la Dirección Física (PADDR) del Page Directory (PD) de la tarea.
Obtención: Si la tarea es la actual, usas rcr3() (leer el registro). Si es otra tarea (ej., en el garbage collector o task killer), usas task_selector_to_CR3(selector).
El CR3 es el primer argumento requerido por funciones que manipulan el mapa de memoria, como mmu_map_page.
Traducción y Mapeo (PD/PT):
Una vez que tienes el CR3, puedes navegar las tablas usando las macros VIRT_PAGE_DIR y VIRT_PAGE_TABLE junto con la Dirección Virtual (VADDR) que deseas traducir o mapear.
*/

Logica de asignacion de memoria;

/*Uso de la Función,Contexto,Propósito
mmu_next_free_kernel_page(),Infraestructura del Kernel,"Para asignar las estructuras del kernel, como Page Directories (PDs), 
Page Tables (PTs), o las pilas de nivel 0 (ESP0)."
mmu_next_free_user_page(),Memoria de la Tarea,"Para asignar páginas de código, datos y pila de usuario, o cualquier memoria solicitada 
por las tareas a través de malloco (Lazy Allocation)."

Regla para el Mapeo
Siempre que una función necesite establecer una traducción de memoria nueva (que no existe), debe solicitar una nueva página física y luego usarla en 
mmu_map_page(CR3, VADDR, PADDR, permisos). Este principio se aplica tanto en la creación inicial de tareas como en la asignación perezosa (Page Fault Handler).

Si no te dan una PADDR definida para el mapeo, casi siempre debes pedir una nueva página con la función mmu_next_free_page adecuada para el contexto (kernel o usuario).*/


"Random para esto de cuando usar cosas"
/*
Necesitas una función que te dé la TSS (Task State Segment) a partir de un selector, que traduzca una dirección virtual a física, o que consiga el CR3 de una tarea, 
en cualquier situación donde el kernel deba interactuar con el contexto o el mapa de memoria de una tarea inactiva (no la que está actualmente corriendo).

1. Obtener la TSS a partir de un Selector
La función que traduce un Selector de Tarea a su estructura TSS es el paso inicial para la inspección y manipulación del estado de cualquier tarea. La necesitas para:

Inspección del Contexto: Obtener los valores de registros específicos (como EAX, EDX, EIP) de una tarea que fue desalojada. Por ejemplo, en el scheduler con prioridades, 
se utiliza para acceder al registro EDX guardado en la pila de la tarea (cuya ubicación se calcula a partir de TSS.esp) para verificar si contiene el valor de prioridad (0x00FAFAFA).

Obtención de Datos Privilegiados: Es el único camino para obtener el CR3 de una tarea que no es la actual, ya que el valor de CR3 se almacena dentro de la estructura TSS.

2. Traducir Dirección Virtual a Física (virt_to_phy)
Esta función es vital cuando el kernel tiene una dirección virtual que pertenece al espacio de direcciones de una tarea de usuario y necesita conocer su ubicación física 
(PADDR) para acceder a ella o manipular su mapeo. La necesitas para:

Monitoreo/Inspección: Se usa para funciones como uso_de_memoria_de_las_parejas o el garbage collector si se requiriera buscar la PTE por la PADDR.

Syscalls de Servicio Cruzado: Es necesaria en syscalls como espiar, donde la tarea A da una dirección virtual de la Tarea B. El kernel debe traducir la VADDR de B a una PADDR, 
y luego mapear esa PADDR a una VADDR temporal de A para poder realizar la copia.

3. Conseguir el CR3 de una Tarea (task_selector_to_CR3)
Necesitas el valor del CR3 (la PADDR del Page Directory) siempre que vayas a realizar una operación de gestión de memoria sobre el mapa de otra tarea o para cambiar el mapa de la CPU.

Gestión del Kernel sobre Otras Tareas: Se necesita en el garbage collector o el Task Killer para modificar o desmapear páginas de la tarea que está siendo limpiada o terminada. 
El Page Fault Handler también utiliza el CR3 para saber dónde realizar el mapeo.

Task Switch (Cambio de Contexto): La rutina de C (sched_next_task) debe obtener el CR3 de la próxima tarea (a través de la TSS) para que el código en ensamblador 
lo pueda cargar en el registro CR3 de la CPU.

Asignación de Memoria Compartida: En el caso de las parejas, el CR3 es necesario para llamar a mmu_map_page(CR3_tarea, VADDR, PADDR, permisos) para crear las entradas de paginación 
(R/W o R/O) en el mapa de memoria de la tarea, asegurando el aislamiento y la protección.
*/
vir_to_phy
/*
El significado de Traducir la Dirección es que el kernel obtiene la Dirección Física (PADDR) de un dato que una tarea de usuario conoce solo por su Dirección Virtual (VADDR). 
Esto se logra usando la función virt_to_phy, la cual navega las tablas de paginación de la tarea. El propósito de obtener la PADDR no es acceder al dato inmediatamente, 
sino controlar o reubicar esa memoria.

El kernel no está interesado en acceder al dato de la tarea directamente, sino en manipular las estructuras que definen su mapa de memoria o acceder al contenido de forma segura:

1. Manipulación del Mapeo (Page Tables)
El kernel utiliza la traducción (VADDR → PADDR) y el registro CR3 para manipular las PTEs (Page Table Entries) de la tarea. Esto permite:

Desmapear Memoria: En el garbage_collector, el kernel obtiene la PADDR de la página para saber si debe liberarla. Luego llama a mmu_unmap_page para borrar la entrada 
de la PTE (poniendo el Bit Presente P=0) y desvincular la página virtual.

Inspección: El kernel puede leer los bits de la PTE (como el Bit Dirty o el Bit Accessed) para tomar decisiones de gestión, como determinar si debe escribir la página a disco.

2. Acceso al Contenido (Mapeo Temporal)
Si el kernel necesita leer o escribir el contenido del dato de la tarea (ej., en syscalls como espiar o copy_page), no puede usar la PADDR directamente. En su lugar, 
debe mapear la PADDR de nuevo a su propio espacio virtual. Para esto, el kernel usa una VADDR temporal (ej., SRC_VIRT_PAGE) y llama a mmu_map_page(rcr3(), SRC_VIRT_PAGE, 
PADDR, permisos). Solo después de este mapeo, el kernel puede acceder al dato usando el puntero de la VADDR temporal.

🚫 Restricción: Acceso a Memoria Física
La restricción fundamental es que el código que se ejecuta en la CPU solo puede acceder a Direcciones Virtuales (VADDRs) una vez que la Paginación está habilitada. 
Si el kernel intentara usar una PADDR (como 0x800000) como un puntero, el subsistema de paginación intentaría traducirla usando el CR3 activo. Si esa PADDR no está 
mapeada en el Page Directory, causaría un Page Fault. En resumen, el kernel no puede acceder a la memoria física directamente, pero puede manipular la información 
de mapeo y acceder al contenido si primero lo mapea a su propio espacio virtual.



La razón por la que el kernel necesita la Dirección Física (PADDR), a pesar de poder "leer" la Dirección Virtual (VADDR), es que la VADDR solo es útil dentro del mapa activo (el CR3 cargado), 
mientras que la PADDR es el recurso que el kernel realmente gestiona y comparte.

1. La VADDR es Insuficiente para la Gestión de Recursos
El kernel gestiona los recursos de hardware, no las abstracciones de software. Rastrear la RAM: Tu kernel gestiona el pool de memoria libre (mmu_next_free_user_page) utilizando direcciones físicas 
(PADDR). Si el kernel quiere devolver una página a ese pool, debe saber su PADDR. Identidad Global: La PADDR es la identidad única y global del bloque de 4 KiB de RAM. La VADDR, en cambio, 
es una identidad local y privada a cada tarea (ej., 0xC0C00000 significa algo distinto para la Tarea A y la Tarea C, si no están compartiendo).

2. PADDR es Necesaria para Romper el Aislamiento
La función virt_to_phy es crucial cuando el kernel necesita realizar una acción que rompe el aislamiento del contexto:

A. Mapeo Cruzado (Syscalls de Servicio)

Cuando la Tarea A quiere que el kernel acceda a la memoria de la Tarea B (ej., la syscall espiar), la Tarea A da el VADDR de B. El kernel usa virt_to_phy para encontrar la PADDR de esa 
memoria en el mapa de B. Luego, el kernel mapea esa PADDR a una VADDR temporal en el mapa de A. No es posible simplemente mapear la VADDR de B a la VADDR de A, ya que la VADDR de B 
es solo una abstracción, no el recurso real. La PADDR es el puente de datos que necesitas mover.

B. Gestión de Recursos Compartidos (Parejas)

Tu solución de Parejas depende de la PADDR para la coherencia. El array shared_page_frames[1024] no almacena VADDRs, sino PADDRs. Cuando una tarea accede, el kernel traduce la VADDR 
para obtener el índice, lee el PADDR del array y usa ese PADDR para mapear a la tarea. Si el kernel no supiera la PADDR, no podría gestionar el pool de memoria física ni garantizar 
que la Tarea B y la Tarea A estén mapeadas a la misma RAM.
*/

"PIC, interrupciones y memoria fisica"

/*
PIC Finish (Hardware vs. Software Interrupts)
La función pic_finish1 (o pic_finish2) se utiliza para comunicarse con el Controlador Programable de Interrupciones (PIC), un componente de hardware que gestiona las interrupciones 
externas (IRQs) de dispositivos como el reloj y el teclado.

¿Por qué es Necesario para las IRQs?
IRQs (Interrupciones de Hardware): Cuando un dispositivo genera una interrupción (ej., el reloj, que activa INT 32), el PIC bloquea esa línea IRQ a la espera de confirmación.
EOI (Fin de Interrupción): pic_finish1 envía un comando End Of Interrupt (EOI) (valor 0x20) al PIC.
Efecto: Este comando EOI notifica al PIC que el kernel ha terminado de atender la interrupción, lo que permite al PIC desbloquear la línea IRQ y aceptar nuevas interrupciones. 
Sin esta llamada, el sistema dejaría de recibir interrupciones de hardware.

¿Por qué No para las Syscalls?
Syscals (Interrupciones de Software): Las syscalls (ej., INT 88) se generan intencionalmente por software usando la instrucción INT.
No Involucra Hardware: Dado que no están mediadas por el PIC, no hay una línea de hardware que reconocer o resetear, haciendo que el comando EOI sea innecesario.

2. 🗺️ Cuándo y Cómo Consultar un Mapeo Virtual
Se necesita verificar si una Dirección Virtual (VADDR) está mapeada a una Dirección Física (PADDR) siempre que el kernel deba manipular la memoria física o garantizar un acceso seguro.

A. Cuándo Consultar (Situaciones)
Liberación de Memoria: Antes de que el kernel llame a mmu_unmap_page sobre el VADDR de una tarea (ej., en el garbage_collector), se verifica el mapeo para saber si debe devolver una PADDR al pool libre.
Inspección/Copia: En syscalls como espiar, el kernel debe verificar que el VADDR solicitado sea válido y esté actualmente mapeado en el espacio de la tarea objetivo antes de intentar leer el dato.
Manejo de Page Faults: Aunque el Page Fault ocurre porque la página no está presente, el handler debe verificar el estado de la Page Directory Entry (PDE) para confirmar que la Page Table (PT) 
existe antes de buscar la PTE.

B. Cómo Consultar (El Bit Presente)
El estado del mapeo se consulta revisando el Bit Presente (P), que es el Bit 0 de la Page Directory Entry (PDE) y la Page Table Entry (PTE).

Estado del Bit	Ubicación	Significado
P=1	PDE o PTE	
La entrada es válida, y el siguiente nivel (o la página física) está presente en RAM.

P=0	PDE o PTE	
La entrada es inválida o la memoria está swappeada (no presente en RAM). Esto dispara un Page Fault.
Exportar a Hojas de cálculo
Para consultar el bit, el kernel navega las tablas comenzando desde el CR3 de la tarea objetivo:
Check PDE: Revisa el Bit P de la PDE correspondiente al VADDR. Si P=0, la PT está ausente.
Check PTE: Si la PDE está presente, revisa el Bit P de la PTE.

3. 🛡️ ¿Cuándo se Consulta el Bit Presente?
El Bit Presente es consultado por dos entidades:
La CPU (Hardware): El procesador revisa el Bit P durante cada acceso a memoria. Si es P=0, genera inmediatamente la excepción de Page Fault (INT 14).
El Kernel (Software): El kernel lo revisa cuando necesita saber el estado del mapa sin provocar un Page Fault (ej., en mmu_unmap_page para ver si se debe devolver la PADDR al pool libre).



🗺️ Page Directory Entry (PDE) y Page Table Entry (PTE)
Una PDE (Page Directory Entry) y una PTE (Page Table Entry) son las dos entradas fundamentales de 32 bits utilizadas en el esquema de Paginación de x86. Ambas son esencialmente descriptores 
que le dicen al procesador cómo traducir una Dirección Virtual (VADDR) a una Dirección Física (PADDR).

1. Page Directory Entry (PDE)
La PDE es una de las 1024 entradas contenidas en el Page Directory (PD), que está apuntado por el registro CR3.

¿Qué es? Es una entrada que gestiona un bloque de 4 MiB del espacio virtual.

Contenido Principal:

Bit P (Presente): Si es 1, la PDE es válida. Si es 0, toda la región de 4 MiB está inaccesible.
Dirección de Page Table: Los 20 bits superiores contienen la dirección física de una Page Table (PT) completa.
Relación con las Tareas: Las PDEs son el primer nivel de traducción. Una tarea necesita PDEs válidas en su PD para que su memoria (código, datos, pila) sea accesible.

2. Page Table Entry (PTE)
La PTE es una de las 1024 entradas contenidas en una Page Table (PT), y es el nivel final de la traducción.

¿Qué es? Es una entrada que gestiona un bloque de 4 KiB (una página) del espacio virtual.

Contenido Principal:
Bit P (Presente): Si es 1, la página está en RAM. Si es 0, dispara un Page Fault (INT 14).
Dirección de Page Frame: Los 20 bits superiores contienen la dirección física de la página de 4 KiB de datos.
Bits de Atributo: Contiene banderas cruciales como Dirty (D), Accessed (A), Read/Write (W) y User/Supervisor (U), que son esenciales para la protección y el monitoreo del kernel.
Relación con las Tareas: Las PTEs definen la ubicación exacta y los permisos de acceso de cada página de la tarea. En el caso de memoria compartida, las PTEs de dos tareas diferentes apuntan al mismo Page Frame.

🤝 Relación con las Tareas en el Kernel
Las PDEs y PTEs se relacionan con las tareas porque el CR3 (apuntado por la TSS de la tarea) indica la ubicación de la tabla que contiene todas sus PDEs.

Aislamiento: Cada tarea tiene su propio conjunto de PDEs y PTEs (su propio mapa virtual), lo que garantiza que la Tarea A no pueda acceder por error o malicia a la memoria de la Tarea B.
Lazy Allocation: El kernel manipula el Bit P de la PTE en la syscall para retrasar la asignación física de la memoria hasta que un Page Fault (P=0) obliga al kernel a asignar una página.
Protección: El kernel usa el Bit U (User/Supervisor) para proteger su propio código (Nivel 0) del acceso por las tareas de usuario (Nivel 3).


Necesitarías obtener la Page Directory Entry (PDE) de una tarea siempre que el kernel deba inspeccionar o manipular las estructuras de paginación de esa tarea sin pasar por el proceso 
completo de mapeo. La PDE es el primer nivel de la jerarquía de paginación que te permite determinar si una región entera de 4 MiB está disponible.

1. 🔍 Situaciones para Obtener la PDE
Las situaciones más comunes donde el kernel (Nivel 0) necesita acceder directamente a una PDE son:

Verificación de Existencia de Page Table: Antes de que el kernel intente mapear una página (ej., en mmu_map_page), debe consultar la PDE para ver si la Page Table (PT) correspondiente ya existe. 
Si la PDE tiene el Bit Presente (P=0), el kernel debe asignar una nueva página para crear la PT, y luego registrar su dirección física en la PDE.
Gestión de Memoria a Granel (4 MiB): Si el kernel necesita invalidar o cambiar los permisos para una región completa de 4 MiB de la tarea, solo necesita manipular la PDE,
 no las 1024 PTEs individuales que contiene.
Diagnóstico de Page Fault: Aunque el Page Fault Handler utiliza el valor de CR2, necesita consultar la PDE para entender por qué falló el acceso (ej., falló en el primer nivel de traducción).

2. 🛠️ Cómo Obtener la PDE (Navegación del Mapeo)
Obtener la PDE de una tarea es un proceso de tres pasos que requiere el CR3 de la tarea objetivo y la Dirección Virtual (VADDR) que se quiere consultar:
Obtener el CR3 de la Tarea:
Primero, debes tener el valor del CR3 de la tarea cuyo mapa quieres inspeccionar. (Si es la tarea actual, usas rcr3(); si es otra tarea, usas task_selector_to_CR3).
Calcular el Índice de la PDE:
Se utiliza la macro VRT_PAGE_DIR(VADDR) para extraer los 10 bits más significativos (bits 31-22) de la Dirección Virtual (VADDR). Este valor es el índice (pd_index) quete indica cuál de las 1024 entradas del Page Directory debes buscar.

Acceder a la PDE:
Se utiliza el valor del CR3 como la dirección base física del Page Directory.
Se accede al array de PDEs en esa dirección utilizando el índice calculado.
En C, el códgo se ve así:

C

// Asumimos que pd_entry_t* es un puntero al Page Directory de la tarea
// y que task_vaddr es la dirección virtual a consultar.

void get_pde_from_vaddr(uint32_t cr3_value, vaddr_t task_vaddr) {
    
    // 1. Obtener la dirección base del Page Directory
    pd_entry_t* pd = (pd_entry_t*)CR3_TO_PAGE_DIR(cr3_value);

    // 2. Calcular el índice de la PDE
    uint32_t pd_index = VIRT_PAGE_DIR(task_vaddr);

    // 3. Acceder y devolver la PDE
    pd_entry_t pde = pd[pd_index];
    
    // Aquí puedes inspeccionar pde.attrs (e.g., verificar si pde.attrs & MMU_P)
    // o usar pde.pt para obtener la PADDR de la Page Table.
}


Ambas operaciones se realizan accediendo y modificando la Page Directory Entry (PDE) correspondiente a la región de 4 MiB dentro del Page Directory (PD) de la tarea.

1. 🔍 Verificación y Creación de la Page Table (PT)
Este proceso es fundamental en tu función mmu_map_page (o similar) y garantiza que la jerarquía de paginación exista antes de intentar crear la entrada final.

Pasos
Obtener la PDE: Se utiliza el CR3 de la tarea y la Dirección Virtual (VADDR) para localizar la PDE que corresponde a esa VADDR.
Verificar Existencia (Bit P): Se consulta el Bit Presente (P) de esa PDE.
Si P=1: La Page Table (PT) ya existe y su dirección física está en los 20 bits superiores de la PDE. El kernel procede a usar esa PT para buscar o crear la PTE.
Si P=0: La PT no existe. El kernel procede a crearla:
Asignación de PT: Llama a mmu_next_free_kernel_page() para obtener una nueva página física (4 KiB) que será la PT.
Inicialización: Limpia esa nueva página de la PT a cero (con zero_page) para que todas sus PTEs iniciales estén en P=0.
Actualización de PDE: La PDE original se sobrescribe con la dirección física de la nueva PT (shifteada 12 bits) y se activa el Bit Presente (P=1) y los permisos necesarios (ej., R/W y User/Supervisor).
Este proceso garantiza que siempre haya una PT válida antes de intentar crear la traducción final a la PADDR.

2. 🛡️ Gestión de Memoria a Granel (4 MiB)
Si el kernel necesita manipular una región completa de 4 MiB (que es el área cubierta por una sola PDE), simplemente manipula la PDE en lugar de iterar por las 1024 PTEs.
Acciones
Obtener la PDE: Localiza la PDE correspondiente a la región de 4 MiB (como en el paso anterior).
Cambio de Permisos (Ej. R/O a R/W):
El kernel solo necesita modificar el Bit R/W (W) y/o el Bit U/S (U) directamente en el campo de atributos de la PDE.
Atención: Los permisos en la PDE actúan como un filtro. Si la PDE se establece como Solo Lectura, ninguna de las 1024 PTEs que contenga podrá tener permisos de escritura, incluso si la PTE individual lo indica.
Invaliación de Región (TLB Flush):
Si el kernel quiere invalidar instantáneamente una región de 4 MiB completa de la tarea, simplemente pone el Bit Presente (P=0) de la PDE.
Esto hace que el próximo acceso a cualquier dirección virtual dentro de esos 4 MiB cause un Page Fault.
Flushing: Después de cualquier manipulación de la PDE, el kernel debe llamar a tlbflush() para asegurarse de que la CPU descarte cualquier entrada antigua en su caché de traducción 
(TLB) y utilice la PDE modificada.
*/

"ranfom"

/*
a estrategia para saber si la página está mapeada requiere que la Tarea A inspeccione directamente el mapa de memoria de la Tarea B.
1. Acceder al Mapa de la Pareja (Tarea B)
Obtener el CR3 de B: La Tarea A debe identificar a su pareja (B) y usar el selector de la Tarea B para obtener su $\mathbf{CR3}$ (la PADDR del Page Directory de B).
Localizar la PTE de B: Usando el $\text{CR3}$ de B y la Dirección Virtual que se está consultando ($\text{0xC0C00000}$), el kernel debe navegar las tablas de 
paginación de la Tarea B para encontrar la $\mathbf{PTE}$ (Page Table Entry) correspondiente.
Consultar el Bit Presente (P): Una vez que el kernel tiene el puntero a la $\text{PTE}$ de la Tarea B, consulta el Bit Presente (P).
Si $\mathbf{P=1}$: ¡La Tarea B ya mapeó la página! 
Esto significa que la $\text{PADDR}$ ya existe y está registrada en la $\text{PTE}$ de B.
Si $\mathbf{P=0}$: La Tarea B aún no ha accedido a esa página (o no se ha mapeado).
*/

"segmen selector"
/*
Usas el Selector de Segmento (Selector) siempre que la CPU necesita acceder a una estructura de control o a un segmento de memoria definido en las tablas de descriptores (GDT o LDT). 
El Selector no es una dirección de memoria; es un valor de 16 bits que actúa como un índice para estas tablas.

1. Cambio de Contexto de Tarea (Task Switch)
El uso más crítico del Selector es para iniciar un Task Switch, que es la base de la concurrencia en tu kernel: la Instrucción LTR (Load Task Register) usa el Selector para decirle al 
registro TR cuál es la TSS (Task State Segment) de la tarea que debe cargar el procesador para que la tarea se convierta en la "Tarea Actual". Esto se hace durante la inicialización del sistema. 
La Instrucción JMP FAR también usa el Selector para forzar un salto a una nueva tarea (Task Switch por hardware), lo cual es el mecanismo principal utilizado en el scheduler (_isr32) o en la 
syscall para cambiar a la próxima tarea elegible. El selector que se usa aquí es el Selector de la TSS de la nueva tarea.
2. Acceso a Segmentos de Código y Datos
Usas los Selectores para configurar el ambiente de ejecución y la pila: en la Carga Inicial, al entrar en modo protegido, la CPU usa selectores para inicializar los registros de segmento 
(CS, DS, ES, SS, FS, GS), apuntándolos a los descriptores de Nivel 0 o Nivel 3 definidos en tu GDT. En la Configuración de Tareas, al crear una nueva tarea (tss_create_user_task), debes usar 
Selectores de Segmento de Nivel 3 para los registros de código y datos de la tarea y Selectores de Nivel 0 para los registros de pila del kernel (SS0, CS0).
3. Syscalls y Rutinas de Interrupción (IDT)
La IDT también usa Selectores: cada entrada de la IDT (Descriptor de Puerta) almacena un Selector de Segmento (segsel). Este selector no apunta a una TSS, sino que apunta al Descriptor 
de Segmento de Código de Nivel 0 (GDT_CODE_0_SEL) en la GDT. Esto asegura que el handler de la interrupción se ejecute con los privilegios de Nivel 0 (kernel).
4. Estructura del Selector
El Selector de Segmento de 16 bits está compuesto de tres partes que el kernel manipula: el Índice (Bits 3-15), que indica qué entrada de la GDT o LDT se debe usar, el TI (Table Indicator - Bit 2), 
que indica si se apunta a la GDT (TI=0) o a la LDT (TI=1), y el RPL (Requested Privilege Level - Bits 0-1), que indica el nivel de privilegio que solicita el código que está usando este Selector 
(0b00 para Nivel 0, 0b11 para Nivel 3
*/

cuando una tarea de usuario ejecta una instruccion de interrupcion (syscall) se origina en el nivel 3 y salta a nivel 0 del kernel.
El codigo de ususuario no tiene los privilegios necesarios para acceder a recursos de hardware o estructuras criticas del sistema operativo.
La IDT para la syscall debe tener DPL 3, qe le permite ser invocada desde el nivel 3.
Cuando la CPU maneja la interrupcion, cambia automaticamente al nivel 0 y usa el selector de segmento definido en la IDT para cargar el segmento de codigo del kernel (GDT_CODE_0_SEL).
Esto asegura que el handler de la syscall se ejecute con los privilegios adecuados del kernel.
Al recibir la interrupción, la CPU:Verifica que la syscall sea válida.Realiza un cambio de pila y transiciona el modo de operación del procesador a Nivel 0.
El descriptor de puerta de la $\text{IDT}$ (Descriptor de Puerta de Interrupción) para la syscall apunta a un segmento de código de $\text{Nivel 0}$ 
(usando el selector $\text{GDT\_CODE\_0\_SEL}$).
Esto garantiza que el handler del kernel tenga los permisos más altos para manipular la $\text{MMU}$, el scheduler y el hardware.

Correcion del ejercicio parejas

Habia un problema en usar shared_page_frames como registro global del kernel.
"El array era de 1024 elementos donde cada uno era una Dir Fisica. Los 1024 slots corresponden a las 1024 paginas de 4KB que componen la pagina compartida de 4MB."
"el array se inicializa en 0, y luego al hacer los mapeos se consulta y si esta el slot en 0 significa q la pagina fisica correspondiente a ese slot todabia no se asigno"
"si no es cero, la pagina fisica ya esta asignada y debe usarse esa misma pagina fisica para mapear en la nueva tarea pareja de la q lo seteo primero"
"Cuando una pareja accede a una dire, se calcula el indice, haciendo (virt - TASK_SHARED_PAGE) / PAGE_SIZE, y se consulta shared_page_frames[indice]"
Pero al ser una variable global, terminaba pasando q cualquier tarea podia modificarla y corromper el estado. Cualquier mapeo podia terminar en la misma
direccion fisica, causando errores de memoria compartida entre parejas que no deberian compartir memoria.
La solucion fue hacer que shared_page_frames sea un array dentro de la estructura de sched_entry_t, asi cada tarea tiene su propio registro de paginas compartidas asignadas.
Solo va a estar seteado en la entry de la tarea lider.

typedef struct {
  // ...
  // Puntero al array de 1024 PADDRs. SOLO se asigna si la tarea es Líder.
  paddr_t* shared_pages_tracker; //lo inicio en crear pareja
} sched_entry_t;

"y despues crear pareja queda asi"

extern paddr_t* kmalloc(size_t size); 

void crear_pareja(void) {
    int8_t id_actual = current_task;
    sched_entry_t* tarea = &sched_tasks[id_actual];
    
    // 1. Verificar si ya pertenece a una pareja
    // Si pareja_id es diferente de -1, ya está en una pareja.
    if (tarea->pareja_id != ID_SIN_PAREJA) {
        return; // Ignora la solicitud y retorna de inmediato.
    }

    // 2. Inicializar la tarea como LÍDER y habilitar el acceso
    tarea->pareja_id = ID_SIN_PAREJA; // Mantiene -1 temporalmente hasta que se una otro.
    tarea->es_lider = true;
    tarea->tiene_acceso_compartido = true; // Habilita el acceso para el Page Fault Handler

    // 3. Inicializar el Tracker de Páginas Físicas (Solución al aislamiento entre parejas)
    
    // Asignar memoria de kernel para el array de seguimiento (1024 paddr_t * 4 bytes/paddr)
    paddr_t* tracker = (paddr_t*)kmalloc(NUM_PAGES_SHARED * sizeof(paddr_t));
    
    // Inicializar el tracker a cero (0 significa "no asignado")
    kmemset(tracker, 0x00, NUM_PAGES_SHARED * sizeof(paddr_t)); 
    
    // Guardar el puntero al array de seguimiento en la estructura del líder
    tarea->shared_pages_tracker = tracker; 

    // 4. Bloquear la tarea hasta que otra se una
    tarea->creando_pareja = true; // Flag para aceptando_pareja()
    tarea->state = TASK_BLOCKED_ESPERANDO_PAREJA; 
    
    // El Task Switch ocurre al salir del handler, que saltará a la siguiente tarea RUNNABLE.
}

kmalloc

Una implementación básica de kmalloc en un kernel que solo maneja asignaciones a nivel de página (o múltiplos de página) 
sería simplemente un wrapper de mmu_next_free_kernel_page.
Sin embargo, dado que kmalloc debe soportar cualquier tamaño (size_t), la implementación más realista sería buscar un bloque libre en el heap de kernel.
Para este contexto, la implementación más simple es reservar la cantidad de páginas necesarias.

1. Implementación de kmalloc (Asignación por Página)
Asumiremos que si se pide un bloque mayor a PAGE_SIZE, devolvemos NULL. Si no, simplemente asignamos la página.
// Definición de kmalloc (Asignación por página)
void* kmalloc(size_t size) {
    // Verificar si el tamaño solicitado cabe en una sola página.
    // Si necesitas asignar bloques grandes, la lógica debe iterar y mapear múltiples páginas.
    if (size > PAGE_SIZE) {
        // En un kernel real, se redondearía 'size' hacia arriba
        // y se asignarían varias páginas contiguas o no contiguas.
        return NULL; 
    }
    
    // 1. Obtener la dirección física de una página de kernel libre
    paddr_t phy_addr = mmu_next_free_kernel_page();
    
    // 2. Mapear la página física a una Dirección Virtual (VADDR) para usarla en el kernel
    // Esto es crucial: el kernel necesita acceder a la memoria recién asignada a través de VADDR.
    // Asumimos un mapeo identidad simple o un mapeo alto ya establecido para el kernel.
    
    // Si el kernel opera con Identity Mapping en su pool, la PADDR es la VADDR
    return (void*)phy_addr; 
}

// Asumimos que esta función es el equivalente de kmalloc que asigna páginas de kernel
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    // 1. Calcular el número de páginas necesarias (Redondeando hacia arriba)
    // El NUM_PAGES_SHARED * sizeof(paddr_t) es 4 KiB (un Page Size completo).
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    // 2. Asignar la primera página física del pool de kernel (que será la VADDR inicial)
    paddr_t start_paddr = mmu_next_free_kernel_page(); 
    
    // 3. Iterar y mapear las páginas
    uint32_t cr3 = rcr3(); // Usar el CR3 del kernel actual

    for (size_t i = 0; i < num_pages; i++) {
        paddr_t current_paddr = start_paddr + (i * PAGE_SIZE);

        if (i > 0) {
            // Asignar el PADDR para las páginas restantes y actualizar current_paddr
            current_paddr = mmu_next_free_kernel_page(); 
            // Si el kmalloc debe devolver un bloque contiguo VIRTUAL, se usa current_paddr como PADDR
            // y se mapea a VADDR contiguas (ej. start_paddr + (i * PAGE_SIZE)). 
            // Para simplificar, asumimos que el pool de kernel da contigüidad PADDR/VADDR.
        }

        // 4. Mapear: PADDR -> VADDR (Identity Mapping en el pool libre de kernel)
        // Permisos: Presente (P), Lectura/Escritura (W)
        mmu_map_page(cr3, (vaddr_t)current_paddr, current_paddr, MMU_P | MMU_W);
    }
    
    // 5. Devolver la VADDR inicial (que es la PADDR inicial en el Identity Mapping)
    return (void*)start_paddr;
}

Razón del Mapeo IdentidadEl kmalloc que implementamos asume Mapeo Identidad (VADDR = PADDR) en el área del pool libre del kernel. 
Esto es necesario porque el código del kernel (kmemset y las funciones auxiliares) deben poder acceder al tracker a través de la VADDR 
que se devuelve. Al mapear $0\text{x100000}$ (PADDR) a $0\text{x100000}$ (VADDR), la dirección devuelta es inmediatamente funcional 
para el código del kernel.2. Por Qué es Válido Llamar en una SyscallEs perfectamente válido llamar a kmalloc (o cualquier otra función del kernel) 
dentro de la syscall crear_pareja por la siguiente razón:Cambio de Nivel de Privilegio: La función crear_pareja es el cuerpo del handler de la syscall. 
Aunque la llamada se origina en Nivel 3 (Modo Usuario), la ejecución del handler ocurre en Nivel 0 (Modo Kernel).
kmalloc es Nivel 0: kmalloc es una función de Nivel 0 que manipula estructuras de kernel (mmu_next_free_kernel_page) y modifica el mapa 
de memoria (mmu_map_page).Acceso Privilegiado: Una vez que el procesador salta al handler de la syscall, el código tiene privilegios de Nivel 0 
y puede llamar a cualquier función del kernel que requiera alto privilegio para realizar tareas como asignar memoria para el tracker de la pareja.

Necesidad de un Puntero (VADDR): La función kmemset en C opera sobre punteros (void*). En Modo Protegido con Paginación, un puntero debe ser una Dirección Virtual (VADDR) válida.

El Mapeo Identidad: El pool de memoria libre del kernel está diseñado para que su Dirección Física y su Dirección Virtual sean el mismo valor (Mapeo Identidad).

kmalloc pide PADDR=0x100000 a mmu_next_free_kernel_page().

kmalloc establece la traducción: VADDR=0x100000→PADDR=0x100000.

kmalloc devuelve la VADDR (0x100000).

Ejecución de kmemset:

Cuando crear_pareja llama a kmemset(tracker, ...) (donde tracker es 0x100000), el kernel ejecuta el código para escribir 0s en esa VADDR.

La CPU utiliza el Page Directory de la Tarea (que rcr3() cargó) para traducir 0x100000 (VADDR) a 0x100000 (PADDR) y escribe directamente en la memoria física recién asignada.

En resumen: kmalloc no solo asigna la memoria física, sino que también realiza el mapeo temporal de Nivel 0 necesario para que el código del kernel pueda acceder y manipular el tracker 
(con kmemset y las funciones auxiliares) mientras se ejecuta en el contexto de la tarea que llamó a la syscall.


int juntarse_con(int id_tarea_lider) {
    int8_t id_tarea_actual = current_task;
    sched_entry_t* tarea_actual = &sched_tasks[id_tarea_actual];
    sched_entry_t* tarea_lider = &sched_tasks[id_tarea_lider];
    
    // 1. Verificación: ¿Ya pertenece a una pareja?
    if (tarea_actual->pareja_id != ID_SIN_PAREJA) { 
        return ID_ERROR; 
    }

    // 2. Verificación: ¿El ID es una tarea que está CREANDO una pareja?
    // Usamos el flag 'creando_pareja' que se setea en crear_pareja().
    if (!tarea_lider->creando_pareja) { 
        return ID_ERROR; 
    }
    
    // --- CONFORMAR PAREJA ---
    
    // 3. Vincular y configurar el LÍDER
    tarea_lider->pareja_id = id_tarea_actual; // El seguidor es la pareja del líder
    tarea_lider->creando_pareja = false;      // Ya no está esperando
    // El líder ya tiene acceso compartido seteado desde crear_pareja()

    // 4. Vincular y configurar el SEGUIDOR (tarea_actual)
    tarea_actual->pareja_id = id_tarea_lider; // El líder es la pareja del seguidor
    tarea_actual->es_lider = false;
    tarea_actual->tiene_acceso_compartido = true; // Habilitar acceso compartido
    
    // 5. Desbloquear al LÍDER
    // El líder estaba en estado TASK_BLOCKED_ESPERANDO_PAREJA
    tarea_lider->state = TASK_RUNNABLE; 
    
    return ID_EXITO; // Retorna 0.
}


void abandonar_pareja(void) {
    int8_t id_actual = current_task;
    sched_entry_t* tarea_actual = &sched_tasks[id_actual];
    int8_t id_pareja = tarea_actual->pareja_id;
    
    // 1. Caso: No pertenece a ninguna pareja
    if (id_pareja == ID_SIN_PAREJA) {
        return; 
    }
    
    // --- Tarea pertenece a una pareja ---
    
    if (tarea_actual->es_lider) {
        // 2. Caso: Es su líder
        // Queda bloqueada hasta que la otra parte abandone.

        // Bloquear al LÍDER. Asumimos que esta es la única acción del líder aquí.
        // El desbloqueo será responsabilidad de la tarea pareja al salir.
        tarea_actual->state = TASK_BLOCKED_LIDER_WAIT; 
        
        // El Task Switch ocurrirá al salir del handler.
        return; 
        
    } else {
        // 3. Caso: No es su líder (Es el seguidor)
        // Romper el vínclo
        romper_pareja(); 
        return; 
    }
}

bool page_fault_handler(vaddr_t virt) {
    
    // 1. Verificar si el fault está en el rango de memoria compartida
    if (virt >= SHARED_MEM_START_VADDR && virt < SHARED_MEM_END_VADDR) {
        
        int8_t task_id = current_task;
        
        // 2. Verificar el permiso LÓGICO de la tarea
        if (!tiene_acceso_compartido(task_id)) {
            // Acceso ilegal: La tarea no es parte de una pareja activa.
            // Esto debería ser un fault fatal que se maneja de forma estándar.
            return false; 
        }
        
        // 3. Determinar Permisos y Roles
        bool esLider = es_lider(task_id);
        uint32_t permisos = MMU_P | MMU_U; // Por defecto: Presente, Usuario, R/O

        if (esLider) {
            permisos |= MMU_W; // Añadir R/W (MMU_W) solo si es líder
        }

        // 4. Obtener la PADDR (Lazy Allocation y Coherencia Compartida)
        // La función get_shared_page_phy asigna la página física si es el primer acceso,
        // o reutiliza la página ya asignada por el compañero.
        paddr_t pagina_fisica = get_shared_page_phy(virt, task_id, esLider);

        // 5. Mapear la Página y Aplicar Permisos de Hardware
        vaddr_t page_base = virt & 0xFFFFF000;
        uint32_t cr3_actual = rcr3(); // CR3 de la tarea que falló
        
        mmu_map_page(cr3_actual, page_base, pagina_fisica, permisos);
        
        // 6. Éxito: Page Fault atendido.
        return true; 
    }
    
    // 7. Manejo de otros Page Faults (e.g., Stack, código, o el Lazy Allocation de malloco)
    // ... Lógica del handler base (que retorna false si es fatal) ...
    return false;
}

// Array global de punteros a páginas físicas compartidas (1024 páginas = 4 MB)
#define NUM_PAGES_SHARED 1024
#define SHARED_MEM_START_VADDR 0xC0C00000

// Prototipos asumidos:
extern paddr_t mmu_next_free_user_page(void);
extern void zero_page(paddr_t addr);
extern int8_t get_leader_id_for_task(int8_t task_id); // Función auxiliar para encontrar el líder

/**
 * Obtiene la dirección física para una dirección virtual dada en el área compartida.
 * Utiliza el tracker privado del líder para la asignación bajo demanda.
 */
paddr_t get_shared_page_phy(vaddr_t vaddr, int task_id, bool is_lider) {
    
    // 1. Obtener el ID del Líder (la clave para el tracker)
    int8_t leader_id = is_lider ? task_id : get_leader_id_for_task(task_id);
    
    // Si la tarea no tiene líder o el líder no es válido, esto es un error de lógica de syscall
    if (leader_id == ID_SIN_PAREJA) {
        return 0; 
    }

    // 2. Acceder al Tracker Privado del Líder
    // El puntero al array de 1024 PADDRs está guardado en el sched_entry_t del líder.
    sched_entry_t* leader_entry = &sched_tasks[leader_id];
    paddr_t* tracker = leader_entry->shared_pages_tracker;
    
    // Si por alguna razón el tracker no fue asignado (kmalloc falló), devolvemos error
    if (tracker == NULL) {
        return 0;
    }

    // 3. Calcular el índice de la página
    // (Aseguramos que la VADDR está alineada a página en el cálculo del índice)
    uint32_t page_index = (vaddr & 0xFFFFF000) - SHARED_MEM_START_VADDR;
    page_index /= PAGE_SIZE;

    // 4. Comprobar Lazy Allocation / Reutilización
    if (tracker[page_index] != 0) {
        // La página física ya existe (fue asignada por el líder o el seguidor).
        return tracker[page_index];
    }

    // 5. Asignación Bajo Demanda (Si la entrada es 0)
    paddr_t new_phy = mmu_next_free_user_page();
    
    // Inicializar la página a cero (requisito calloc)
    zero_page(new_phy); 
    
    // 6. Registrar la nueva página física en el tracker del Líder
    tracker[page_index] = new_phy;
    
    return new_phy;
}

Y por ultimo el de contar memoria deberia hacer lo mismo pero iterando por cada traquer de cada lider activo.

size_t contar_memoria_compartida_activa(void) {
    size_t total_memoria = 0;

    // Iterar sobre todas las tareas en el scheduler
    for (int8_t i = 0; i < MAX_TASKS; i++) {
        sched_entry_t* tarea = &sched_tasks[i];

        // Solo considerar tareas que son líderes y tienen un tracker válido
        if (tarea->es_lider && tarea->shared_pages_tracker != NULL) {
            paddr_t* tracker = tarea->shared_pages_tracker;

            // Contar las páginas asignadas en el tracker del líder
            for (uint32_t j = 0; j < NUM_PAGES_SHARED; j++) {
                if (tracker[j] != 0) {
                    total_memoria += PAGE_SIZE; // Sumar 4KB por cada página asignada
                }
            }
        }
    }

    return total_memoria; // Retorna el total de memoria compartida activa en bytes
}

