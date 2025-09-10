#!/bin/bash
# Script to stop multiple EC2 instances

# List of EC2 Instance IDs to stop
INSTANCES=(
i-0dbd43f12f568bf27
i-06752f9e1e8156a58
i-07c2749c1c1c0431b
i-04b687be029c81534
i-0e0d916f1d684c7e2
i-0eb023fc3917a7aa9
i-05897912ca05cc907
i-00cede28f4a6f7f71
)

# Set your AWS region
REGION="sa-east-1"

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
