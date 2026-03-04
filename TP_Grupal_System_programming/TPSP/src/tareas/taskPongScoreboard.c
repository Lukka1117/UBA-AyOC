#include "task_lib.h"

#define WIDTH TASK_VIEWPORT_WIDTH
#define HEIGHT TASK_VIEWPORT_HEIGHT

#define SHARED_SCORE_BASE_VADDR (PAGE_ON_DEMAND_BASE_VADDR + 0xF00)
#define CANT_PONGS 3

void task(void)
{
	screen pantalla;
	// ¿Una tarea debe terminar en nuestro sistema?
	while (true)
	{
		uint32_t *p = (uint32_t *)SHARED_SCORE_BASE_VADDR;
		task_print_dec(pantalla, p[0], 3, 5, 5, C_FG_CYAN);
		task_print(pantalla, "-", 9, 5, C_FG_RED);
		task_print_dec(pantalla, p[1], 3, 11, 5, C_FG_WHITE);

		task_print_dec(pantalla, p[2], 3, 5, 7, C_FG_CYAN);
		task_print(pantalla, "-", 9, 7, C_FG_RED);
		task_print_dec(pantalla, p[3], 3, 11, 7, C_FG_CYAN);

		task_print_dec(pantalla, p[4], 3, 5, 9, C_FG_WHITE);
		task_print(pantalla, "-", 9, 9, C_FG_RED);
		task_print_dec(pantalla, p[5], 3, 11, 9, C_FG_WHITE);
		// Completar:

		// - Pueden definir funciones auxiliares para imprimir en pantalla
		// - Pueden usar `task_print`, `task_print_dec`, etc.

		syscall_draw(pantalla);
	}
}

void print_puntaje(screen pantalla, uint32_t scorep1, uint32_t scorep2, uint16_t x, uint16_t y)
{

	task_print_dec(pantalla, scorep1, 3, x, y, C_FG_WHITE);
	task_print(pantalla, '-', x, y + 4, C_FG_WHITE);
	task_print_dec(pantalla, scorep2, 3, x, y + 5, C_FG_WHITE);
}