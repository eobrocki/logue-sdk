#!/bin/bash
#Recieve three params from input, the exact name of the Unit file, the module, and the slot # as optional param

CMD_EXEC(){
    output=$( ("$@") 2>&1 )
    ret=$?
    if [[ $ret -eq 0 ]]
    then
        echo  "Success: [ $* ]"
        return 0
    else
        echo  "Error:   [ $* ] returned $ret${output:+ ($output)}" 1>&2
        return $ret
    fi
}



LOGCLIDIR=/home/eobrocki/Documents/Nutekt_SDK/logue-sdk/tools/logue-cli/logue-cli-linux64-0.07-2b



#Store input params as variables
FILENAME=$1 #Exact Name of the Unit File
MODULE=$2   #Module (osc, delfx, modfx, revfx)
PKGTYPE=$3  #Package Type (mnlgxdunit, prlgunit, or ntkdigunit)
SLOT=$4     #Slot Number



#To make sure the midi port is clear
amidi -p hw:1 -S "00 00 00 00" 



if [[ $PKGTYPE =~ ^m.* ]]
    then

        PKGTYPE=mnlgxdunit

elif [[ $PKGTYPE =~ ^n.* ]]
    then

        PKGTYPE=ntkdigunit

elif [[ $PKGTYPE =~ ^p.* ]]
    then

        PKGTYPE=prlgunit

else

        echo "Not a valid Package Type"
        PKGTYPE=""

fi


echo "Package Type : $PKGTYPE"


if [ -n "$FILENAME" ] && [ -n "$MODULE" ] && [ -n "$PKGTYPE" ]
then

    #Store output of the CLI Command into Text File in order to parse out the Sysex Message generated
    if [ -z "$SLOT" ]
    then

        #If SLOT is empty use the first available slot
        $LOGCLIDIR/logue-cli load -i 1 -o 1 -m $MODULE -u $LOGCLIDIR/Unit_Files/$FILENAME.$PKGTYPE -d > $LOGCLIDIR/$FILENAME.txt

    else

        #Replace values in Command with variables
        $LOGCLIDIR/logue-cli load -i 1 -o 1 -m $MODULE -u $LOGCLIDIR/Unit_Files/$FILENAME.$PKGTYPE -s $SLOT -d > $LOGCLIDIR/$FILENAME.txt

    fi



    echo "Parsing Command Line Interface Output for Generated Sysex Message..."

    
    #Add if statement to determine the package type
    if [[ "$PKGTYPE" == "mnlgxdunit" ]] || [[ "$PKGTYPE" == "prlgunit" ]]
    then
 
        echo "Applying $PKGTYPE filter for command line"
        # SEARCH AFTER "Target platform: " then store in variable
        SYSEXMSG=$(grep -i -A 6 "Target platform: " $LOGCLIDIR/$FILENAME.txt)


    else
        
        echo "Applying $PKGTYPE filter for command line"
        # SEARCH AFTER "Target platform: " then store in variable
        SYSEXMSG=$(grep -i -A 3 "Target platform: " $LOGCLIDIR/$FILENAME.txt)

    fi



    echo "Cleaning Up Sysex Message of un-needed text..."
        
    # Replace everything before { and after } with nothing (inclusive)
    SYSEXMSG=$(echo $SYSEXMSG | sed 's/.*>>> {//' | sed 's/.}//' | sed 's/.<<< { f0, 42, 30, 0, 1, 57, 23, f7 }//')

    echo "Removing Commas from Sysex String..."
    
    #Remove all commas from the Sysex String
    SYSEXMSG=$(echo $SYSEXMSG | tr ',' ' ')
    


    if [[ $SYSEXMSG ]]
    then        
        
        echo "$SYSEXMSG" > $LOGCLIDIR/"$FILENAME.txt"

        echo "Sending Sysex message to NuTekt NTS-1"
        
        #Use Sed to grab cleaned up sysex msg and put in between the quotes of the amidi 
        #sed -i 's/F0.*F7/'"$SYSEXMSG"'/I' $LOGCLIDIR/SendSysex.sh
        #bash $LOGCLIDIR/SendSysex.sh

        CMD_EXEC amidi -p hw:1 -S "$SYSEXMSG"

    else
  
        echo "ERR: SYSEX Message Was Empty"

    fi    
else

    echo "*************************************"
    echo "ReadCLISenSysex:INVALID INPUT: Invalid Filename Module or PackageType"
    echo "*************************************"

fi
