#!/bin/csh
echo Testing file $1

iis_dec $1
mv $1.dec ref

mp3decode $1
mv $1.raw slow

bcmp ref slow > $1.ref-slow.txt

mp3dec_opt $1
mv $1.raw fast

bcmp ref fast > $1.ref-fast.txt
bcmp slow fast > $1.slow-fast.txt

