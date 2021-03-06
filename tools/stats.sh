#! /bin/bash

#By default, stay in the current directory and used the installed eddic version
executable=${1:-"eddic"}
base_dir=${2:-"."}

count_ltac=0
count_mtac=0

#Get the directories
sample_dir="$base_dir/samples/*.eddi"
cases_dir="$base_dir/test/cases/*.eddi"

#This sample should be avoided for performance reasons
big_sample="$base_dir/samples/big.eddi"

#Count Statements in samples
for file in $sample_dir ; do
    if [ "$file" != "$big_sample" ] ; then
        count="`$executable --quiet --ltac-only $file | egrep -v NOP | sed '/^\s*$/d' | wc -l`"
        let "count_ltac += $count"
    
        count="`$executable --quiet --mtac-only $file | egrep -v NOP | sed '/^\s*$/d' | wc -l`"
        let "count_mtac += $count"
    fi
done

#Count Statements in test cases
for file in $cases_dir ; do
    count="`$executable --quiet --ltac-only $file | egrep -v NOP | sed '/^\s*$/d' | wc -l`"
    let "count_ltac += $count"
    
    count="`$executable --quiet --mtac-only $file | egrep -v NOP | sed '/^\s*$/d' | wc -l`"
    let "count_mtac += $count"
done

echo "Total MTAC Statements: $count_mtac"
echo "Total LTAC Statements: $count_ltac"
