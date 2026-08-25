%include "misc.inc"

%ifidn __OUTPUT_FORMAT__, win32
    %define Game_Enh2D_BeginFrame _Game_Enh2D_BeginFrame
    %define Game_Enh2D_BeginQuadrants _Game_Enh2D_BeginQuadrants
    %define Game_Enh2D_NextTile _Game_Enh2D_NextTile
    %define Game_Enh2D_Finish _Game_Enh2D_Finish
    %define Game_Enh2D_SelectDraw _Game_Enh2D_SelectDraw
%endif

extern Game_Enh2D_BeginFrame
extern Game_Enh2D_BeginQuadrants
extern Game_Enh2D_NextTile
extern Game_Enh2D_Finish
extern Game_Enh2D_SelectDraw

global Game_Enh2D_BeginFrame_proc
global _Game_Enh2D_BeginFrame_proc

global Game_Enh2D_BeginQuadrants_proc
global _Game_Enh2D_BeginQuadrants_proc

global Game_Enh2D_NextTile_proc
global _Game_Enh2D_NextTile_proc

global Game_Enh2D_Finish_proc
global _Game_Enh2D_Finish_proc

global Game_Enh2D_SelectDraw_proc
global _Game_Enh2D_SelectDraw_proc

%ifidn __OUTPUT_FORMAT__, elf32
section .note.GNU-stack noalloc noexec nowrite progbits
section .text progbits alloc exec nowrite align=16
%else
section .text code align=16
%endif


align 16
Game_Enh2D_BeginFrame_proc:
_Game_Enh2D_BeginFrame_proc:

; [esp      ] = return address (void proc, no arguments, no result)


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

        mov eax, esp
        sub esp, byte 4
        and esp, 0FFFFFFF0h
        mov [esp], eax

        call Game_Enh2D_BeginFrame

        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure Game_Enh2D_BeginFrame_proc


align 16
Game_Enh2D_BeginQuadrants_proc:
_Game_Enh2D_BeginQuadrants_proc:

; [esp      ] = return address (void proc, no arguments, no result)


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

        mov eax, esp
        sub esp, byte 4
        and esp, 0FFFFFFF0h
        mov [esp], eax

        call Game_Enh2D_BeginQuadrants

        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure Game_Enh2D_BeginQuadrants_proc


align 16
Game_Enh2D_NextTile_proc:
_Game_Enh2D_NextTile_proc:

; [esp      ] = return address (void proc, no arguments, no result)


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

        mov eax, esp
        sub esp, byte 4
        and esp, 0FFFFFFF0h
        mov [esp], eax

        call Game_Enh2D_NextTile

        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure Game_Enh2D_NextTile_proc


align 16
Game_Enh2D_Finish_proc:
_Game_Enh2D_Finish_proc:

; [esp      ] = return address (void proc, no arguments, no result)


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

        mov eax, esp
        sub esp, byte 4
        and esp, 0FFFFFFF0h
        mov [esp], eax

        call Game_Enh2D_Finish

        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure Game_Enh2D_Finish_proc


align 16
Game_Enh2D_SelectDraw_proc:
_Game_Enh2D_SelectDraw_proc:

; eax = X (in), edx = Y (in) - full replacement of Display_M2_selection (loc_13F1C),
; called from every real x86 call site by external_procedures.sci; no result
; [esp      ] = return address


        push ecx

; [esp + 1*4] = return address

        mov ecx, esp
        sub esp, byte 12
        and esp, 0FFFFFFF0h

        mov [esp], eax
        mov [esp + 1*4], edx

        mov [esp + 2*4], ecx

        call Game_Enh2D_SelectDraw

        mov esp, [esp + 2*4]

        pop ecx

        retn

; end procedure Game_Enh2D_SelectDraw_proc
