#!/bin/sh
echo "abc123" | passwd root --stdin
systemctl stop linux2v
/opt/py35/bin/conda update conda -y
git init linux2v
cd linux2v
git remote add origin git@bitbucket.org:saasame/linux2v.git
git pull origin master
git pull
git reset --hard
git branch --set-upstream-to=origin/master master
git pull
mkdir tmp
