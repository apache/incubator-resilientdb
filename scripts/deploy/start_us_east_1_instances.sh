#!/bin/bash

# List of instance IDs to start
INSTANCES=(
i-042492e1a3720812e
i-0ff25e9ff3af0218d
i-08efffeae5073468d
i-0ecddf5c390ceb155
i-0cb76ab3684c96b16
i-062e6b70e1fd43ba3
i-00d09797e3144ffb8
i-06f4d03307bd84d77
i-0aa91720a4ecd07e9
i-0171f7596c8a3d87f
i-09bf4e5297eef4af6
i-0de0cd42d5e472750
i-07ab29e8ec0ea6da3
i-03ff9935cd4564645
i-0b126923b19a26f0d
i-0997be0bb1d3163ee
i-045f1ee6f6c9ba54a
i-0a13bea7dec3f56bc
i-086bb363d7e792816
i-04c9fb45939ce1d0d
i-09f4a852caadd9716
i-099fc0927bb3b1feb
i-028c9b23288087c4d
i-01083aee0a33b1947
i-097ac6b022087be42
i-0d4024141a850b367
i-0fc6ca6e64c61d25e
i-015633fdaf2d098b5
i-04cb35cdf208f69fb
i-0119059e4d86a5b01
i-0ca0e6cad4e8b2902
i-07ac437fb03baecff
i-0269ae176b69db0cf
i-079b32ab39b3224a8
i-09412c26986b20530
i-0e97fe438db4cfaf9
i-0074309614965a0b3
i-0388a5f80596fb136
i-0bc1e215b7ccbc471
i-0dcaedb9d8ee965be
i-0693604e78342b85f
i-00036b465f840f829
i-06aebaf1a820a9c5b
i-0e1bdee8f783d5cbe
i-0942d51399006fcde
i-0a2aefddf78a24f13
i-0704b2191e368fe27
i-05429513efd269d5f
i-01337920422d861b2
i-02444e46dbce3ef62
i-0845e57d4d2486cf6
i-0c41a39b1457a0f7e
i-05c879cc3a2e14dfd
i-097f7f1d1f73468dd
i-08efcd6bcfc7d3dd4
i-049e82c97afaf5f38
i-01c11d2afc298d072
i-064d9ea4196dca10e
i-0c0dee031bb803294
i-0f44b2b76e9fd7a3f
i-0e22920ca36647e4b
i-02cd1a32a0a277c84
i-0d2145658a236c17d
i-009b5c337601ca7d3
i-02af0e619467a5ca9
)

x=$1
x=$((x + 1))
REGION="us-east-1"

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