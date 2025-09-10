#!/bin/bash
# Script to stop multiple EC2 instances

# List of EC2 Instance IDs to stop
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

# Set your AWS region
REGION="ap-east-1"

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
