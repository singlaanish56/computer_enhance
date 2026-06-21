struct register_info
{
    int Index;
    u16 Mask;
    u8 Shift;
};

static std::unordered_map<std::string, register_info> um = {
    {"al", {0, 0x00FF, 0} }, {"ah", {0, 0xFF00, 8} }, {"ax", {0, 0xFFFF, 0} },
    {"bl", {1, 0x00FF, 0} }, {"bh", {1, 0xFF00, 8} }, {"bx", {1, 0xFFFF, 0} },
    {"cl", {2, 0x00FF, 0} }, {"ch", {2, 0xFF00, 8} }, {"cx", {2, 0xFFFF, 0} },
    {"dl", {3, 0x00FF, 0} }, {"dh", {3, 0xFF00, 8} }, {"dx", {3, 0xFFFF, 0} },
    {"sp", {4, 0xFFFF, 0} }, {"bp", {5, 0xFFFF, 0} }, {"si", {6, 0xFFFF, 0} }, 
    {"di", {7, 0xFFFF, 0} }, {"es", {8, 0xFFFF, 0} }, {"cs", {9, 0xFFFF, 0} },
    {"ss", {10, 0xFFFF, 0} }, {"ds", {11, 0xFFFF, 0} }, {"flags", {12, 0xFFFF, 0}}
};

static void SimulateInstruction(instruction Instruction, u16* Registers, FILE *Dest);
static u16 ReadRegister(register_access reg, u16* Registers_Storage);
static void WriteRegister(register_access reg, u16 value, u16* Registers_Storage);
