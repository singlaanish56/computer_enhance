
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

static void SimulateInstruction(instruction Instruction, u16* Registers_Storage, FILE *Dest)
{
    auto dest  = Instruction.Operands[0].Register;
    for(u32 operandIndex=1; operandIndex<ArrayCount(Instruction.Operands); operandIndex++)
    {
        instruction_operand Operand = Instruction.Operands[operandIndex];
        
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
                case Operand_Immediate:{
                    value = Operand.ImmediateS32;
                    break;
                }
                default:{
                    break;
                }
            }
        }


        switch(Instruction.Op)
        {
            case Op_None:{
                break;
            }
            case Op_mov:{
                auto old_value = ReadRegister(dest, Registers_Storage);
                WriteRegister(dest,value,Registers_Storage);
                auto new_value =ReadRegister(dest, Registers_Storage);

                fprintf(Dest, " ; %s:0x%x->0x%x", GetRegName(dest),old_value, new_value);
        
                break;
            }
            case Op_add:{
                //auto current_flags = Registers_Storage[um["flags"].first] & um["flags"].second;
                
            }
            case Op_sub:{
                
            }
            case Op_cmp:{
                
            }
            default:
                break;
        }        
    }
}