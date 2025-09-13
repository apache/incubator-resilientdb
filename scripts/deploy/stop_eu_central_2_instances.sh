#!/bin/bash
# Script to stop multiple EC2 instances

# List of EC2 Instance IDs to stop
INSTANCES=(
i-027ff7750d1a7e177
i-0af6119461e3ae0c9
i-0df8fe5a68f73a2c4
i-0e73401a0276fe1da
i-044cf75d8aafb18ea
i-0a2711f6c7a848801
)

# Set your AWS region
REGION="eu-central-2"

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
