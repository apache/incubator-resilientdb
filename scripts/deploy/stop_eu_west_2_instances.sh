#!/bin/bash
# Script to stop multiple EC2 instances

# List of EC2 Instance IDs to stop
INSTANCES=(
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
i-0bc32a6c92158be6d
i-0d25f2a2602ebc303
i-084843194861db9b5
i-0c2ce07ce91e11242
i-0593109f2715fba52
i-0b87e7c835bca8b1b
i-02770d4b0aabcffe0
i-07466df43733d754c
i-0fc6ec3d3d82fc05d
i-098cfba6d727ea8ea
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
