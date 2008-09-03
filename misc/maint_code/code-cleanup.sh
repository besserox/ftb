#! /bin/bash

indent_code()
{
    file=$1

    indent --k-and-r-style --line-length105 --else-endif-column1 --start-left-side-of-comments \
	--break-after-boolean-operator --dont-cuddle-else --dont-format-comments --comment-indentation1 \
	--indent-level4 \
	${file}
    rm -f ${file}~
    cp ${file} /tmp/${USER}.__tmp__ && \
	cat ${file} | sed -e 's/ *$//g' -e 's/( */(/g' -e 's/ *)/)/g' \
	-e 's/if(/if (/g' -e 's/while(/while (/g' -e 's/do{/do {/g' -e 's/}while/} while/g' > \
	/tmp/${USER}.__tmp__ && mv /tmp/${USER}.__tmp__ ${file}
}

usage()
{
    echo "Usage: $1 [filename | --all] {--recursive} {--debug}"
    echo "Note:This script has only been tested with indent version 2.2.9"
}

# Check usage
if [ -z "$1" ]; then
    usage $0
    exit
fi

# Make sure the parameters make sense
all=0
recursive=0
got_file=0
debug=
for arg in $@; do
    if [ "$arg" == "--all" ]; then
	all=1
    elif [ "$arg" == "--recursive" ]; then
	recursive=1
    elif [ "$arg" == "--debug" ]; then
	debug="echo"
    else
	got_file=1
    fi
done
if [ "$recursive" == "1" -a "$all" == "0" ]; then
    echo "--recursive cannot be used without --all"
    usage $0
    exit
fi
if [ "$got_file" == "1" -a "$all" == "1" ]; then
    echo "--all cannot be used in conjunction with a specific file"
    usage $0
    exit
fi

if [ "$recursive" == "1" ]; then
    for file in `find . -name '*.[ch]' | grep -v git | xargs file | grep -v directory | cut -f1 -d':'`; do
	${debug} indent_code ${file}
    done
elif [ "$all" == "1" ]; then
    for file in *.[ch]; do
	${debug} indent_code ${file}
    done
else
    ${debug} indent_code $@
fi
