        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB

        EXPORT get_cpu_registers
        EXPORT set_register_value

;-------------------------------------------------------------------------------
; void get_cpu_registers(uint32_t *reg_array);
; R0 contiene la dirección del arreglo uint32_t regs[16]
;-------------------------------------------------------------------------------
get_cpu_registers PROC
    PUSH    {R1, R2}           ; Guardar R1 y R2 en la pila para usarlos de comodín
    MOV     R1, R0             ; Copiar el puntero base a R1
    
    ; Recuperar y guardar el R0 original que traía la aplicación
    ; (está en la pila debido al PUSH)
    LDR     R2, [SP, #4]
    STR     R2, [R1, #0]       ; Guardar R0 en regs[0]
    
    ; Recuperar R1 original
    LDR     R2, [SP, #0]
    STR     R2, [R1, #4]       ; Guardar R1 en regs[1]
    
    ; Guardar del R2 al R12 directamente
    STR     R2, [R1, #8]       ; regs[2]
    STR     R3, [R1, #12]      ; regs[3]
    STR     R4, [R1, #16]      ; regs[4]
    STR     R5, [R1, #20]      ; regs[5]
    STR     R6, [R1, #24]      ; regs[6]
    STR     R7, [R1, #28]      ; regs[7]
    STR     R8, [R1, #32]      ; regs[8]
    STR     R9, [R1, #36]      ; regs[9]
    STR     R10, [R1, #40]     ; regs[10]
    STR     R11, [R1, #44]     ; regs[11]
    STR     R12, [R1, #48]     ; regs[12]
    
    ; Guardar SP, LR y PC
    MOV     R2, SP
    ADD     R2, R2, #8         ; Ajustar SP al valor previo al PUSH
    STR     R2, [R1, #52]      ; regs[13] = SP
    STR     LR, [R1, #56]      ; regs[14] = LR
    MOV     R2, PC
    STR     R2, [R1, #60]      ; regs[15] = PC

    POP     {R1, R2}           ; Restaurar registros comodín
    BX      LR
    ENDP

;-------------------------------------------------------------------------------
; void set_register_value(uint32_t *reg_array, uint32_t reg_num, uint32_t val);
; R0 = Dirección del arreglo
; R1 = Número de registro (0 a 12)
; R2 = Nuevo valor Hexadecimal
;-------------------------------------------------------------------------------
set_register_value PROC
    CMP     R1, #12            ; Validar que solo modifique R0-R12
    BHI     exit_set           ; Si es mayor a 12, salir

    LSL     R1, R1, #2         ; Multiplicar reg_num * 4 (offset en bytes)
    STR     R2, [R0, R1]       ; Escribir el nuevo valor en reg_array[reg_num]

exit_set
    BX      LR
    ENDP

    END