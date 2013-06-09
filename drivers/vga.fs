\
\ Fcode payload for QEMU VGA graphics card
\
\ This is the Forth source for an Fcode payload to initialise
\ the QEMU VGA graphics card.
\

: qemu-vga-driver-init ( -- )
  qemu-video-addr to frame-buffer-adr
  default-font set-font
  qemu-video-width qemu-video-height over char-width / over char-height /
  fb8-install
;
