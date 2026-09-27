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
    else {
        USART_putString("\r\nComando no reconocido.\r\n");
    }
}

int main(void) {
    USART_config(115200);
    
    USART_putString("\r\n========================================\r\n");
    USART_putString("    Proyecto Final Microprocesadores    \r\n");
    USART_putString("            Programa Monitor            \r\n");
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