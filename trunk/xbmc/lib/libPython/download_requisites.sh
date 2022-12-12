#!/bin/sh
OPENSSL="openssl-1.0.2u"
BZIP2="bzip2-1.0.6"
SQLITE="sqlite-amalgamation"
SQLITEVER="3081101"

set -x

[ ! -f $OPENSSL.tar.gz ] && wget "http://www.openssl.org/source/$OPENSSL.tar.gz"
rm -rf openssl
tar xvzf $OPENSSL.tar.gz
mv $OPENSSL openssl
cd openssl && patch -p1 <../patches/$OPENSSL.diff && cd ..

[ ! -f $BZIP2.tar.gz ] && wget "http://www.bzip.org/1.0.6/$BZIP2.tar.gz"
rm -rf bzip2
tar xvzf $BZIP2.tar.gz
mv $BZIP2 bzip2

[ ! -f "$SQLITE-$SQLITEVER.zip" ] && wget "http://sqlite.org/$SQLITE-$SQLITEVER.zip"
rm -rf sqlite
unzip -o $SQLITE-$SQLITEVER.zip
mv $SQLITE-$SQLITEVER sqlite
cd sqlite && patch -p1 <../patches/sqlite-$SQLITEVER.diff && cd ..
