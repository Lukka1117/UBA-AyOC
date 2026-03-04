usa asi los bloques de codigo

```C
uint32_t task_id_to_cr3(int8_t task_id) {
    uint16_t index = sched_tasks[task_id].selector >> 3;
    gdt_entry_t* task_descriptor = &gdt[index];
    tss_t* task_tss = (tss_t*)((task_descriptor->base_31_24 << 24) | 
                               (task_descriptor->base_23_16 << 16) | 
                               (task_descriptor->base_15_0));
    return task_tss->cr3;
}
```