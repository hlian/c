CFLAGS="-g -std=c99 -Wall -pedantic"
echo "clang -c -o \$3 \$1.cpp $CFLAGS" >$3
chmod a+x $3
