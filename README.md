# What is this?

This essentially implements the following CE script in C++:

```
define(address,"DarkSoulsII.exe"+126388)
define(bytes,89 46 14 F3 0F 11 46 2C)

[ENABLE]

assert(address,bytes)
alloc(newmem,$1000)
alloc(player,8)

label(code)
label(return)

registersymbol(player)

newmem:
  push rbx
  mov rbx,player
  mov [rbx],rcx
  pop rbx

code:
  mov [esi+14],eax
  movss [esi+2C],xmm0
  jmp return

address:
  jmp newmem
  nop 3
return:

[DISABLE]

address:
  db bytes
  // mov [esi+14],eax
  // movss [esi+2C],xmm0

dealloc(newmem)

{
// ORIGINAL CODE - INJECTION POINT: DarkSoulsII.exe+126388

DarkSoulsII.exe+12636D: 51                 - push ecx
DarkSoulsII.exe+12636E: 8B CF              - mov ecx,edi
DarkSoulsII.exe+126370: F3 0F 11 04 24     - movss [esp],xmm0
DarkSoulsII.exe+126375: FF D0              - call eax
DarkSoulsII.exe+126377: 8B 46 18           - mov eax,[esi+18]
DarkSoulsII.exe+12637A: 2B 06              - sub eax,[esi]
DarkSoulsII.exe+12637C: 39 46 14           - cmp [esi+14],eax
DarkSoulsII.exe+12637F: 7D 1B              - jnl DarkSoulsII.exe+12639C
DarkSoulsII.exe+126381: 80 7E 63 00        - cmp byte ptr [esi+63],00
DarkSoulsII.exe+126385: 0F 57 C0           - xorps xmm0,xmm0
// ---------- INJECTING HERE ----------
DarkSoulsII.exe+126388: 89 46 14           - mov [esi+14],eax
// ---------- DONE INJECTING  ----------
DarkSoulsII.exe+12638B: F3 0F 11 46 2C     - movss [esi+2C],xmm0
DarkSoulsII.exe+126390: 74 0A              - je DarkSoulsII.exe+12639C
DarkSoulsII.exe+126392: 8B 4D F0           - mov ecx,[ebp-10]
DarkSoulsII.exe+126395: 50                 - push eax
DarkSoulsII.exe+126396: 53                 - push ebx
DarkSoulsII.exe+126397: E8 94 FB FF FF     - call DarkSoulsII.exe+125F30
DarkSoulsII.exe+12639C: 8B 0D 14 04 C8 01  - mov ecx,[DarkSoulsII.exe+1150414]
DarkSoulsII.exe+1263A2: 8B 89 C4 0C 00 00  - mov ecx,[ecx+00000CC4]
DarkSoulsII.exe+1263A8: 6A 06              - push 06
DarkSoulsII.exe+1263AA: E8 21 68 3D 00     - call DarkSoulsII.exe+4FCBD0
}

```
