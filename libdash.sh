#! /bin/sh
### Building-Script for third-party libraries

ROOT_UID="0"

#Check if run as root
if [ `whoami` != root ]; then
    echo "Please run this script as root or using sudo"
    exit
fi

echo "Building and installing third party libraries"

## Building LibDash

echo "Building LibDash.."

BOOST_DIR="./libdash"
if [ ! -d "$BOOST_DIR" ]; then
	echo "Fetching LibDash"
    git clone git://github.com/bitmovin/libdash.git
    cd libdash/libdash
	mkdir build
	cd build
	cmake ../
	make
  cd ../../..
  sudo cp ./libdash/libdash/build/bin/libdash.so /usr/local/lib/
	sudo mkdir /usr/local/include/libdash
  sudo cp -r ./libdash/libdash/libdash/include/* /usr/local/include/libdash/
	echo "Building LibDash finished."
else
	echo "LibDash already existing."
	echo "To build LibDash again please delete ./libdash"
	echo "Press [Enter] key to continue.."
	read dummy < /dev/tty
fi

## End LibDash

echo "Install-Script finished."
