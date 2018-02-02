#!/bin/sh
ARC=$HOME/bin/arc
while getopts a: opt; do
    case $opt in
	a)
	    ARC=$OPTARG
    esac
done
shift $((OPTIND-1))
echo $ARC
cd $1
echo "Running tests in $1"
cat << EOT | $ARC
(load "../unit-test.arc")
(load "tests.arc")
(rat)
EOT
