#!/bin/bash

if type "python3" > /dev/null ; then
  echo "Run with Python 3:"
  python3 $1
else
  echo "Run with default python:"
  python $1
fi

