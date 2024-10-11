; ` in chinese input is still `
`::SendInput {U+0060}

; add comment
; / in chinese input is still /
/::send,{U+002F}

; ctrl + / is `
^/::
    send,{U+0060}{U+0060}
    send, {Left}
Return               ; 结束此热键定义

; -> ```
!CapsLock::
    SendInput {U+0060}{U+0060}{U+0060} 
Return

; ctrl + {',', '.'} -> {'<', '>'}
^,::
    SendInput, {U+003C}
Return

; 中文输出情况下仍然输出[]
[::send,{U+005B}
]::send,{U+005D}


^]::  ; 这一行定义了当Ctrl和[键同时按下时的组合
    ;SendInput {U+0028}{U+0029}
    send,{U+0028}{U+0029}
    send, {Left}
Return               ; 结束此热键定义

; 这里可以进一步，进行括号匹配，如果前面有一个没有被匹配到的 < 符号，那么就不生成 <> ,而是应该只生成 >
^.::
    send, {U+003C}{U+003E}
    send, {Left}
Return

#Hotstring EndChars : U+0024
; Ctrl + 4 -> $
^4::
    Send {U+0024}
return
