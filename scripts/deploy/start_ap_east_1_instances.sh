#!/bin/bash

# List of instance IDs to start
INSTANCES=(
i-0af61570b5bc95153
i-026a8fcc4490963f8
i-0c08fdc467b5d09ef
i-0b8fe42c692a4eb63
i-011ea895cb645bd05
i-08fe19d24e8388eb3
i-0eadb6af36d7d212f
i-02c4963b19ac09281
i-0e22684a1058bc373
i-0449a1d530b4f0622
i-024c29e0e61327f89
i-0fe849df117cd6758
i-059fb562f69d7623a
i-0b7bdaecb858fb79f
i-0fe479a7df90adf95
i-09194db7e4251a36a
)

x=$1
REGION="ap-east-1"

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