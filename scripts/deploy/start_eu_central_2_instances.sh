#!/bin/bash

# List of instance IDs to start
INSTANCES=(
i-027ff7750d1a7e177
i-0af6119461e3ae0c9
i-0df8fe5a68f73a2c4
i-0e73401a0276fe1da
i-044cf75d8aafb18ea
i-0a2711f6c7a848801
)

x=$1
REGION="eu-central-2"

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