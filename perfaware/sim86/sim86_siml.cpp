
#include <complex.h>
static u16 ReadRegister(register_access reg, u16* Registers_Storage)
{
    auto reg_name = GetRegName(reg);
    auto reg_info = um[reg_name];
    return (Registers_Storage[reg_info.Index] & reg_info.Mask) >> reg_info.Shift;
}

static void WriteRegister(register_access reg, u16 value, u16* Registers_Storage)
{
    auto dest_reg_name = GetRegName(reg);
    auto dest_reg_info = um[dest_reg_name];
    
    auto old_value = Registers_Storage[dest_reg_info.Index];
    auto clear_old_value = old_value & ~dest_reg_info.Mask; // make space for the current register(not mask)

    auto insert_new_value = (value << dest_reg_info.Shift) & dest_reg_info.Mask; // move the current value into the correct position
    u16 new_value = clear_old_value | insert_new_value;

    Registers_Storage[dest_reg_info.Index] = new_value;
}

static u16 GetMemoryIndex(effective_address_expression memory_address, u16* Registers_Storage){
    auto effective_address_string = GetEffectiveAddressExpression(memory_address);

    u16 index =0;
    for(register_access reg : um_memory[effective_address_string].ListOfRegisters)
    {
        index += ReadRegister(reg, Registers_Storage);
    }

    if(memory_address.Displacement != 0){
        index += memory_address.Displacement;
    }

    return index;
}

static void StoreMemory(u16 Index, u16 value, u8* Memory_Storage, u32 IsWord)
{

    Memory_Storage[Index] = value & 0x00FF;
    if (IsWord) {
        Memory_Storage[Index + 1] = (value >> 8) & 0x00FF;
    }
}

static u16 LoadMemory(u16 Index, u8* Memory_Storage, u32 IsWord )
{
    u16 ans = Memory_Storage[Index];
    if (IsWord) {
        u16 highbyte = Memory_Storage[Index + 1];
        ans = (highbyte << 8) | ans;
    }
    return ans;
}

u32 IsWord(instruction Instruction)
{
    u32 Inst_Flag = Instruction.Flags;
    return Inst_Flag & Inst_Wide;
}

static void SimulateInstruction(instruction Instruction, u16* Registers_Storage,u8* Memory_Storage, u32& instruction_start, FILE *Dest)
{
    auto dest  = Instruction.Operands[0].Register; 
    for(u32 operandIndex=1; operandIndex<ArrayCount(Instruction.Operands); operandIndex++)
    {
        instruction_operand Operand = Instruction.Operands[operandIndex];
        
        auto prev_ip = Registers_Storage[um["ip"].Index];
        auto current_ip = prev_ip + Instruction.Size;
        
        s32 value;
        if (Operand.Type != Operand_None){
            switch(Operand.Type)
            {
                case Operand_None:{
                    break;
                }
                case Operand_Register:{
                    value = ReadRegister(Operand.Register, Registers_Storage);
                    break;
                }
                case Operand_Memory:{
                    u16 index = GetMemoryIndex(Operand.Address, Registers_Storage);
                    value = LoadMemory(index, Memory_Storage, IsWord(Instruction) );

                    break;
                }
                case Operand_Immediate:{
                    value = Operand.ImmediateS32;
                    break;
                }
                default:{
                    break;
                }
            }
        }

        //!TODO: improve the add / sub /cmp
        switch(Instruction.Op)
        {
            case Op_None:{
                break;
            }
            case Op_mov:{

                if(Instruction.Operands[0].Type == Operand_Register)
                {
                    auto old_value = ReadRegister(dest, Registers_Storage); 
                    WriteRegister(dest,value,Registers_Storage); 
                    auto new_value = ReadRegister(dest, Registers_Storage);

                    fprintf(Dest, " ; %s:0x%x->0x%x", GetRegName(dest),old_value, new_value); 
                }else{
                    u16 index = GetMemoryIndex(Instruction.Operands[0].Address, Registers_Storage);
                    StoreMemory(index, value, Memory_Storage, IsWord(Instruction));
                }
                
                break;
            }
            case Op_add:{
                //auto current_flags = Registers_Storage[um["flags"].first] & um["flags"].second;

                auto current_value = ReadRegister(dest, Registers_Storage);
                s32 wide_value = current_value + value;
                auto new_value = wide_value;
                
                WriteRegister(dest, new_value, Registers_Storage);
                auto stored_result = ReadRegister(dest, Registers_Storage);
                
                fprintf(Dest, " ; %s:0x%x->0x%x", GetRegName(dest),current_value, stored_result);
                auto current_flags = Registers_Storage[um["flags"].Index] & um["flags"].Mask;

                //zero flag
                if((current_value + value)==0){

                    Registers_Storage[um["flags"].Index] |= (1 << 4);
                    fprintf(Dest, " ; flags:->Z");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 4);
                }

                //parity flag backward compatibility only checks the last8 bits of then number
                auto n = stored_result & 0x00FF;
                // the idea to caluclate the odd parity is to xor the two halves of the binaru number together to cancel out the even/ same bits
                n= n ^(n>>4);
                n= n ^(n>>2);
                n= n ^(n>>1);
                if(n&1){
                    Registers_Storage[um["flags"].Index] &= ~(1 << 2);
                
                }else{
                    Registers_Storage[um["flags"].Index] |= ( 1 << 2);
                    fprintf(Dest, " ; P");
                }

                //sign flag
                u32 Inst_Flag = Instruction.Flags;
                u32 w = Inst_Flag & Inst_Wide;

                if((w && (new_value & 0x8000)) || (!w && (new_value & 0x0080))){
                    Registers_Storage[um["flags"].Index] |=  (1 << 5);
                    fprintf(Dest, " ; S");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~( 1 << 5);
                }
          

                u16 a = (u16)current_value;
                u16 b = (u16)value;  
                u32 unsigned_sum = w ? ((u32)a + (u32)b) : (((u32)(a & 0xFF)) + ((u32)(b & 0xFF)));
                
                if((w && (unsigned_sum > 0xFFFF)) || (!w && (unsigned_sum > 0xFF))){
                    Registers_Storage[um["flags"].Index] |= (1 << 1);
                    fprintf(Dest, " ; C");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 1);
                }
                
                // overflow flag
                auto result_sign_flag = w ? ((stored_result >> 15) & 1) : ((stored_result >> 7) & 1);
                auto result_dest_flag = w ? ((value  >> 15) & 1) : ((value  >> 7) & 1);
                auto result_old_flag  = w ? ((current_value >> 15) & 1) : ((current_value >> 7) & 1);
                
                if((result_old_flag == result_dest_flag) && (result_sign_flag != result_old_flag)){
                    Registers_Storage[um["flags"].Index] |= (1 << 6);
                    fprintf(Dest, " ; O");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 6);
                }
                // auxillary flag
                if (((current_value & 0xF) + (value & 0xF)) > 0xF){
                    Registers_Storage[um["flags"].Index] |= (1 << 3);
                    fprintf(Dest, " ; A");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 3);
                }

                break;
            }
            case Op_sub:{
                auto current_value = ReadRegister(dest, Registers_Storage);
                s32 wide_value = current_value - value;
                auto new_value = wide_value;
                
                
                WriteRegister(dest, new_value, Registers_Storage);
                auto stored_result = ReadRegister(dest, Registers_Storage);
                fprintf(Dest, " ; %s:0x%x->0x%x", GetRegName(dest),current_value, stored_result);
                
                auto current_flags = Registers_Storage[um["flags"].Index] & um["flags"].Mask;

                //zero flag
                if(new_value==0){

                    Registers_Storage[um["flags"].Index] |= (1 << 4);
                    fprintf(Dest, " ; flags:->Z");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 4);
                }

                //parity flag backward compatibility only checks the last8 bits of then number
                auto n = stored_result & 0x00FF;
                // the idea to caluclate the odd parity is to xor the two halves of the binaru number together to cancel out the even/ same bits
                n= n ^(n>>4);
                n= n ^(n>>2);
                n= n ^(n>>1);
                if(n&1){
                    Registers_Storage[um["flags"].Index] &= ~(1 << 2);
                
                }else{
                    Registers_Storage[um["flags"].Index] |= ( 1 << 2);
                    fprintf(Dest, " ; P");
                }
                
                //sign flag
                u32 Inst_Flag = Instruction.Flags;
                u32 w = Inst_Flag & Inst_Wide;

                if((w && (new_value & 0x8000)) || (!w && (new_value & 0x0080))){
                    Registers_Storage[um["flags"].Index] |=  (1 << 5);
                    fprintf(Dest, " ; S");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~( 1 << 5);
                }

                //carry flag
                u16 a = (u16)current_value;
                u16 b = (u16)value;
                
                if((w && (a < b)) || (!w && ((a & 0xFF) < (b & 0xFF)))){
                    Registers_Storage[um["flags"].Index] |= (1 << 1);
                    fprintf(Dest, " ; C");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 1);
                }
                
                // overflow flag
                auto result_sign_flag = w ? ((stored_result >> 15) & 1) : ((stored_result >> 7) & 1);
                auto result_dest_flag = w ? ((value  >> 15) & 1) : ((value  >> 7) & 1);
                auto result_old_flag  = w ? ((current_value >> 15) & 1) : ((current_value >> 7) & 1);

                
                if((result_old_flag ^ result_dest_flag) && (result_sign_flag ^ result_old_flag)){
                    Registers_Storage[um["flags"].Index] |= (1 << 6);
                    fprintf(Dest, " ; O");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 6);
                }
                
                // auxillary flag
                if ((current_value & 0xF) < (value & 0xF)){
                    Registers_Storage[um["flags"].Index] |= (1 << 3);
                    fprintf(Dest, " ; A");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 3);
                }

                break;
                
            }
            case Op_cmp:{
                auto current_value = ReadRegister(dest, Registers_Storage);
                s32 wide_value = current_value - value;
                auto new_value = wide_value;
                u16 stored_result = (u16)wide_value;
                fprintf(Dest, " ; %s:0x%x->0x%x", GetRegName(dest),current_value, stored_result);
                
                auto current_flags = Registers_Storage[um["flags"].Index] & um["flags"].Mask;

                //zero flag
                if(new_value==0){

                    Registers_Storage[um["flags"].Index] |= (1 << 4);
                    fprintf(Dest, " ; flags:->Z");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 4);
                }

                //parity flag backward compatibility only checks the last8 bits of then number
                auto n = stored_result & 0x00FF;
                // the idea to caluclate the odd parity is to xor the two halves of the binaru number together to cancel out the even/ same bits
                n= n ^(n>>4);
                n= n ^(n>>2);
                n= n ^(n>>1);
                if(n&1){
                    Registers_Storage[um["flags"].Index] &= ~(1 << 2);
                
                }else{
                    Registers_Storage[um["flags"].Index] |= ( 1 << 2);
                    fprintf(Dest, " ; P");
                }
                
                //sign flag
                u32 Inst_Flag = Instruction.Flags;
                u32 w = Inst_Flag & Inst_Wide;

                if((w && (new_value & 0x8000)) || (!w && (new_value & 0x0080))){
                    Registers_Storage[um["flags"].Index] |=  (1 << 5);
                    fprintf(Dest, " ; S");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~( 1 << 5);
                }

                //carry flag
                u16 a = (u16)current_value;
                u16 b = (u16)value;
                
                if((w && (a < b)) || (!w && ((a & 0xFF) < (b & 0xFF)))){
                    Registers_Storage[um["flags"].Index] |= (1 << 1);
                    fprintf(Dest, " ; C");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 1);
                }
                
                // overflow flag
                auto result_sign_flag = w ? ((stored_result >> 15) & 1) : ((stored_result >> 7) & 1);
                auto result_dest_flag = w ? ((value  >> 15) & 1) : ((value  >> 7) & 1);
                auto result_old_flag  = w ? ((current_value >> 15) & 1) : ((current_value >> 7) & 1);

                
                if((result_old_flag ^ result_dest_flag) && (result_sign_flag ^ result_old_flag)){
                    Registers_Storage[um["flags"].Index] |= (1 << 6);
                    fprintf(Dest, " ; O");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 6);
                }
                
                // auxillary flag
                if ((current_value & 0xF) < (value & 0xF)){
                    Registers_Storage[um["flags"].Index] |= (1 << 3);
                    fprintf(Dest, " ; A");
                }else{
                    Registers_Storage[um["flags"].Index] &= ~(1 << 3);
                }

                break;
            }
            case Op_jne:{
                auto flags = Registers_Storage[um["flags"].Index] & um["flags"].Mask;
                if(!(flags & (1 << 4))){
                    s32 offset = Instruction.Operands[0].ImmediateS32;
                    current_ip = (u16)(prev_ip + offset);
                }
                break;
            }
            default:
                break;
        }
        
        instruction_start = current_ip;
        Registers_Storage[um["ip"].Index] =current_ip;
       fprintf(Dest, " ip:0x%x->0x%x", prev_ip, current_ip);  
       // print all the flags
       // print all the registers summary
       
    }
}