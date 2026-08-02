;;
;;  Copyright (C) 2016-2019 Roman Pauer
;;
;;  Permission is hereby granted, free of charge, to any person obtaining a copy of
;;  this software and associated documentation files (the "Software"), to deal in
;;  the Software without restriction, including without limitation the rights to
;;  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
;;  of the Software, and to permit persons to whom the Software is furnished to do
;;  so, subject to the following conditions:
;;
;;  The above copyright notice and this permission notice shall be included in all
;;  copies or substantial portions of the Software.
;;
;;  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;;  SOFTWARE.
;;

%include "misc.inc"

%ifidn __OUTPUT_FORMAT__, win32
    %define draw_3dscene _draw_3dscene
    %define Set_3DM_ViewDepth _Set_3DM_ViewDepth
    %define Set_3DM_ShadeTable _Set_3DM_ShadeTable
    %define Set_3DM_SquareSize _Set_3DM_SquareSize
    %define Init_3DM_SkyTable_Core _Init_3DM_SkyTable_Core
%endif

extern draw_3dscene

extern loc_8B6BB

extern Set_3DM_ViewDepth
extern Set_3DM_ShadeTable
extern Set_3DM_SquareSize
extern Init_3DM_SkyTable_Core

global draw_3dscene_proc
global _draw_3dscene_proc

global sub_8B6BB
global _sub_8B6BB

global Set_3DM_ViewDepth_proc
global _Set_3DM_ViewDepth_proc

global Set_3DM_ShadeTable_proc
global _Set_3DM_ShadeTable_proc

global Set_3DM_SquareSize_proc
global _Set_3DM_SquareSize_proc

global Init_3DM_SkyTable_Core_proc
global _Init_3DM_SkyTable_Core_proc

%ifidn __OUTPUT_FORMAT__, elf32
section .note.GNU-stack noalloc noexec nowrite progbits
section .text progbits alloc exec nowrite align=16
%else
section .text code align=16
%endif

align 16
draw_3dscene_proc:
_draw_3dscene_proc:

; [esp      ] = return address


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

    ; remember original esp value
        mov eax, esp
    ; reserve 4 bytes on stack
        sub esp, byte 4
    ; align stack to 16 bytes
        and esp, 0FFFFFFF0h
    ; save original esp value on stack
        mov [esp], eax

    ; stack is aligned to 16 bytes

        call draw_3dscene

    ; restore original esp value from stack
        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure draw_3dscene_proc


align 16
sub_8B6BB:
_sub_8B6BB:

; [esp +   4] = handle
; [esp      ] = return address


        push ebx
        push esi
        push edi
        push ebp

; [esp + 5*4] = handle
; [esp + 4*4] = return address

        mov eax, [esp + 5*4]

        call loc_8B6BB

        pop ebp
        pop edi
        pop esi
        pop ebx

        retn

; end procedure sub_8B6BB


align 16
Set_3DM_ViewDepth_proc:
_Set_3DM_ViewDepth_proc:

; eax = view_depth (in); eax = result (out)
; [esp      ] = return address


        push ecx
        push edx

; [esp + 2*4] = return address

    ; remember original esp value (right after the two pushes above)
        mov ecx, esp
    ; reserve 16 bytes on stack (1 argument slot + saved-esp slot, with room to spare)
        sub esp, byte 16
    ; align stack to 16 bytes
        and esp, 0FFFFFFF0h
    ; save original esp value on stack (offset 4, keeping offset 0 free for the argument)
        mov [esp + 4], ecx

    ; stack is aligned to 16 bytes; place the single cdecl argument
        mov [esp], eax

        call Set_3DM_ViewDepth

    ; restore original esp value from stack
        mov esp, [esp + 4]

        pop edx
        pop ecx

        retn

; end procedure Set_3DM_ViewDepth_proc


align 16
Set_3DM_ShadeTable_proc:
_Set_3DM_ShadeTable_proc:

; eax = shade_table (in); eax = result (out)
; [esp      ] = return address


        push ecx
        push edx

; [esp + 2*4] = return address

        mov ecx, esp
        sub esp, byte 16
        and esp, 0FFFFFFF0h
        mov [esp + 4], ecx

        mov [esp], eax

        call Set_3DM_ShadeTable

        mov esp, [esp + 4]

        pop edx
        pop ecx

        retn

; end procedure Set_3DM_ShadeTable_proc


align 16
Set_3DM_SquareSize_proc:
_Set_3DM_SquareSize_proc:

; eax = square_size_cm (in); eax = result (out)
; [esp      ] = return address


        push ecx
        push edx

; [esp + 2*4] = return address

        mov ecx, esp
        sub esp, byte 16
        and esp, 0FFFFFFF0h
        mov [esp + 4], ecx

        mov [esp], eax

        call Set_3DM_SquareSize

        mov esp, [esp + 4]

        pop edx
        pop ecx

        retn

; end procedure Set_3DM_SquareSize_proc


align 16
Init_3DM_SkyTable_Core_proc:
_Init_3DM_SkyTable_Core_proc:

; [esp      ] = return address (void proc, no arguments, no result)


        push eax
        push ecx
        push edx

; [esp + 3*4] = return address

        mov eax, esp
        sub esp, byte 4
        and esp, 0FFFFFFF0h
        mov [esp], eax

        call Init_3DM_SkyTable_Core

        mov esp, [esp]

        pop edx
        pop ecx
        pop eax

        retn

; end procedure Init_3DM_SkyTable_Core_proc
