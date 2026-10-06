#include "RTE_Components.h"
#include CMSIS_device_header
#include "USART.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

extern void get_cpu_registers(uint32_t *reg_array);
extern void set_cpu_register(uint32_t *reg_array, uint32_t reg_num, uint32_t val);
extern void jump_to_address(uint32_t addr);
extern uint32_t call_subroutine(uint32_t addr, uint32_t arg0, uint32_t arg1, uint32_t arg2);


char out_buf[128]; 
uint32_t monitor_regs[16] = {0};
uint8_t regs_initialized = 0;

void process_command(char *cmd) {
    char op[10] = {0};
    char arg1[20] = {0}, arg2[20] = {0}, arg3[20] = {0}, arg4[20] = {0};
    
    // 1. Limpiar saltos de línea (\r y \n) del final de la cadena
    int len = strlen(cmd);
    for (int i = 0; i < len; i++) {
        if (cmd[i] == '\r' || cmd[i] == '\n') {
            cmd[i] = '\0';
            break;
        }
    }

    // 2. Extraer el comando y los argumentos
    int parsed = sscanf(cmd, "%s %s %s %s %s", op, arg1, arg2, arg3, arg4);
    if (parsed <= 0) return;

    if (!regs_initialized) {
        get_cpu_registers(monitor_regs);
        regs_initialized = 1;
    }

    // 1. Register Display (RD)
    if (strcasecmp(op, "RD") == 0) {
        uint32_t current_cpu[16];
        get_cpu_registers(current_cpu);
        monitor_regs[13] = current_cpu[13]; // SP
        monitor_regs[14] = current_cpu[14]; // LR
        monitor_regs[15] = current_cpu[15]; // PC

        USART_putString("\r\n--- Registros del CPU ---\r\n");
        for (int k = 0; k <= 12; k++) {
            sprintf(out_buf, "R%-2d: 0x%08X\r\n", k, (unsigned int)monitor_regs[k]);
            USART_putString(out_buf);
        }
        sprintf(out_buf, "SP : 0x%08X\r\nLR : 0x%08X\r\nPC : 0x%08X\r\n", 
                (unsigned int)monitor_regs[13], 
                (unsigned int)monitor_regs[14], 
                (unsigned int)monitor_regs[15]);
        USART_putString(out_buf);
    }
    
    // 2. Register Modify (RM R{X} data)
    else if (strcasecmp(op, "RM") == 0) {
        if (parsed < 3) {
            USART_putString("\r\nSintaxis correcta: RM R4 0x1234ABCD\r\n");
            return;
        }

        int reg_num = -1;
        // Permite leer "R4" o "r4"
        if (arg1[0] == 'R' || arg1[0] == 'r') {
            reg_num = atoi(&arg1[1]);
        }

        // Convertir el texto hexadecimal a número entero
        uint32_t val = (uint32_t)strtoul(arg2, NULL, 16);

        if (reg_num >= 0 && reg_num <= 12) {
            set_cpu_register(monitor_regs, (uint32_t)reg_num, val);
            sprintf(out_buf, "\r\nRegistro R%d actualizado a: 0x%08X\r\n", reg_num, (unsigned int)val);
            USART_putString(out_buf);
        } else {
            USART_putString("\r\nRegistro invalido. Debe ser de R0 a R12.\r\n");
        }
    }
    // 3. Memory Display (MD [start] [end])
    else if (strcasecmp(op, "MD") == 0) {
        uint32_t start = 0x20000000; // Dirección inicial por defecto (SRAM)
        uint32_t end = start + 0x40;  // 64 bytes por defecto (0x40)

        // Si el usuario ingresa inicio
        if (parsed >= 2) {
            start = (uint32_t)strtoul(arg1, NULL, 16);
            end = start + 0x40; // Mantener un rango de 64 bytes si no da el final
        }
        // Si el usuario ingresa inicio y fin
        if (parsed >= 3) {
            end = (uint32_t)strtoul(arg2, NULL, 16);
        }

        // Ajustar dirección inicio a alineación de 4 bytes
        start &= ~0x03;

        USART_putString("\r\n--- Despliegue de Memoria (Hex Dump) ---\r\n");
        for (uint32_t addr = start; addr <= end; addr += 4) {
            uint32_t val = *(uint32_t *)addr;
            sprintf(out_buf, "0x%08X: 0x%08X\r\n", (unsigned int)addr, (unsigned int)val);
            USART_putString(out_buf);
        }
    }
    // 4. Memory Modify (MM addr data [size])
    else if (strcasecmp(op, "MM") == 0) {
        if (parsed < 3) {
            USART_putString("\r\nSintaxis: MM addr data [size]\r\n");
            return;
        }

        uint32_t addr = (uint32_t)strtoul(arg1, NULL, 16);
        uint32_t data = (uint32_t)strtoul(arg2, NULL, 16);
        int size = (parsed >= 4) ? atoi(arg3) : 1; // Default a 1 byte si no se especifica

        // Validar que la dirección pertenezca a la memoria RAM (0x20000000)
        if (addr < 0x20000000 || addr > 0x20007FFF) {
            USART_putString("\r\nError: Direccion fuera del rango de la SRAM segura.\r\n");
            return;
        }

        if (size == 1) {
            *(uint8_t *)addr = (uint8_t)data;
        } else if (size == 2) {
            *(uint16_t *)addr = (uint16_t)data;
        } else if (size == 4) {
            *(uint32_t *)addr = (uint32_t)data;
        } else {
            USART_putString("\r\nError: Tamanio invalido (Usar 1, 2 o 4 bytes).\r\n");
            return;
        }

        sprintf(out_buf, "\r\nMemoria 0x%08X modificada con valor 0x%X (tamanio: %d byte/s)\r\n", 
                (unsigned int)addr, (unsigned int)data, size);
        USART_putString(out_buf);
    }
    // 5. Block Fill (BF start end data [size])
    else if (strcasecmp(op, "BF") == 0) {
        if (parsed < 4) {
            USART_putString("\r\nSintaxis: BF start end data [size]\r\n");
            return;
        }

        uint32_t start = (uint32_t)strtoul(arg1, NULL, 16);
        uint32_t end   = (uint32_t)strtoul(arg2, NULL, 16);
        uint32_t data  = (uint32_t)strtoul(arg3, NULL, 16);
        int size = (parsed >= 5) ? atoi(arg4) : 1; // Default: 1 byte

        // Validar direcciones dentro de la SRAM
        if (start < 0x20000000 || end > 0x20007FFF || start > end) {
            USART_putString("\r\nError: Rango de memoria invalido o fuera de SRAM.\r\n");
            return;
        }

        // Validar tamaño permitido
        if (size != 1 && size != 2 && size != 4) {
            USART_putString("\r\nError: Tamanio invalido (usar 1, 2 o 4 bytes).\r\n");
            return;
        }

        // Rellenar la memoria
        for (uint32_t addr = start; addr <= end; addr += size) {
            if (size == 1)      *(uint8_t *)addr  = (uint8_t)data;
            else if (size == 2) *(uint16_t *)addr = (uint16_t)data;
            else if (size == 4) *(uint32_t *)addr = (uint32_t)data;
        }

        sprintf(out_buf, "\r\nBloque 0x%08X - 0x%08X rellenado con 0x%X (size: %d)\r\n", 
                (unsigned int)start, (unsigned int)end, (unsigned int)data, size);
        USART_putString(out_buf);
    }
    // 6. Run Code (RUN addr)
    else if (strcasecmp(op, "RUN") == 0) {
        if (parsed < 2) {
            USART_putString("\r\nSintaxis: RUN addr\r\n");
            return;
        }

        uint32_t target_addr = (uint32_t)strtoul(arg1, NULL, 16);

        // Validar dirección dentro de la SRAM
        if (target_addr < 0x20000000 || target_addr > 0x20007FFF) {
            USART_putString("\r\nError: Direccion fuera del rango de la SRAM.\r\n");
            return;
        }

        sprintf(out_buf, "\r\nEjecutando codigo en 0x%08X...\r\n", (unsigned int)target_addr);
        USART_putString(out_buf);

        // Llamar a la subrutina en ensamblador para saltar a la dirección
        jump_to_address(target_addr);

        // Si el código ejecutado en RAM hace un "BX LR", volverá a esta línea:
        USART_putString("\r\nRetorno exitoso de la ejecucion.\r\n");
    }
    else if (strcasecmp(op, "CALL") == 0) {
        if (parsed < 2) {
            USART_putString("\r\nSintaxis: CALL addr [arg1] [arg2] [arg3]\r\n");
            return;
        }

        uint32_t target_addr = (uint32_t)strtoul(arg1, NULL, 16);
        uint32_t a0 = (parsed >= 3) ? (uint32_t)strtoul(arg2, NULL, 16) : 0;
        uint32_t a1 = (parsed >= 4) ? (uint32_t)strtoul(arg3, NULL, 16) : 0;
        uint32_t a2 = (parsed >= 5) ? (uint32_t)strtoul(arg4, NULL, 16) : 0;

        // Validar dirección dentro de la SRAM
        if (target_addr < 0x20000000 || target_addr > 0x20007FFF) {
            USART_putString("\r\nError: Direccion fuera del rango de la SRAM.\r\n");
            return;
        }

        sprintf(out_buf, "\r\nLlamando subrutina en 0x%08X...\r\n", (unsigned int)target_addr);
        USART_putString(out_buf);

        // Ejecutar la subrutina
        uint32_t res = call_subroutine(target_addr, a0, a1, a2);

        // Mostrar el resultado devuelto en R0 por la subrutina
        sprintf(out_buf, "Subrutina finalizada. Valor retornado (R0): 0x%08X\r\n", (unsigned int)res);
        USART_putString(out_buf);
    }
    else {
        USART_putString("\r\nComando no reconocido.\r\n");
    }
}
int main(void) {
    USART_config(115200);
    
    USART_putString("\r\n========================================\r\n");
    USART_putString("    Proyecto Final Microprocesadores    \r\n");
    USART_putString("            Programa Monitor            \r\n");
    USART_putString("              Teresa Rivera            \r\n");
    USART_putString("========================================\r\n\r\n");
    USART_putString(">> ");
    
    while(1) {
        if(cmd_ready) {
            process_command((char*)rx_data);
            
            // Limpiar el buffer de recepción para el siguiente comando
            memset((void*)rx_data, 0, sizeof(rx_data));
            cmd_ready = 0;
            n = 0;
            
            USART_putString(">> ");
        }
    }
}