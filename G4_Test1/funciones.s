        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB

        EXPORT get_cpu_registers
        EXPORT set_cpu_register

;-------------------------------------------------------------------------------
; void get_cpu_registers(uint32_t *reg_array);
; Obtiene el estado actual de los registros
;-------------------------------------------------------------------------------
get_cpu_registers PROC
    PUSH    {R1, R2}
    MOV     R1, R0             ; R1 = Puntero al arreglo

    ; Leer R0 guardado en la pila
    LDR     R2, [SP, #4]
    STR     R2, [R1, #0]
    
    ; Leer R1 guardado en la pila
    LDR     R2, [SP, #0]
    STR     R2, [R1, #4]
    
    ; Guardar R2 a R12
    STR     R2, [R1, #8]
    STR     R3, [R1, #12]
    STR     R4, [R1, #16]
    STR     R5, [R1, #20]
    STR     R6, [R1, #24]
    STR     R7, [R1, #28]
    STR     R8, [R1, #32]
    STR     R9, [R1, #36]
    STR     R10, [R1, #40]
    STR     R11, [R1, #44]
    STR     R12, [R1, #48]
    
    ; SP, LR, PC
    MOV     R2, SP
    ADD     R2, R2, #8
    STR     R2, [R1, #52]
    STR     LR, [R1, #56]
    MOV     R2, PC
    STR     R2, [R1, #60]

    POP     {R1, R2}
    BX      LR
    ENDP

;-------------------------------------------------------------------------------
; void set_cpu_register(uint32_t *reg_array, uint32_t reg_num, uint32_t val);
; Actualiza la estructura de registros del monitor en ensamblador
;-------------------------------------------------------------------------------
set_cpu_register PROC
    CMP     R1, #12            ; Validar que solo altere R0-R12
    BHI     exit_set
    
    LSL     R1, R1, #2         ; R1 = reg_num * 4 (offset en bytes)
    STR     R2, [R0, R1]       ; reg_array[reg_num] = val

exit_set
    BX      LR
    ENDP

    END