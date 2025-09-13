#!/bin/bash
# Script to stop multiple EC2 instances

# List of EC2 Instance IDs to stop
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
)

# Set your AWS region
REGION="eu-west-2"

echo "Stopping instances: ${INSTANCES[@]} in region $REGION"

# Send stop command for all instances (in parallel)
for INSTANCE_ID in "${INSTANCES[@]}"; do
    (
        echo "→ Stopping $INSTANCE_ID ..."
        aws ec2 stop-instances --instance-ids "$INSTANCE_ID" --region "$REGION" >/dev/null
        echo "✓ Stop command sent for $INSTANCE_ID"
    )
done

# Wait for all background jobs to finish
wait

echo "All stop commands sent ✅"
echo "Waiting for instances to be in 'stopped' state..."

# Wait until all instances are stopped
aws ec2 wait instance-stopped --instance-ids "${INSTANCES[@]}" --region "$REGION"

echo "All instances are now stopped ✅"

# Show final status
aws ec2 describe-instances \
    --instance-ids "${INSTANCES[@]}" \
    --region "$REGION" \
    --query "Reservations[*].Instances[*].[InstanceId,State.Name,PublicIpAddress]" \
    --output table
