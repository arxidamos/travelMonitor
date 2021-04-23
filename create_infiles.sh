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

# Store args
inputFile=$1
input_dir=$2
numFilesPerDirectory=$3
# Store all countries in an array
countries=()

# Check if country already stored in array
country_exists() {
    local found=0
    for each in "${countries[@]}"
    do
        if [[ $1 == $each ]]
        then
            found=1
            break
        fi
    done
    echo $found
}

# Fill country array
add_countries() {
    local i=0
    # Read from inputFile
    while read -r line
    do
        # Country resides on the 4th column of each line
        local x=$(echo $line | awk '{print $4}')
        # Make sure each country gets stored only once
        check=$(country_exists $x)
        if [[ $check -ne 1 ]]
        then
            countries[i]=$x
        fi
        ((i++))
    done < $inputFile
}

# Create directories and files
create_dirs() {
    # Create initial dir
    mkdir ./$input_dir
    
    # Move into initial dir
    cd $input_dir
    
    for country in "${countries[@]}"
    do
        # Create one dir per country
        mkdir $country
        # Move into country's dir
        cd $country
        # Create numFilesPerDirectory files for each country dir
        for (( i=0; i<numFilesPerDirectory; i++ ))
        do
            touch $country-$i
        done
        # Move back into initial dir
        cd ../
    done
}

# Main
add_countries
create_dirs
# TO-DO: round robin katanomi twn records se kathe arxeio, diavazontas apo ton input FILe