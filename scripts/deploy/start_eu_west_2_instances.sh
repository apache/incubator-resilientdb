#!/bin/bash

# List of instance IDs to start
INSTANCES=(
i-06d348331145bf0c4
i-0dd6be1d568a5f530
i-04203b3961c260387
i-006a443286b92f405
i-0af6a05abf91b75a5
i-022102af35aaa2af2
i-0cb32f659078602a6
i-0dada6fe17612180b
i-063a16b449209b3da
i-0fe9c9b25e3d73156
i-0ebddb665111c9f51
i-0ff3d67c842995571
i-09154a0c7faaf358b
i-0c430492cfaccfe49
i-0c8d6a34a2e7a808f
i-032b7b3067d6925ce
i-0ae1aa24112d8943a
i-0def0806b8c64e5a0
i-0a041ec2be124bd05
i-087a09e22ed877a5b
i-00473f6e160d1401e
i-0d6a26559dcf28047
i-0073ad8885d943f81
i-0cceb13be2c44b2e0
i-0f079c972e80100a0
i-0612136db585edec6
i-08b70b321a8cdec05
i-078d9d9216756f652
i-0ae7500ef29ece4d1
i-08a46d00f4da4d763
i-03061fd5897a1a797
)

x=$1
REGION="eu-west-2"

if [ -z "$x" ]; then
    echo "Usage: $0 <number_of_instances_to_start>"
    exit 1
fi

# Take only the first x instances
SELECTED_INSTANCES=("${INSTANCES[@]:0:$x}")

echo "Starting first $x instances concurrently: ${SELECTED_INSTANCES[@]} in region $REGION"

for INSTANCE_ID in "${SELECTED_INSTANCES[@]}"; do
    (
        echo "→ Starting $INSTANCE_ID ..."
        aws ec2 start-instances --instance-ids "$INSTANCE_ID" --region "$REGION" >/dev/null
        echo "✓ $INSTANCE_ID start command sent."
    ) 
done

wait 

echo "All start commands issued"
echo "Waiting for instances to be running..."

# Take only the first x instances
SELECTED_INSTANCES=("${INSTANCES[@]:0:$x}")

# Wait until the first x instances are running
echo "Waiting for the first $x instances to be in 'running' state..."
aws ec2 wait instance-running --instance-ids "${SELECTED_INSTANCES[@]}" --region "$REGION"
echo "✓ First $x instances are now running."

# Show details
aws ec2 describe-instances \
    --instance-ids "${INSTANCES[@]}" \
    --region "$REGION" \
    --query "Reservations[*].Instances[*].[InstanceId,State.Name,PrivateIpAddress]" \
    --output table