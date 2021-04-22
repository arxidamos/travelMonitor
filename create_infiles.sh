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

# echo $inputFile
# echo $input_dir
# echo $numFilesPerDirectory



# Create parent directory
# mkdir ./$input_dir

# Store all countries in an array
countries=()

# Check if country already stored
country_exists() {
    found=0
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

i=0
while read -r line
do
    # Country resides on the 4th column of each line
    x=$(echo $line | awk '{print $4}')
    # Make sure each country gets stored only once
    check=$(country_exists $x)

    if [[ $check -ne 1 ]]
    then
        countries[i]=$x
    fi
    ((i++))
done < $inputFile


for each in "${countries[@]}"
do
    echo $each
done






# # Toss a YES/NO coin
    # toss_coin() {
    #     local x=$(( $RANDOM%2 )) # x can be either 0 or 1
    #     if [ "$x" -eq 0 ]
    #     then
    #         echo "NO"
    #     else
    #         echo "YES"
    #     fi
    # }

    # # Produce unique random numbers within given limits
    # random_id() {
    #     num_array=( $( shuf -i 0-9999 -n "$numLines") )
    #     # If duplicates are allowed, up to 2% of IDs will be duplicates
    #     if [ $1 -eq 1 ]
    #     then
    #         local p=`bc <<< "$numLines*0.03/1"` # Floor to nearest integer
    #         while [ $p -gt 0 ]
    #         do
    #             # get random indices in num_array
    #             local k=$(( $RANDOM%$numLines ))
    #             local j=$(( $RANDOM%$numLines ))
    #             # make the one duplicate of the other
    #             num_array[$j]=${num_array[$k]}

    #             p=$(( p - 1 ))
    #         done
    #     fi
    # }

    # # Produce random date
    # random_date() {
    #     # let d=$date+%d
    #     local dd=$(( RANDOM%1+2020 ))
    #     local mm=$(( RANDOM%12+1 ))
    #     local yyyy=$(( RANDOM%28+1 ))
    #     d=`date -d "$dd-$mm-$yyyy" '+%d-%m-%Y'`
    #     echo "$d"
    # }

    # random_id $duplicatesAllowed
    # i=0
    # while [ $i -lt $numLines ]
    # do
    #     # Create random citizenID
    #     citizenID=${num_array[$i]}
    #     echo $citizenID | tr '\n' ' ' >> "citizenRecordsFile.txt"

    #     # Create random name
    #     name_length=$(( $RANDOM%13+3 ))
    #     # tr -dc: remove all chars from incoming command BUT the ones in the set
    #     # head -c : keep first <name_length> bytes (chars) from /urandom
    #     tr -dc 'a-zA-Z' </dev/urandom | head -z -c $name_length >> "citizenRecordsFile.txt"
    #     echo -n ' ' >> "citizenRecordsFile.txt"

    #     # Create random surname
    #     name_length=$(( $RANDOM%13+3 ))
    #     tr -dc 'a-zA-Z' </dev/urandom | head -z -c $name_length >> "citizenRecordsFile.txt"
    #     echo -n ' ' >> "citizenRecordsFile.txt"

    #     # Select random country
    #     shuf -n 1 $countriesFile | tr '\n' ' ' >> "citizenRecordsFile.txt" 

    #     # Create random age
    #     age=$(( $RANDOM%120+1 ))
    #     printf "%d " "$age" >> "citizenRecordsFile.txt"

    #     # Select random virus
    #     shuf -n 1 $virusesFile | tr '\n' ' '>> "citizenRecordsFile.txt"

    #     # Randomly choose YES/NO
    #     vaccined=$(toss_coin)
    #     printf "%s" "$vaccined" >> "citizenRecordsFile.txt"

    #     # Create random date, if YES was generated
    #     if [[ $vaccined == "YES" ]]
    #     then
    #         date=$(random_date)
    #         printf " %s\n" "$date" >> "citizenRecordsFile.txt"
    #     else
    #         printf "\n" >> "citizenRecordsFile.txt"
    #     fi

    #     i=$(( $i + 1 ))
    # done

    # # Remove last blank line
# truncate -s -1 citizenRecordsFile.txt