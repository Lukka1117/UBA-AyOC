# Arquitectura de sistemas según Intel

Intel define la arquitectura de sistemas, o *system-level architecture*, como el conjunto de registros, estructuras de datos e instrucciones diseñados para soporte básico de operaciones de nivel de sistemas.  

Entre estas operaciones básicas están:
- Manejo de memoria
- Manejo de interrupciones y excepciones
- Manejo de tareas
- Control de múltiples procesadores  

Todos los procesadores de Intel, tanto 64 como IA-32, entran en **modo REAL** cuando encienden o cuando se resetean. A través de software se hace el cambio a **modo protegido**. En esta sección haremos una descripción detallada de cómo se logra este comportamiento.

---

## Modos de operación

La arquitectura IA-32 soporta tres modos de operación del procesador y un cuasi modo. En este trabajo nos interesan solo dos:

- **Modo protegido**  
  Es el modo operativo nativo del procesador. En este modo se provee la mayoría de las características arquitecturales: flexibilidad, alta performance y retrocompatibilidad con software base existente.

- **Modo real**  
  Este modo provee el ambiente de programación del procesador Intel 8086, con algunas extensiones, como por ejemplo la capacidad de cambiar a modo protegido.  

Como aclaramos antes, el procesador entra en modo real cada vez que se resetea o se enciende. El flag **PE** en el registro de control **CR0** controla si el procesador está operando en modo real o modo protegido.

---

## ¿Por qué no se puede hacer un sistema operativo en modo real?

Se puede, pero el problema es que carece de todas las características principales de un sistema moderno como:
- Protección de memoria
- Multitasking
- Redireccionamiento de memoria extenso  

El modo real se mantiene en la fabricación de procesadores por la política de Intel de retrocompatibilidad.  

La **BIOS/UEFI system** de las computadoras arranca en modo real y después pasa a modo protegido. Con esto, Intel se asegura de mantener compatibilidad con cualquier software que usara características del 8086 y el costo de mantenerlo es casi inexistente.  

¿Podrían dejar de hacer procesadores con modo real?  
Sí, pero Intel lo mantiene por retrocompatibilidad con **legacy OS**, como dijimos antes.

---

## Manejo de memoria

El manejo de memoria de la arquitectura IA-32 se divide en dos partes:

- **Segmentación**: provee un mecanismo para aislar código, datos y stack, de manera que múltiples programas puedan correr en el mismo procesador sin interferirse.  
- **Paginación**: provee un mecanismo para implementar demanda de páginas en un sistema de memoria virtual, donde las secciones de un programa en ejecución se mapean a memoria física solo cuando se requieren.

No hay manera de deshabilitar la segmentación en procesadores Intel. El uso de la paginación es opcional, pero es lo más utilizado.

---

## Descriptores de segmento

Los descriptores de segmento son estructuras de datos que se encuentran en una tabla, la cual puede ser:

- **GDT** (*Global Descriptor Table*)  
- **LDT** (*Local Descriptor Table*)  

Las entradas de la GDT/LDT son los descriptores de segmento. Estos proveen al procesador el tamaño y la ubicación de los segmentos.  

Todos los descriptores de segmento están conformados por un registro de **8 bytes**, que Intel representa como dos palabras de 32 bits cada una.

### Campos importantes del descriptor

- **Campo Segment Limit**  
  Especifica el largo del segmento. Es un número de 20 bits, formado por la unión de partes de la primera y segunda palabra.  
  Rango: `0x00000` a `0xFFFFF` (1 MB).  
  Depende del flag **G (granularidad):**
  - `G = 0`: tamaño de 1 byte a 1 MB, en incrementos de 1 byte.  
  - `G = 1`: tamaño de 4 KB a 4 GB, en incrementos de 4 KB.  

- **Campo Base Address**  
  Define la dirección base del segmento (byte 0). Es un número de 32 bits formado por distintas secciones del descriptor.  
  Debe estar alineada a **16 bytes** (compatibilidad y performance).

- **Flag G (Granularity)**  
  Determina la escala del campo Segment Limit:  
  - `G = 0`: límite en bytes.  
  - `G = 1`: límite en bloques de 4 KB.  
  En este caso los **12 bits menos significativos del offset** se ignoran.  
  Ejemplo: con `G=1` y `L=0`, el tamaño mínimo es 4 KB y los offsets válidos van de 0 a 4095.

- **Flag P (Present)**  
  Indica si el segmento está presente en memoria. Si es 0, el procesador genera una excepción.  

- **Flag DPL (Descriptor Privilege Level)**  
  Nivel de privilegio (0 a 3).  
  - 0 = más privilegiado  
  - 3 = menos privilegiado  

- **Flag S (System/Code-Data)**  
  Especifica si un descriptor corresponde a un segmento de sistema o a uno de código/datos.

---

## Segmentos de código y datos

Cuando el bit **S** está encendido, el descriptor indica código o datos. El **bit 11** indica si es de datos o código.

### Tabla de tipos de segmentos

| Dec | T | W/R | E/C | A | Type | Description |
|-----|---|-----|-----|---|------|-------------|
| 0   | 0 | 0   | 0   | 0 | Data | Read-Only |
| 1   | 0 | 0   | 0   | 1 | Data | Read-Only, accessed |
| 2   | 0 | 0   | 1   | 0 | Data | Read/Write |
| 3   | 0 | 0   | 1   | 1 | Data | Read/Write, accessed |
| 4   | 0 | 1   | 0   | 0 | Data | Read-Only, expand-down |
| 5   | 0 | 1   | 0   | 1 | Data | Read-Only, expand-down, accessed |
| 6   | 0 | 1   | 1   | 0 | Data | Read/Write, expand-down |
| 7   | 0 | 1   | 1   | 1 | Data | Read/Write, expand-down, accessed |
| 8   | 1 | 0   | 0   | 0 | Code | Execute-Only |
| 9   | 1 | 0   | 0   | 1 | Code | Execute-Only, accessed |
| 10  | 1 | 0   | 1   | 0 | Code | Execute/Read |
| 11  | 1 | 0   | 1   | 1 | Code | Execute/Read, accessed |
| 12  | 1 | 1   | 0   | 0 | Code | Execute-Only, conforming |
| 13  | 1 | 1   | 0   | 1 | Code | Execute-Only, conforming, accessed |
| 14  | 1 | 1   | 1   | 0 | Code | Execute/Read, conforming |
| 15  | 1 | 1   | 1   | 1 | Code | Execute/Read, conforming, accessed |

---

### Ejemplo

Si queremos especificar un segmento para ejecución y lectura de código, deberíamos usar:

- `1010` → Execute/Read  
- `1011` → Execute/Read, accessed  


