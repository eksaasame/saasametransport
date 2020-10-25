Set PROJECTPATH=%~dp0.

"%PROJECTPATH%\qemu\qemu-system-x86_64.exe" -device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd -drive if=none,id=hd,file="%PROJECTPATH%\linux_launcher.img",index=0,media=disk,snapshot=on -net nic,macaddr=ba:be:00:fa:ce:01,model=virtio -net user,hostfwd=tcp::2500-:22 -boot c -m 256 -localtime -name linux_launcher