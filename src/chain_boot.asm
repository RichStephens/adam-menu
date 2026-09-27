    SECTION code_user

    PUBLIC  _chain_boot

; Enter a boot block already read to $C800 the way EOS does: B = device
_chain_boot:
    ld      b,l
    jp      $c800
