#!/bin/sh

if [ "$(whoami)" != "root" ]; then
	echo "Run this as root! sudo should do the trick."
	exit
fi

if [ -f /etc/arch-release ]; then
	echo -e "\e[1;31mArch Linux detected!\e[0m"
	DEPS=`pacman -T apr apr-util base-devel boost boost-libs c-ares cmake curl db dbus-glib expat fmodex fontconfig freealut freetype2 gperftools glib2 gstreamer0.10 gtk2 hunspell libjpeg-turbo libogg libpng libvorbis openal openssl pcre qt qtwebkit sdl zlib | sed -e 's/\n/ /g'`
	if [ -z "${DEPS}" ]; then
		echo "Dependencies already installed."
	else
		pacman -S --asdeps ${DEPS}
	fi
else # else
	echo "Unsupported operating system."
fi
