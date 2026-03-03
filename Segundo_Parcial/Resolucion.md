##1.
Primero, para que el kernel provea las syscalls, debo agregarlas en la idt (interrupt descriptor table), se pueden agregar al codigo de idt.c

```C
void idt_init() {
    //entradas anteriores usadas en el tp
    IDT_ENTRY3(90); //entrada de solicitar
    IDT_ENTRY3(91); //entrada de recurso listo
}
```
Se usa idt_entry3 porque una tarea de usuario no tiene los permisos para acceder a ciertos recursos o estructruas del sistema, entonces su entrada en la IDT debe tener privilegios nivel 3 para que la puedan invocar. Luego cuando se maneja la interrupcion, se realiza un cambio de pila y transiciona el modo de operacion a nivel 0.
Ademas uso las entradas 90 y 91 para que no clasheen con otras entradas que estan en uso.

En sched.c voy a agregar algunos campos a las estructuras que ya teniamos para manejar el ejercicio. Quiero agregar un estado que me diga si una tarea esta esperando un recurso, y quiero guardar en cada tarea que recurso produce y qué recurso esta solicitando. Tambien agrego un campo que me guarda un task_id, este va a tener el id de la tarea que le esté solicitando un recurso cuando esté produciendo

```C
typedef enum {
  TASK_SLOT_FREE,
  TASK_RUNNABLE,
  TASK_PAUSED,
  //nuevos estados
  TASK_WAITING_RESOURCE,
} task_state_t;

typedef struct {
    int16_t selector;
    task_state_t state;
    //campos agregados 
    recurso_t produce;      //guarda que recurso produce
    recurso_t solicita;     //guarda que recurso solicito
    task_id_t produce_para; //guarda para cual tarea está produciendo
} sched_entry_t;

```

Luego, hay que definir en isr.asm el handler de cada interrupción
La syscall solicitar debera recibir como parametro el recurso que necesita, en este caso los parametros se pasan por pila por usar convencion de 32 bits. Digo que se lo paso por el registro edi, entonces antes de llamar a la función lo pusheo para que lo tome como argumento, y despues le sumo 4 al puntero de la pila para alinearla nuevamente.
Además me dice que no se devuelve el control a la tarea solicitante hasta que el recurso esté listo, entonces una vez llamada la funcion de solicitar, la tarea cambia de estado a uno esperando recurso, y en el handler de la syscall llama sched_next_task que nos devuelve un selector de tarea distinto al que se estaba ejecutando (porq la tarea actual ya no va a estar runnable) y hace el salto a esa nueva tarea, pausando la ejecución de la tarea que llamó.
En el caso de recurso listo, no necesita pasar nada por parametro ya que la tarea que lo llama sólo lo usa para avisar que el recurso está listo, entonces en el handler de asm solo llamo a la función que va a tener todo lo que hace la tarea cuando termina un recurso.

```ASM
extern solicitar
extern recurso_listo

global _isr90
_isr90:
    pushad      ;preservo todos los registros
    push edi
    call solicitar
    add esp, 4
    call sched_next_task

    mov word [sched_task_selecto], ax
    jmp far [sched_task_offset]         ;aca hace el jump far a la siguiente tarea, el offset se ignora cuando el
                                        ;selector apunta a un descriptor de tareas
    popad      ;restauro todos los registros
    iret       ;termina interrupcion

global _isr91
_isr91:
    pushad
    call recurso_listo          ;pendiente lo de la tarea
    popad
    iret
```

Me falta definir en C lo que hace cada función.
La idea de solicitar, es que reciba un recurso_t, y al inicio ya setear la tarea llamadora como esperando recurso para asegurar que la tarea quede pausada hasta que se termine de preparar el recurso que pidio. Porque al salir de la función el scheduler encuentra la siguiente tarea activa infinitamente y no vuelve a ejectuar la que llamo solicitar hasta que se vuelva a setear como runnuble. Además hay setear su campo de recurso solicitado al pasado por parametro. Para esto accedo a los campos de la tarea actual usando la lista sched_tasks, que contiene todas las tareas guardadas en el scheduler, y la indexo con el task_id de la current task que es la que llamó a solicitar. Y por ultimo, quiero buscar en la lista de tareas del scheduler, una tarea que esté disponible para producir y que produzca el recurso que solicita, uso la funcion auxiliar dada por la cátedra que me da el id de una tarea disponible para producir. Entoncces, puedo setear en la tarea solicitada para quién produce (para current task) y se setea su estado a runnable para indicar que está produciendo un recurso.

```C
void solicitar(recurso_t recurso) {
    sched_tasks[current_task].state = TASK_WAITING_RESOURCE;
    sched_tasks[current_task].solicita = recurso;

    task_id_t solicitada_id = hay_tarea_disponible_para(recurso);

    sched_entry_t task_solicitada = sched_tasks[solicitada_id];

    task_solicitada.produce_para = current_task;
    task_solicitada.state = TASK_RUNNABLE;    
}
```

Ahora cuando una tarea llama a recurso listo, este tiene información en 4KB a partir de la dirección virtual 0x0AAAA000, esa informacion quiero copiarla a la tarea que pidio el recurso desde la direccion virtual 0x0BBBB000. 4KB es una página, asi que quiero copiar la información de una pagina de la tarea llamadora a una pagina de la tarea que solicito el recurso.

La tarea que llama a recurso_listo tiene guardado qué tarea le solicitó ese recurso, entonces puedo conseguir su sched_entry_t con el id que obtengo del campo donde tenia guardada la tarea que le solicita recursos.

Ahora, quiero copiar los datos de la página de la tarea llamadora, en la página de la tarea solicitante. Para copiar datos, los tenemos que sacar de la dirección fisica a la que apunta la dirección virtual de la tarea llamadora, y copiarlos en la direccion fisica a la que apunta la direccion virtual de la tarea que sollicita. Eso me suena al copypage que hicimos en el tp, hace exactamente eso.

Luego de todo eso, la tarea que estaba solicitandole ese recurso pasa a estado pausado, para indicar que está disponible para producir y ya no está esperando recursos. Y la tarea que avisó que terminó su recurso, debe restaurarse como si no se hubiera usado, o sea que vuelve a antes de que produzca algun recurso, usando  la funcion restaurar_tarea.

```C
#define PRODCUTORA_VIRT 0x0AAAA000
#define SOLICITADORA_VIRT 0xBBBB000

void recurso_listo(recurso_t recurso) {

    sched_entry_t* task_actual = &sched_tasks[current_task]
    task_id_t tarea_solicitadora = task_actual->produce_para;
    uint16_t selector_solicitadora = sched_tasks[tarea_solicitadora].selector;

    uint32_t cr3_productora = rcr3();
    uint32_t cr3_solicitadora = task_selector_to_cr3(selector_solicitadora);
    
    //obetenemos la direccion fisica de la tarea que produce y la que solicita, pueden no estar mapeadas
    //y en tal caso hay q limpiarlas, asi que hago los dos casos
    paddr_t phy_tarea_productora = virt_to_phy(cr3_productora, PRODCUTORA_VIRT);

    if (phy_tarea_productora == 0) {
        //si no esta mapeada, pido una pagina nueva al kernel, la mapeo y la limpio
        paddr_t phy_tarea_productora = mmu_next_free_kernel_page();
        mmu_map_page(cr3_productora, PRODCUTORA_VIRT, phy_tarea_productora, MMU_P | MMU_U | MMU_W);
        zero_page(PRODUCTORA_VIRT);
    }

    paddr_t phy_tarea_solicitadora = virt_to_phy(cr3_solicitadora, SOLICITADORA_VIRT);
    //hago lo mismo con la otra direccion
    if (phy_tarea_solicitadora == 0) {
        paddr_t phy_tarea_solicitadora = mmu_next_free_kernel_page();
        mmu_map_page(cr3_solicitadora, SOLICITADORA_VIRT, phy_tarea_solicitadora, MMU_P | MMU_U | MMU_W);
        zero_page(PRODUCTORA_VIRT);
    }
    
    //se mapearon las direcciones con atributos de usuario porq son tareas de nivel 3, si no se marca solo podría acceder el kernel
    //y las tareas llegarian a una excepcio nde proteccion. Y necesitan write para poder copiar los datos correctamente.

    copy_page(phy_tarea_solicitadora, phy_tarea_llamadora) //solicitadora es el desino y llamadora es source

    restaurar_tarea(current_task);
    sched_tasks[tarea_solicitadora].state = TASK_RUNNABLE;

}

//funcion definida en la clase pre parcial
uint32_t task_sel_to_cr3(int16_t selector) {
    uint16_t index = selector >> 3;
    gdt_entry_t* task_descriptor = &gdt[index];
    tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                            (task_descriptor->base_23_16 << 16) | 
                            (task_descriptor->base_15_0));
    return task_tss->cr3;
}

//funcion definida en clase pre parcial
paddr_t mmu_virt_to_phys(uint32_t cr3, vaddr_t virt) {
    pd_entry_t *pd = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);
    uint32_t pd_index = VIRT_PAGE_DIR(virt);
   
    pt_entry_t *pt = (pt_entry_t *)(pd_entry.pt << 12);
    uint32_t pt_index = VIRT_PAGE_TABLE(virt);

    paddr_t phys = pt[pt_index].page << 12;

    return phys
}
```

##2.

Queremos restaurar la tarea como si no se hubiese ejecutado. ASumiento que no se usaron paginas nuevas a parte de 0x0AAAA000 y 0x0BBBB000, debo desmapear esas paginas primero. Luego, como cuando la tarea llama a recurso_listo ya está produciendo, y nosotros queremos que vuelva a cuando no estaba produciendo, asi que para restaurarla deberia volver a setear sus valores a los que se usan cuando se crean las tareas en tss_create_user_task. Después dejo la tarea disponible para generar mas recursos.

```C
void restaurar_tarea(task_id_t id_tarea)

    //obtengo la tss de la tarea a restaurar usando el array de tss que tenemos en el kernel
    tss_t* tss = &tss_tasks(id_tarea);

    //tengo q restaurar la pila, uso los valores de tss_create_user_task
    tss->eip = TASK_CODE_VIRTUAL; 
    tss->esp = TASK_STACK_BASE;  
    tss->ebp = TASK_STACK_BASE;
    //reseteo los otros registros para que no me queden datos anteriores
    tss->eax = 0;
    tss->ebx = 0;
    tss->ecx = 0;
    tss->edx = 0;
    tss->esi = 0;
    tss->edi = 0;

    //ahora, con el cr3 de la tss, puedo desmapear las direcciones virtuales usadas como me piden
    paddr_t mmu_unmap_page(tss->cr3, 0X0AAAA000);
    paddr_t mmu_unmap_page(tss->cr3, 0X0BBBB000);

    //y por ultimo la tarea vuelve a pausarse porq ya terminó de producir y esta disponible para producir otro recurso
    sched_tasks[id_tarea].state = TASK_PAUSED;
    
}

```

##3.

Para que el kernel provea el arranque mannual mediante interrupción externa, necesitamos una interrupción de hardware. La consigna me dice que a la interrupción externa le corresponde la entrada 41 de la idt, asi que primero debo agregar esa entrada en la idt.
Esta vez, la entrada de idt debe ser de nivel 0 porque no queremos que tareas de usuario puedan ejecutar esta interrupción, asi que debe tener privilegios de kernel

```C
void idt_init() {
    //entradas anteriores usadas en el tp
    IDT_ENTRY0(41) //entry de la interrupcion externa
    //syscalls
}
```

Luego, definir el handler en isr.asm.
Aca antes de llamar a la funcion hay q llamar a pic_finish1, esta funcion le avisa al pic (que es un componente de hardware que gestiona las interrupciones) que el kernel terminó de atender la interrupción, y le permite al pic desbloquear la linea de interrupciones de hardware y aceptar nuevas. Sin esto, el sistema dejaria de recibir este tipo de interrupciones.
Despues quiero llamar a solicitar, pasandole por parametro el recurso_t q se encuentra en la direccion 0xFAFAFA del espacio del kernel. Para esto me voy a hacer una funcion aauxiliar en C, que me obtenga el recurso_t a partir de la direccion FAFAFA, y me de el struct en eax. Luego pusheo eax para pasarselo como parámetro a solicitar.

```ASM
extern obtener_recurso

global _isr41
_isr41:
    pushad
    call pic_finish1

    call obtener_recurso
    push eax
    call solicitar
    add esp, 4

    popad
    iret
```
```C
recurso_t obtener_recurso() {
    recurso_t res = *(recurso_t*)0xFAFAFA;
    return res;
}
```

##4.

Para modificar el sistema y almacenar qué recurso produce cada tarea, modificaria el sched_entry_t, como dije al principio del archivo. Agrego el campo recurso_t produce que me guarda qué recurso produce cada tarea.
Y para llevar registro de qué tareas están esperando que se liberen productoras del recurso que solicitan, agregue el campo TASK_WAITING_PRODUCE en task_state_t, que a su vez se guarda en el campo state de cada entry de tarea. Entonces siempre se puede consultar si una tarea esta pausada, corriendo, o esperando un recurso.

Para hay tarea disponible, quiero ver qué tareas están disponibles para producir el recurso pasado por parámetro. Para esto puedo iterar por todas las tareas que tiene el scheduler, donde cada indice es su task_id, y ver su estado. Una tarea que está esperando por un recurso no puede producir, ya que se pausa toda su ejecucion hasta que el recurso que quiere se termine de producir, asi que las tareas en ese estado no van a estar disponibles. Una tarea disponible para producción es una tarea pausada pero que no está esperando un recurso. Si una tarea no está pausada ni esperando recurso, está produciendo para otra tarea asi que no nos sirve. Si no hay ningun que esté disponible, devuelve 0

```C
task_id_t hay_tarea_disponible_para_recurso(recurso_t recurso) {

    for (task_id_t task_id = 0; task_id < MAX_TASKS; task_id++) {
        sched_entry_t* tarea = &sched_tasks[task_id];
        recurso_t recurso_que_prudce = tarea->produce;
        bool produce_recurso = (recurso_que_produce == recurso);

        if (tarea->state == TASK_PAUSED && produce_recurso) {
            return task_id;
        }
    }

    return 0;
}
```

Para quien produce, devuelve el id de la tarea que solicitó la produccion. Primero entro al sched_entry_t de la tarea y obtengo su state, si está produciendo sigo. Como en cada sched entry tengo un campo que me guarda el id de la tarea para quien produce, puedo simplemente consultarlo y devolverlo. Si el task_id no pertenece a ninguna entrada de sched_tasks, devuelvo 0.

```C
task_id_t para_quien_produce(task_id_t id_tarea) {

    sched_entry_t* task = &sched_tasks(id_tarea);
    if(task-> state == TASK_RUNNING) {
        task_id_t tarea_solicitadora = task->produce_para;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (sched_tasks[i] = sched_tasks[task_id]) {
                return tarea_solicitadora
            } else {
                continue;
            }
        }
        return 0;
    }
}
```

Para impelementar hay_consumidora_esperando, quiero ver si hay alguna tarea que este esperando que finalice la produccion del recurso pasado por parámetro. Para esto, puedo iterar por la lista de tareas del scheduler, donde cada indice es el task_id de la tarea, y así entrar al sched_entry_t de cada tarea. Una vez tengo eso, puedo ver los campos state y solicita, si está solicitando el recurso pasado por parametro y esta en estado waiting resource, devuelvo su id y termina el programa. Si no encuentra ninguno en todo el ciclo, sale y devuelve 0.

```C
}

 task_id_t hay_consumidora_esperando(recurso_t recurso) {
    
    for (int8_t task_id = 0; i < MAX_TASKS; i++ ) {
        shed_entry_t* tarea = &sched_tasks[task_id];
        recurso_t recurso_esperado = tarea -> solicita 
        if (recurso_esperado && (tarea->state == TASK_WAITING_RESOURCE)) {
            return task_id;
        }
    }
    return 0;
 }	
 ```
