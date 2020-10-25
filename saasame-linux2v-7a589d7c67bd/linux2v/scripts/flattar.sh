#!/bin/sh
outfile=$1; find $2 -type f | while read file; do
    tar rf $outfile -C $(dirname $file) $(basename $file)
done
