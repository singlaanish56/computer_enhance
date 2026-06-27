/* ========================================================================

   (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
   
   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.
   
   Please see https://computerenhance.com for more information
   
   ======================================================================== */

#include "sim86.h"

#include "sim86_memory.h"
#include "sim86_text.h"
#include "sim86_decode.h"
#include "sim86_siml.h"

#include "sim86_memory.cpp"
#include "sim86_text.cpp"
#include "sim86_decode.cpp"
#include "sim86_siml.cpp"

static void DisAsm8086(memory *Memory, u32 DisAsmByteCount, segmented_access DisAsmStart)
{
    segmented_access At = DisAsmStart;
    
    disasm_context Context = DefaultDisAsmContext();
    
    u32 Count = DisAsmByteCount;

    //array of size 8 registers with each register being a 2 bytes and assigned to zero
    u16* Registers_Storage = new u16[14]();
    instruction instruction_storage[Count];
    u32 instruction_index = 0;
   // printf("%u\n", Count);
    
    while(Count)
    {
        instruction Instruction = DecodeInstruction(&Context, Memory, &At);
        if(Instruction.Op)
        {
            if(Count >= Instruction.Size)
            {   
                instruction_storage[instruction_index] = Instruction;
                instruction_index+=Instruction.Size;
                Count -= Instruction.Size;
            }
            else
            {
                fprintf(stderr, "ERROR: Instruction extends outside disassembly region\n");
                break;
            }
            
            UpdateContext(&Context, Instruction);

        }
        else
        {
            fprintf(stderr, "ERROR: Unrecognized binary in instruction stream.\n");
            break;
        }
    }
    
   // printf("%zu\n", ArrayCount(instruction_storage));
    u32 instruction_start=0;
    while(instruction_start < ArrayCount(instruction_storage))
    {
        instruction Instruction = instruction_storage[instruction_start];
        if(IsPrintable(Instruction))
        {
            PrintInstruction(Instruction, stdout);
            SimulateInstruction(Instruction, Registers_Storage, instruction_start, stdout);
           // printf("instruction start %d\n", instruction_start);
            printf("\n");
        }
    }
    
}

int main(int ArgCount, char **Args)
{
    memory *Memory = (memory *)malloc(sizeof(memory));
    
    if(ArgCount > 1)
    {
        for(int ArgIndex = 1; ArgIndex < ArgCount; ++ArgIndex)
        {
            char *FileName = Args[ArgIndex];
            u32 BytesRead = LoadMemoryFromFile(FileName, Memory, 0);
            
            printf("; %s disassembly:\n", FileName);
            printf("bits 16\n");
            DisAsm8086(Memory, BytesRead, {});
        }
    }
    else
    {
        fprintf(stderr, "USAGE: %s [8086 machine code file] ...\n", Args[0]);
    }
    
    return 0;
}
