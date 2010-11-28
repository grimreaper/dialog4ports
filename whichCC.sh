#!/bin/sh
if [ -x /usr/bin/clang -o -x /usr/local/bin/clang ]
then
	echo "clang";
else
if [ -x /usr/local/bin/gcc45 ]
then
	echo "gcc45";
fi
echo "cc";
fi
