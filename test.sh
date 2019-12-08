#!/bin/bash


rm -f test.txt
echo "X" > test.txt

OUT0=$(./hashcache -sc test.txt)
EXP0='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff n test.txt'

if [ "$OUT0" != "$EXP0" ] ; then

	echo test failed.
	exit 1
fi

OUT1=$(./hashcache -s test.txt)
EXP1='################################################################ - test.txt'

if [ "$OUT1" != "$EXP1" ] ; then

	echo test failed.
	exit 1
fi

OUT2=$(./hashcache -scu test.txt)
EXP2='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff n test.txt'

if [ "$OUT2" != "$EXP2" ] ; then

	echo test failed.
	exit 1
fi

OUT3=$(./hashcache -sc test.txt)
EXP3='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff c test.txt'

if [ "$OUT3" != "$EXP3" ] ; then

	echo test failed.
	exit 1
fi

OUT4=$(./hashcache -sr test.txt)
EXP4='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff v test.txt'

if [ "$OUT4" != "$EXP4" ] ; then

	echo test failed.
	exit 1
fi

#sleep 2
touch test.txt

OUT5=$(./hashcache -s test.txt)
EXP5='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff s test.txt'

if [ "$OUT5" != "$EXP5" ] ; then

	echo test failed.
	exit 1
fi

OUT6=$(./hashcache -sc test.txt)
EXP6='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff r test.txt'

if [ "$OUT6" != "$EXP6" ] ; then

	echo test failed.
	exit 1
fi

OUT7=$(./hashcache -scu test.txt)
EXP7='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff r test.txt'

if [ "$OUT7" != "$EXP7" ] ; then

	echo test failed.
	exit 1
fi

OUT8=$(./hashcache -sr test.txt)
EXP8='7058299627365fc7a3dd7840fd3d56f29306cd30c0f2c13cb500fe79617290ff v test.txt'

if [ "$OUT8" != "$EXP8" ] ; then

	echo test failed.
	exit 1
fi

#sleep 2
echo "Y" > test.txt

OUT9=$(./hashcache -sru test.txt)
EXP9='d08c5f95ebb8581ee4e5c0a2ee534d5a10d3c8e7f3a18d961adf902602bbd8a3 u test.txt'

if [ "$OUT9" != "$EXP9" ] ; then

	echo test failed.
	exit 1
fi

OUTa=$(./hashcache -sru test.txt)
EXPa='d08c5f95ebb8581ee4e5c0a2ee534d5a10d3c8e7f3a18d961adf902602bbd8a3 v test.txt'

if [ "$OUTa" != "$EXPa" ] ; then

	echo test failed.
	exit 1
fi

rm test.txt







