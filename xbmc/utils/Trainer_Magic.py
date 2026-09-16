import struct
import keystone

PLACEHOLDERS = {
    "TRAINER_BASE": 0xCA7F00D0,
    "DBGPRINT": 0xCA7F00D1,
    "FREE_FUNC": 0xCA7F00D2,
}

ASM_SRC = r"""
start:
    jmp loader
dbgprint_thunk:
    mov ebx, {DBGPRINT:#010x}
    jmp ebx
loader:
    pushad
    mov edx, {TRAINER_BASE:#010x}
    pushad
    push 1
    push 7
    call sm_bus
    push 0xc
    push 8
    call sm_bus
    popad
    mov esi, [0x10118]
    add esi, 8
    mov eax, [esi]
    mov esi, [edx+0x12]
    add esi, edx
    mov ecx, 3
loop_check:
    cmp eax, [esi]
    je found
    add esi, 4
    loop loop_check
    push 0xf0
    jmp tail
found:
    mov ebp, edx
    cmp dword ptr [edx+0x1a], 0
    je use_etm
    mov ecx, [edx+0x1a]
    jmp have_ecx
use_etm:
    mov ecx, [edx+0x16]
have_ecx:
    add ecx, edx

    push ecx # DbgPrint("Executing trainer 0x%p\n", ecx)
    push edx
    push ecx
    mov eax, msg_executing - end_loader
    add eax, edx
    push eax
    call dbgprint_thunk
    add esp, 8
    pop edx
    pop ecx

    mov eax, cr0
    push eax
    and eax, 0xfffeffff
    mov cr0, eax
    push edx
    call ecx
    pop edx
    pop eax
    mov cr0, eax

    push edx # DbgPrint("Executed trainer\n")
    mov eax, msg_executed - end_loader
    add eax, edx
    push eax
    call dbgprint_thunk
    add esp, 4
    pop edx

    sub edx, end_loader - start
    mov eax, {FREE_FUNC:#010x}
    test eax, eax
    je skip_free

    push eax
    push edx
    mov ecx, msg_freeing - start
    add ecx, edx
    push edx # DbgPrint("isPTM(), freeing heap 0x%p\n", heap_base)
    push ecx
    call dbgprint_thunk
    add esp, 8
    pop edx
    pop eax

    push eax
    push edx
    push 0xff
    push 8
    call sm_bus
    pop edx
    pop eax
    add esp, 32
    push edx
    push dword ptr [0x10128]
    jmp eax

skip_free:
    push 0xff
tail:
    push 8
    call sm_bus
    popad
    call [0x10128]
    ret
msg_executing:
    .string "Executing trainer 0x%p\n"
msg_executed:
    .string "Executed trainer\n"
msg_freeing:
    .string "isPTM(), freeing heap 0x%p\n"
sm_bus:
    push ebp
    mov ebp, esp
    mov dx, 0xc004
    mov al, 0x20
    out dx, al
    mov dx, 0xc008
    mov al, [ebp+8]
    out dx, al
    mov dx, 0xc006
    mov al, [ebp+0xc]
    out dx, al
    out dx, al
    mov dx, 0xc002
    mov al, 0x1a
    out dx, al
    push eax
    mov eax, 0xf4240
delay:
    dec eax
    jnz delay
    pop eax
    leave
    ret 8
end_loader:
""".format(**PLACEHOLDERS)


def assemble(asm_src):
    ks = keystone.Ks(keystone.KS_ARCH_X86, keystone.KS_MODE_32)
    try:
        encoding, _ = ks.asm(asm_src, 0)
    except keystone.KsError as e:
        raise SystemExit(f"assembly failed: {e}")
    if encoding is None:
        raise SystemExit("assembly failed")
    return bytes(encoding)


def find_offset(blob, value, name):
    pattern = struct.pack("<I", value)
    count = blob.count(pattern)
    if count != 1:
        raise SystemExit(f"expected exactly one {name} placeholder, found {count}")
    return blob.find(pattern)


def format_c_array(blob):
    lines = []
    num_line = 16
    for i in range(0, len(blob), num_line):
        chunk = blob[i:i + num_line]
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    return "\n".join(lines)


def main():
    blob = assemble(ASM_SRC)

    offsets = {name: find_offset(blob, value, name) for name, value in PLACEHOLDERS.items()}

    print(f"static unsigned char trainerloaderdata[{len(blob)}] =")
    print("{")
    print(format_c_array(blob))
    print("};")
    print()
    print(f"#define SIZEOFLOADERDATA {len(blob)}// loaderdata is our kernel hack to handle if trainer (com file) is executed for title about to run")
    print(f"#define TRAINER_BASE_OFFSET {hex(offsets['TRAINER_BASE'])}")
    print(f"#define DBGPRINT_FUNC_OFFSET {hex(offsets['DBGPRINT'])}")
    print(f"#define FREE_FUNC_OFFSET {hex(offsets['FREE_FUNC'])}")

if __name__ == "__main__":
    main()
