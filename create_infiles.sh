#!/bin/bash

# Check command line arguments
# Number of args has to be 3
if [ $# -ne 3 ]
then
    echo "You must enter exactly 3 parameters"
    exit 2
fi

# First arg must refer to existing file
if [[ ! -f $1 ]]
then
    echo "You must enter an existing file for parameter #1"
    exit 2
fi

# Second arg has to contain valid chars for directories
if [[ ! $2 =~ ^[0-9a-zA-Z_-]+$ ]]
then
    echo "You must enter a valid dir name for parameter #2"
    exit 2
fi

# Second arg can't be an existing file
if [[ -d $2 ]]
then
    echo "The dir name you typed for parameter #2 already exists. Try a different one."
    exit 2
fi

# Third arg has to be a contiguous string of digits
if [[ ! $3 =~ ^[0-9]+$ ]]
then
    echo "You must enter an integer for parameter #3"
    exit 2
fi

# Store arguments
inputFile=$1
input_dir=$2
numFilesPerDirectory=$(($3-1))
# Associative array to store countries and find which country's file (0, 1, ...) to write next
declare -A countries

# Check if country already stored in array
country_exists() {
    local found=0
    for each in "${!countries[@]}"
    do
        if [[ $1 == $each ]]
        then
            found=1
            break
        fi
    done
    echo $found
}

# Create initial dir
mkdir ./$input_dir

while read -r line
do
    # Get country from 4th column of each line
    country=$(echo $line | awk '{print $4}')
    # Make sure each country gets stored only once
    check=$(country_exists $country)
    if [[ $check -ne 1 ]]
    then
        # Initialize values of each country with '0' (as in: "country-0")
        countries[$country]=0

        # Move into initial dir
        cd $input_dir

        # Create dir for new country
        mkdir $country
        
        # Move into country's dir
        cd $country

        # Create numFilesPerDirectory files for country's dir
        for (( j=0; j<numFilesPerDirectory; j++ ))
        do
            touch $country-$j
        done

        # Move back to initial dir
        cd ../../
    fi

    # Move into initial dir
    cd $input_dir

    # Move into country's dir
    cd $country

    # Iterate through stored countries and their file numbers
    for item in "${!countries[@]}"
    do
        # Match current record's country
        if [[ $country == $item ]]
        then
            # Return next number to be used for file writing
            n=${countries[$item]}

            if [[ ${countries[$item]} -lt numFilesPerDirectory ]]
            then
                # Increment file number
                (( countries[$item]++ ))
            else
                # If limit surpassed, reset to 0
                countries[$item]=0
            fi
            break
        fi
    done

    # Write record to correct file
    echo $line >> $country-$n

    # Move back into initial dir
    cd ../../
done < $inputFile