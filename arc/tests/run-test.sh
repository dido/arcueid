#!/bin/sh
ARC=$HOME/bin/arc
cd $1
echo "Running tests in $1"
cat << EOT | $ARC
(load "../unit-test.arc")
(load "tests.arc")
(rat)
EOT
