/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "defines.h"
#include "i386.h"

#include "kassert.h"

static pd_entry_t *kpd = (pd_entry_t *)KERNEL_PAGE_DIR;
static pt_entry_t *kpt = (pt_entry_t *)KERNEL_PAGE_TABLE_0;

static const uint32_t identity_mapping_end = 0x003FFFFF;
static const uint32_t user_memory_pool_end = 0x02FFFFFF;

static paddr_t next_free_kernel_page = 0x100000;
static paddr_t next_free_user_page = 0x400000;

/**
 * kmemset asigna el valor c a un rango de memoria interpretado
 * como un rango de bytes de largo n que comienza en s
 * @param s es el puntero al comienzo del rango de memoria
 * @param c es el valor a asignar en cada byte de s[0..n-1]
 * @param n es el tamaño en bytes a asignar
 * @return devuelve el puntero al rango modificado (alias de s)
 */
static inline void *kmemset(void *s, int c, size_t n)
{
  uint8_t *dst = (uint8_t *)s;
  for (size_t i = 0; i < n; i++)
  {
    dst[i] = c;
  }
  return dst;
}

/**
 * zero_page limpia el contenido de una página que comienza en addr
 * @param addr es la dirección del comienzo de la página a limpiar
 */
static inline void zero_page(paddr_t addr)
{
  kmemset((void *)addr, 0x00, PAGE_SIZE);
}

void mmu_init(void) {}

/**
 * mmu_next_free_kernel_page devuelve la dirección física de la próxima página
 * de kernel disponible. Las páginas se obtienen en forma incremental, siendo la
 * primera: next_free_kernel_page
 * @return devuelve la dirección de memoria de comienzo de la próxima página
 * libre de kernel
 */
paddr_t mmu_next_free_kernel_page(void)
{
  paddr_t current_free_kernel_page = next_free_kernel_page;
  next_free_kernel_page += PAGE_SIZE;

  return current_free_kernel_page;
}

/**
 * mmu_next_free_user_page devuelve la dirección de la próxima página de usuarix
 * disponible
 * @return devuelve la dirección de memoria de comienzo de la próxima página
 * libre de usuarix
 */
paddr_t mmu_next_free_user_page(void)
{
  paddr_t current_free_user_page = next_free_user_page;
  next_free_user_page += PAGE_SIZE;

  return current_free_user_page;
}

/**
 * mmu_init_kernel_dir inicializa las estructuras de paginación vinculadas al
 * kernel y realiza el identity mapping
 * @return devuelve la dirección de memoria de la página donde se encuentra el
 * directorio de páginas usado por el kernel
 */
paddr_t mmu_init_kernel_dir(void)
{
  kpd[0] = (pd_entry_t){.attrs = MMU_P | MMU_W, .pt = ((uint32_t)kpt) >> 12};

  for (int i = 0; i < 1024; ++i)
    kpt[i] = (pt_entry_t){.attrs = MMU_P | MMU_W, .page = i};

  return (paddr_t)kpd;
}

/**
 * mmu_map_page agrega las entradas necesarias a las estructuras de paginación
 * de modo de que la dirección virtual virt se traduzca en la dirección física
 * phy con los atributos definidos en attrs
 * @param cr3 el contenido que se ha de cargar en un registro CR3 al realizar la
 * traducción
 * @param virt la dirección virtual que se ha de traducir en phy
 * @param phy la dirección física que debe ser accedida (dirección de destino)
 * @param attrs los atributos a asignar en la entrada de la tabla de páginas
 */
void mmu_map_page(uint32_t cr3, vaddr_t virt, paddr_t phy, uint32_t attrs)
{
  pd_entry_t *pd = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);
  uint32_t pd_index = VIRT_PAGE_DIR(virt);
  pd_entry_t *pd_entry = &pd[pd_index];

  if ((pd_entry->attrs & MMU_P) == 0)
  {
    paddr_t pt = mmu_next_free_kernel_page();
    zero_page(pt);

    pd[pd_index] = (pd_entry_t){.attrs = attrs, .pt = (uint32_t)pt >> 12};
  }

  pd[pd_index].attrs |= attrs;

  pt_entry_t *pt = (pt_entry_t *)(pd_entry->pt << 12);
  uint32_t pt_index = VIRT_PAGE_TABLE(virt);

  pt[pt_index] = (pt_entry_t){.attrs = attrs, .page = phy >> 12};
  tlbflush();
}

/**
 * mmu_unmap_page elimina la entrada vinculada a la dirección virt en la tabla
 * de páginas correspondiente
 * @param virt la dirección virtual que se ha de desvincular
 * @return la dirección física de la página desvinculada
 */
paddr_t mmu_unmap_page(uint32_t cr3, vaddr_t virt)
{
  pd_entry_t *pd = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);
  uint32_t pd_index = VIRT_PAGE_DIR(virt);
  pd_entry_t pd_entry = pd[pd_index]; // entramos a la tabla del directorio

  pt_entry_t *pt = (pt_entry_t *)(pd_entry.pt << 12);
  uint32_t pt_index = VIRT_PAGE_TABLE(virt);

  pt_entry_t pt_entry = pt[pt_index];
  paddr_t adress_page = (paddr_t)(MMU_ENTRY_PADDR(pt_entry.page));
  pt[pt_index] = (pt_entry_t){.attrs = 0x0, .page = 0x0};
  tlbflush();
  return adress_page;
}

#define DST_VIRT_PAGE 0xA00000
#define SRC_VIRT_PAGE 0xB00000

/**
 * copy_page copia el contenido de la página física localizada en la dirección
 * src_addr a la página física ubicada en dst_addr
 * @param dst_addr la dirección a cuya página queremos copiar el contenido
 * @param src_addr la dirección de la página cuyo contenido queremos copiar
 *
 * Esta función mapea ambas páginas a las direcciones SRC_VIRT_PAGE y
 * DST_VIRT_PAGE, respectivamente, realiza la copia y luego desmapea las
 * páginas. Usar la función rcr3 definida en i386.h para obtener el cr3 actual
 */
void copy_page(paddr_t dst_addr, paddr_t src_addr)
{
  uint32_t cr3_actual = rcr3();
  mmu_map_page(cr3_actual, DST_VIRT_PAGE, dst_addr, MMU_P | MMU_W);
  mmu_map_page(cr3_actual, SRC_VIRT_PAGE, src_addr, MMU_P);

  // typedef struct { uint8_t data[PAGE_SIZE]; } page;
  //*((page*)DST_VIRT_PAGE) = *((page*)SRC_VIRT_PAGE);
  // OTRA MANERA

  for (int i = 0; i < PAGE_SIZE; i++)
  {
    ((uint8_t *)DST_VIRT_PAGE)[i] = ((uint8_t *)SRC_VIRT_PAGE)[i];
  }
  mmu_map_page(cr3_actual, DST_VIRT_PAGE, dst_addr, MMU_P | MMU_W);
  mmu_map_page(cr3_actual, SRC_VIRT_PAGE, src_addr, MMU_P);

  // typedef struct { uint8_t data[PAGE_SIZE]; } page;
  //*((page*)DST_VIRT_PAGE) = *((page*)SRC_VIRT_PAGE);
  // OTRA MANERA

  for (int i = 0; i < PAGE_SIZE; i++)
  {
    ((uint8_t *)DST_VIRT_PAGE)[i] = ((uint8_t *)SRC_VIRT_PAGE)[i];
  }
  mmu_unmap_page(cr3_actual, SRC_VIRT_PAGE);
  mmu_unmap_page(cr3_actual, DST_VIRT_PAGE);
}

/**
 * mmu_init_task_dir inicializa las estructuras de paginación vinculadas a una
 * tarea cuyo código se encuentra en la dirección phy_start
 * @pararm phy_start es la dirección donde comienzan las dos páginas de código
 * de la tarea asociada a esta llamada
 * @return el contenido que se ha de cargar en un registro CR3 para la tarea
 * asociada a esta llamada
 */
paddr_t mmu_init_task_dir(paddr_t phy_start)
{

  pd_entry_t *cr3 = (pd_entry_t *)mmu_next_free_kernel_page();
  zero_page((paddr_t)cr3);

  pt_entry_t *task_pt = (pt_entry_t *)mmu_next_free_kernel_page();

  cr3[0] =
      (pd_entry_t){.attrs = MMU_P | MMU_W, .pt = ((uint32_t)task_pt) >> 12};

  // TODO: PREGUNTAR SI ESTO ES UNA i O mmu_next_free_user_page();
  for (int i = 0; i < 1024; ++i)
    task_pt[i] = (pt_entry_t){.attrs = MMU_P | MMU_W, .page = i};

  uint32_t cr3_r = (uint32_t)cr3;
  mmu_map_page(cr3_r, TASK_CODE_VIRTUAL, phy_start, MMU_U | MMU_P);
  mmu_map_page(cr3_r, TASK_CODE_VIRTUAL + PAGE_SIZE, phy_start + PAGE_SIZE,
               MMU_U | MMU_P);

  mmu_map_page(cr3_r, TASK_STACK_BASE - PAGE_SIZE, mmu_next_free_user_page(),
               MMU_U | MMU_P | MMU_W);
  mmu_map_page(cr3_r, TASK_SHARED_PAGE, SHARED_MEM_START, MMU_U | MMU_P);

  return cr3_r;
}

// COMPLETAR: devuelve true si se atendió el page fault y puede continuar la
// ejecución y false si no se pudo atender
bool page_fault_handler(vaddr_t virt)
{
  print("Atendiendo page fault...", 0, 0, C_FG_WHITE | C_BG_BLACK);
  // Chequeemos si el acceso fue dentro del area on-demand
  // En caso de que si, mapear la pagina
  if (virt >= ON_DEMAND_MEM_START_VIRTUAL && virt < ON_DEMAND_MEM_END_VIRTUAL)
  {
    uint32_t cr3 = rcr3();

    mmu_map_page(cr3, ON_DEMAND_MEM_START_VIRTUAL, ON_DEMAND_MEM_START_PHYSICAL,
                 MMU_P | MMU_U | MMU_W);
    return 1;
  }

  return 0;
}
