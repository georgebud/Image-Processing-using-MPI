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
	REPS=$7

	DIFF=0
	echo "Testing filters on file: $INPUT_PATH/$FILE.$EXT"
	echo
	HEADER=""
	for i in $(seq 1 1 $NP) ; do
		HEADER="$HEADER       $i"
	done
	printf "%-20s $HEADER\n" "Running for NP = "

	for filter in "${FILTERS[@]}" ; do
		TIMES=""
		for np in $(seq 1 1 $NP) ; do
			TOTALTIME="0"
			for i in $(seq 1 1 $REPS) ; do
				RUNTIME=$( { time mpirun -np "$np" "$HOMEWORK" "${INPUT_PATH}/${FILE}.${EXT}" "${OUTPUT_PATH}/${FILE}-${filter}.${EXT}" "${filter}"; } 2>&1 )
				TOTALTIME=$(echo "$TOTALTIME + $RUNTIME" | bc)
			done

			if [ 1 -eq "$(echo "${TOTALTIME} < 1.0" | bc)" ] ; then
				TIMES="$TIMES    $TOTALTIME"
			elif [ 1 -eq "$(echo "${TOTALTIME} < 10.0" | bc)" ] ; then
				TIMES="$TIMES   $TOTALTIME"
			else
				TIMES="$TIMES  $TOTALTIME"
			fi
		done
		printf "%-20s $TIMES\n" "filter: $filter"
	done

	## Special test, multiple filters:
	TIMES=""
	for np in $(seq 1 1 $NP) ; do
		TOTALTIME="0"
		for i in $(seq 1 1 $REPS) ; do
			RUNTIME=$( { time mpirun -np "$np" "$HOMEWORK" "${INPUT_PATH}/${FILE}.${EXT}" "${OUTPUT_PATH}/${FILE}-bssembssem.${EXT}" "blur" "smooth" "sharpen" "emboss" "mean" "blur" "smooth" "sharpen" "emboss" "mean"; } 2>&1 )
			TOTALTIME=$(echo "$TOTALTIME + $RUNTIME" | bc)
		done

		if [ 1 -eq "$(echo "${TOTALTIME} < 1.0" | bc)" ] ; then
			TIMES="$TIMES    $TOTALTIME"
		elif [ 1 -eq "$(echo "${TOTALTIME} < 10.0" | bc)" ] ; then
			TIMES="$TIMES   $TOTALTIME"
		else
			TIMES="$TIMES  $TOTALTIME"
		fi
	done

	printf "%-20s $TIMES\n" "filters: bssembssem"
}


FILTERS=("smooth" "blur" "sharpen" "mean" "emboss")
EXTENSIONS=("pgm" "pnm")
INP='Colectie Poze'
OUT='out'
REF='ref'

TIMEFORMAT=%3R

if [ "$1" = "-h" ] ; then
	echo "./test-mpi-scal.sh [NP] [REPS] [FILE]"
	echo "NP = Numar procese"
	echo "Reps = nr de repetari a fiecarui test pentru un numar de procese"
	echo "FILE = Fisierul de testat (fara extensie); de exemplu \"macro\""
	echo "Recomandat de rulat fara parametrii :)"
	exit 0
fi


NP=$1

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

REPEAT=$2

if [ -z "$REPEAT" ] ; then
	echo "Running default; each test 10 times"
	REPEAT="10"
else
	echo "Running each test $REPEAT times"
fi

if ! [[ $REPEAT =~ ^-?[0-9]+$ ]] ; then
	echo "$REPEAT NOT AN INTEGER! STOPPING!"
	exit 1
fi

FILE=$3
if [ -z "$FILE" ] ; then
	echo "Running default; image \"macro\""
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

	test_filters "$FILE" "$INP_PATH" "$OUT_PATH" "$REF_PATH" "$e" "$NP" "$REPEAT"
done
