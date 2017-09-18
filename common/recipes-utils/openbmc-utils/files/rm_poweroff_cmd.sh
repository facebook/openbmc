#Remove poweroff command from .bash
echo "Remove poweroff command from .bash"
rm /sbin/poweroff
rm /sbin/poweroff.sysvinit

#Copy modified inittab to change "init 0" action from shutdown to reboot
cp /usr/local/bin/revise_inittab /etc/inittab

