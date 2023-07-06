##!/bin/bash
#
# script to generate asz test file
#
# <path>
#
# Changed: <2023-07-06 09:00:04 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
printf "\t.data\n"
printf "dbuf:\tdef byte\t0\n"
printf "\t.text\n"
printf "_start:\tld de,dbuf0\n"
printf "\tld bc,_start\n"
printf "\n"
vars=$(seq -f "_var%04g" -s " " 0 20)
for i in $vars; do
	printf '\t.extern\t%s\n' $i
done
for i in $vars; do
	printf '\tld\thl,%s\n' $i
done
exit 0
