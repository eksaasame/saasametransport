#!/bin/bash

export PATH="$PATH:/bin:/sbin"
export LANG=C.UTF-8

mount_points=($(cat /etc/mtab | grep "^/dev/" | awk -F ' ' '{print $2}'))

for mount_point in "${mount_points[@]}"; do
    excluded_file="${mount_point}/.excluded"
    if [ "$mount_point" = "/" ] ; then
   	    excluded_file="${mount_point}.excluded"
    fi
    if [ -f "$excluded_file" ] ; then
		echo "Found '${excluded_file}'"
        paths=($(sed '1s/^\xef\xbb\xbf//' "${excluded_file}" | grep "/"))
		for path in "${paths[@]}"; do
			full_path="${mount_point}/${path}"
			if [ "$mount_point" = "/" ] ; then
				full_path="${path}"
			fi
			echo "Looking for excluded path '${full_path}' type"
			if [ -d "$full_path" ] ; then
				echo "rm -rf '${full_path}'/*"
				`rm -rf "${full_path}"/*`
			elif [ -f "$full_path" ] ; then
				echo "rm -f '${full_path}'"
				`rm -f "${full_path}"`
			fi
		done
		`rm -f "${excluded_file}"`
    fi
done

