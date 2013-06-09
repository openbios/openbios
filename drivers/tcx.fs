\
\ Fcode payload for QEMU TCX graphics card
\
\ This is the Forth source for an Fcode payload to initialise
\ the QEMU TCX graphics card.
\

: qemu-tcx-driver-init ( -- )
  qemu-video-addr to frame-buffer-adr
  default-font set-font
  qemu-video-width qemu-video-height over char-width / over char-height /
  fb8-install
;
