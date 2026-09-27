#include "RTE_Components.h"
#include CMSIS_device_header
#include "USART.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Declaración de funciones externas en Ensamblador
extern void get_cpu_registers(uint32_t *reg_array);
extern void set_register_value(uint32_t *reg_array, uint32_t reg_num, uint32_t val);



char out_buf[128];
uint32_t cpu_context[16] = {0}; // Banco de registros del monitor

void process_command(char *cmd) {
    char op[10];
    char arg1[20] = {0}, arg2[20] = {0};
    
    int parsed = sscanf(cmd, "%s %s %s", op, arg1, arg2);
    if (parsed <= 0) return;

    // 1. Comando RD (Register Display)
    if (strcasecmp(op, "RD") == 0) {
        // Obtenemos los registros reales del CPU mediante Ensamblador
        get_cpu_registers(cpu_context);
        
        USART_putString("\r\n--- Registros del CPU ---\r\n");
        for (int k = 0; k <= 12; k++) {
            sprintf(out_buf, "R%-2d: 0x%08X\r\n", k, (unsigned int)cpu_context[k]);
            USART_putString(out_buf);
        }
        sprintf(out_buf, "SP : 0x%08X\r\nLR : 0x%08X\r\nPC : 0x%08X\r\n", 
                (unsigned int)cpu_context[13], 
                (unsigned int)cpu_context[14], 
                (unsigned int)cpu_context[15]);
        USART_putString(out_buf);
    }
    
    // 2. Comando RM (Register Modify) -> Sintaxis: RM R3 1A2B3C4D
    else if (strcasecmp(op, "RM") == 0) {
        if (parsed < 3) {
            USART_putString("\r\nSintaxis correcta: RM R{X} data\r\n");
            return;
        }

        int reg_num = -1;
        // Parsear "R4" o "r4" para extraer el entero 4
        if (arg1[0] == 'R' || arg1[0] == 'r') {
            reg_num = atoi(&arg1[1]);
        }

        // Convertir la cadena hex ("1A2B3C4D") a valor numérico
        uint32_t val = (uint32_t)strtoul(arg2, NULL, 16);

        if (reg_num >= 0 && reg_num <= 12) {
            // Llamada a la función en Ensamblador para modificar el registro
            set_register_value(cpu_context, (uint32_t)reg_num, val);
            
            sprintf(out_buf, "\r\nRegistro R%d actualizado a: 0x%08X en ensamblador\r\n", reg_num, (unsigned int)val);
            USART_putString(out_buf);
        } else {
            USART_putString("\r\nRegistro invalido. Solo se permite modificar de R0 a R12.\r\n");
        }
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
            cmd_ready = 0;
            n = 0;
            USART_putString(">> ");
        }
    }
}