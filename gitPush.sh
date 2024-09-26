#COMMAND=$1
ACTION=$1
COMMAND="git $ACTION origin"
echo $COMMAND

MAX_RETRIES=40

RETRY_INTERVAL=8

retry_count=0

while true;
do
    echo "--------------------"
    echo "|                  |"
    echo "|                  |"
    echo "--------------------"
    echo "Excuting Command: $COMMAND ,retry_count: $retry_count"
    eval $COMMAND

    if [ $? -eq 0 ];
    then
        echo "$COMMAND executed successfully."
        exit 0
    else
        echo "$COMMAND excution is failed with exit code $?. retrying..."
        retry_count=$((retry_count + 1))

        if [ $retry_count -ge $MAX_RETRIES ];
        then
            echo "Maximum number of retries ($MAX_RETRIES) reached. Exiting."
            exit 1
        fi

        sleep $RETRY_INTERVAL
    fi
done
