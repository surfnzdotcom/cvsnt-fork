#!/bin/sh

if [ ! -n "$1" ]; then echo "rmla.sh PREFIX"; exit 1; fi
PREFIX=$1
echo "Remove the .la files from ${PREFIX} if they exist"
if [ ! -d ${PREFIX}/lib ]; then exit 0; fi
if [ -f ${PREFIX}/lib/libcvsapi.la ]; then rm -f ${PREFIX}/lib/libcvsapi.la; fi
if [ -f ${PREFIX}/lib/libcvstools.la ]; then rm -f ${PREFIX}/lib/libcvstools.la; fi
if [ -f ${PREFIX}/lib/libmdnsclient.la ]; then rm -f ${PREFIX}/lib/libmdnsclient.la; fi

if [ -f ${PREFIX}/lib/libcvsapi.la ]; then echo "libcvsapi.la not removed."; exit 1; fi
if [ -f ${PREFIX}/lib/libcvstools.la ]; then echo "libcvstools.la not removed"; exit 1; fi
if [ -f ${PREFIX}/lib/libmdnsclient.la ]; then echo "libmdnsclient.la not removed"; exit 1; fi

echo "Removed."
