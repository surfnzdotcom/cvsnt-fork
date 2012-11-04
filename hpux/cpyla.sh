#!/bin/sh

if [ ! -n "$1" ]; then echo "cpyla.sh PREFIX CVSNT|DESTDIR"; exit 1; fi
if [ ! -n "$2" ]; then echo "cpyla.sh PREFIX CVSNT|DESTDIR"; exit 1; fi
PREFIX=$1
CVSNT=$2
echo "Copying .la files from $CVSNT to $PREFIX"
if [ ! -d ${PREFIX}/lib ]; then mkdir ${PREFIX}/lib; fi
if [ -f ${CVSNT}/mdnsclient/.libs/libmdnsclient.la ]; then 
  cp ${CVSNT}/mdnsclient/.libs/libmdnsclient.la ${PREFIX}/lib
  cp ${CVSNT}/mdnsclient/.libs/libmdnsclient*.sl ${PREFIX}/lib
fi
if [ -f ${CVSNT}/${PREFIX}/lib/libmdnsclient.la ]; then 
  cp ${CVSNT}/${PREFIX}/lib/libmdnsclient.la ${PREFIX}/lib
  cp ${CVSNT}/${PREFIX}/lib/libmdnsclient*.sl ${PREFIX}/lib
fi
if [ -f ${CVSNT}/cvsapi/.libs/libcvsapi.la ]; then 
  cp ${CVSNT}/cvsapi/.libs/libcvsapi.la ${PREFIX}/lib
  cp ${CVSNT}/cvsapi/.libs/libcvsapi*.sl ${PREFIX}/lib
fi
if [ -f ${CVSNT}/${PREFIX}/lib/libcvsapi.la ]; then 
  cp ${CVSNT}/${PREFIX}/lib/libcvsapi.la ${PREFIX}/lib
  cp ${CVSNT}/${PREFIX}/lib/libcvsapi*.sl ${PREFIX}/lib
fi
if [ -f ${CVSNT}/cvstools/.libs/libcvstools.la ]; then 
  cp ${CVSNT}/cvstools/.libs/libcvstools.la ${PREFIX}/lib
  cp ${CVSNT}/cvstools/.libs/libcvstools*.sl ${PREFIX}/lib
fi
if [ -f ${CVSNT}/${PREFIX}/lib/libcvstools.la ]; then 
  cp ${CVSNT}/${PREFIX}/lib/libcvstools.la ${PREFIX}/lib
  cp ${CVSNT}/${PREFIX}/lib/libcvstools*.sl ${PREFIX}/lib
fi

