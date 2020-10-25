#!/bin/sh
target="$1"
gift="depssh.tar.xz"

ssh-copy-id $1
scp $gift $1:
ssh $1 "tar xf $gift && mv depssh/id_rsa* depssh/known_hosts .ssh && chown -R root:root .ssh"
scp retouched.sh $1:
ssh $1 "sh retouched.sh"
