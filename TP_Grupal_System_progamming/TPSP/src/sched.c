/* ** por compatibilidad se omiten tildes **
================================================================================
 TALLER System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones del scheduler
*/

#include "sched.h"

#include "i386.h"
#include "kassert.h"

/**
 * Estados posibles de una tarea en el scheduler:
 * - `TASK_SLOT_FREE`: No existe esa tarea
 * - `TASK_RUNNABLE`: La tarea se puede ejecutar
 * - `TASK_PAUSED`: La tarea se registró al scheduler pero está pausada
 */
typedef enum {
  TASK_SLOT_FREE,
  TASK_RUNNABLE,
  TASK_PAUSED
} task_state_t;

/**
 * Estructura usada por el scheduler para guardar la información pertinente de
 * cada tarea.
 */
typedef struct {
  int16_t selector;
  task_state_t state;
} sched_entry_t;

static sched_entry_t sched_tasks[MAX_TASKS] = {0};

/**
 * Tarea actualmente en ejecución (excepto que esté pasuada, en cuyo caso se
 * corre la idle).
 */
int8_t current_task = 0;

/**
 * Agrega una tarea al primer slot libre
 *
 * @param selector el selector de la GDT de la tarea
 * @returns el identificador de la tarea creada
 */
int8_t sched_add_task(uint16_t selector) {
  kassert(selector != 0, "No se puede agregar el selector nulo");

  // Se busca el primer slot libre para agregar la tarea
  for (int8_t i = 0; i < MAX_TASKS; i++) {
    if (sched_tasks[i].state == TASK_SLOT_FREE) {
      sched_tasks[i] = (sched_entry_t) {
        .selector = selector,
	.state = TASK_PAUSED,
      };
      return i;
    }
  }
  kassert(false, "No task slots available");
}

/**
 * Deshabilita una tarea en el scheduler
 *
 * @param task_id la tarea a deshabilitar
 */
void sched_disable_task(int8_t task_id) {
  kassert(task_id >= 0 && task_id < MAX_TASKS, "Invalid task_id");
  sched_tasks[task_id].state = TASK_PAUSED;
}

/**
 * Habilita un tarea en el scheduler
 *
 * @param task_id la tarea a habilitar
 */
void sched_enable_task(int8_t task_id) {
  kassert(task_id >= 0 && task_id < MAX_TASKS, "Invalid task_id");
  sched_tasks[task_id].state = TASK_RUNNABLE;
}

/**
 * Inicializa el scheduler
 */
void sched_init(void) {
  for (size_t i = 0; i < MAX_TASKS; i++) {
    sched_tasks[i].state = TASK_SLOT_FREE;
  }
}

/**
 * Obtiene la siguiente tarea disponible con una política round-robin. Si no
 * hay tareas disponibles, se salta a la tarea Idle.
 *
 * @return uint16_t el selector de segmento de la tarea a saltar
 */
uint16_t sched_next_task(void) {
  // Buscamos la próxima tarea viva (comenzando en la actual)
  int8_t i;
  for (i = (current_task + 1); (i % MAX_TASKS) != current_task; i++) {
    // Si esta tarea está disponible la ejecutamos
    if (sched_tasks[i % MAX_TASKS].state == TASK_RUNNABLE) {
      break;
    }
  }

  // Ajustamos i para que esté entre 0 y MAX_TASKS-1
  i = i % MAX_TASKS;

  // Si la tarea que encontramos es ejecutable entonces vamos a correrla.
  if (sched_tasks[i].state == TASK_RUNNABLE) {
    current_task = i;
    return sched_tasks[i].selector;
  }

  // En el peor de los casos no hay ninguna tarea viva. Usemos la idle como
  // selector.
  return GDT_IDX_TASK_IDLE << 3;
}


/*

void garbage_collector() {
  while (true) {
    for ( i = 0 ; i < MAX_TAREAS; i ++) {
      uint32_t cr3_actual = task_id_to_cr3(i);
      reservas_por_tarea* reservas = dameReservas(i);
      for (j = 0 ; j < reservas->reservas_size; j++) {
        reserva_t* reserva = &(reservas->array_reservas[j]);
        if(reserva->estado == 2) {  //si esta para liberar la libera
          //desmapear la reserva
          //necesito el numero de paginas a liberar, se asume q el tamaño es multiplo de page size
          uint32_t num_pages = reserva->tamanio / PAGE_SIZE; //se divide para convertir de bytes a la cantidad de paginas necesarias
          vaddr_t vir = reserva->virt;

          for (k = 0; k < num_pages; k++) {
            mmu_unmap_page(cr3_actual, vir);
            vir += PAGE_SIZE;   //avanzo a la siguiente pagina;
          }
        }
        reserva->estado = 3; //se libero la reserva;
      }
    }
  }
}

task_id_to_cr3(int8_t task_id) {
 uint16_t index = sched_tasks[task_id].selector >> 3;
 gdt_entry_t* task_descriptor = &gdt[index];
 tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                            (task_descriptor->base_23_16 << 16) | 
                            (task_descriptor->base_15_0));
 return task_tss->cr3;
}

*/