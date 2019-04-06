#!/bin/bash

inputFiles=($(find input/ -type f -printf "%f\n" | sort | tr '\n' ' '))

failures=0

if make
then
  mkdir -p output
  mkdir -p output/diffs
  mkdir -p output/out
  for file in "${inputFiles[@]}"
  do
    inputFile="./input/${file}"
    outputFile="./output/${file}"
    outputCompareFile="./output_compare/${file}"
    outFile="./output/out/${file}"
    diffFile="./output/diffs/${file}"
    { ./mr-wordc "${inputFile}" "${outputFile}" 32 ; } &> "${outFile}"
    if diff "${outputFile}" "${outputCompareFile}" > "${diffFile}"
    then
      echo "Test Case ${type} ${inputFile} passed"
    else
      echo "Test Case ${type} ${inputFile} failed"
      echo "  Incorrect output for ${outputFile}. Check diff file."
      let failures++
    fi
  done

  if [ "${failures}" -eq 0 ]
  then
    echo "All tests passed"
  else
    echo "Failures: ${failures}"
  fi

else
  echo "Make failed"
fi

