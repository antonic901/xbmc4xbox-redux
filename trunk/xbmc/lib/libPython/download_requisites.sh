#!/bin/sh
OPENSSL="openssl-0.9.8y"
BZIP2="bzip2-1.0.6"
SQLITE="sqlite-amalgamation"
SQLITEVER="3.6.21"

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

[ ! -f "$SQLITE-$SQLITEVER.tar.gz" ] && wget "http://sqlite.org/$SQLITE-$SQLITEVER.tar.gz"
rm -rf sqlite
tar xvzf $SQLITE-$SQLITEVER.tar.gz
mv sqlite-$SQLITEVER sqlite
cd sqlite && patch -p1 <../patches/sqlite-$SQLITEVER.diff & cd ..
