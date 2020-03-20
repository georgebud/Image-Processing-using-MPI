#!/bin/bash

HOMEWORK="tema3"

test_filters()
{
	FILE=$1
	INPUT_PATH=$2
	OUTPUT_PATH=$3
	REFERENCE_PATH=$4
	EXT=$5
	NP=$6

	DIFF=0
	echo "Testing filters on file: $INPUT_PATH/$FILE.$EXT"
	echo
	for filter in "${FILTERS[@]}" ; do
		OK=""
		for i in $(seq 1 1 $NP) ; do
			mpirun -np "$i" "$HOMEWORK" "${INPUT_PATH}/${FILE}.${EXT}" "${OUTPUT_PATH}/${FILE}-${filter}.${EXT}" "${filter}"
			diff "${REFERENCE_PATH}/${FILE}-${filter}.${EXT}" "${OUTPUT_PATH}/${FILE}-${filter}.${EXT}" &>/dev/null
			if [ $? -ne 0 ] ; then
				OK="1"
				printf "%-40s NOT OK for NP=$i...STOPPING this test\n" "filter: $filter"
				break
			fi
		done
		if [ -z $OK ] ; then
			printf "%-40s OK\n" "filter: $filter"
		fi
	done

	## Special test, multiple filters:
	OK=""
	for i in $(seq 1 1 $NP) ; do
		mpirun -np "$i" "$HOMEWORK" "${INPUT_PATH}/${FILE}.${EXT}" "${OUTPUT_PATH}/${FILE}-bssembssem.${EXT}" "blur" "smooth" "sharpen" "emboss" "mean" "blur" "smooth" "sharpen" "emboss" "mean"
		diff "${REFERENCE_PATH}/${FILE}-bssembssem.${EXT}" "${OUTPUT_PATH}/${FILE}-bssembssem.${EXT}" &>/dev/null
		if [ $? -ne 0 ] ; then
			OK="1"
			printf "%-40s NOT OK for NP=$i...STOPPING this test\n" "filters: bssembssem"
			break
		fi
	done
	if [ -z $OK ] ; then
		printf "%-40s OK\n" "filters: bssembssem"
	fi
}


FILTERS=("smooth" "blur" "sharpen" "mean" "emboss")
EXTENSIONS=("pgm" "pnm")
INP='Colectie Poze'
OUT='out'
REF='ref'

NP=$1

TIMEFORMAT=%3R

if [ -z "$NP" ] ; then
	echo "Running default; NP = 1 .. 8"
	NP=8
else
	echo "Running for NP = 1 .. $NP"
fi

if ! [[ $NP =~ ^-?[0-9]+$ ]] ; then
	echo "$NP NOT AN INTEGER! STOPPING!"
	exit 1
fi

FILE=$2
if [ -z "$FILE" ] ; then
	echo "Running default (image \"macro\")"
	FILE="macro"
else
	echo "Running on $FILE"
fi

echo "Running make..."
make
if [ $? -ne 0 ] ; then
	echo "Make ERRORS / WARNINGS! STOPPING!"
	exit 1
fi
echo "Running make... DONE"

rm -rf "$OUT"
mkdir "$OUT"

for e in "${EXTENSIONS[@]}" ; do
	ue="$(echo "$e" | tr a-z A-Z)"

	INP_PATH="$INP/$ue"
	OUT_PATH="$OUT/$e"
	REF_PATH="$REF/$e"

	echo
	echo "------------------------------TESTING $e------------------------------"
	mkdir "$OUT_PATH"
	if ! [ -f "$INP_PATH/$FILE.$e" ] ; then
		echo "$INP_PATH/$FILE.$e doesn't exist"
		continue;
	fi

	test_filters "$FILE" "$INP_PATH" "$OUT_PATH" "$REF_PATH" "$e" "$NP"
done
