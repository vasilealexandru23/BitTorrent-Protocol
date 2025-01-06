#!/bin/bash

# Path to your Docker script
DOCKER_SCRIPT="/home/parallels/Desktop/APD-2024/teme/tema2/run_with_docker.sh"

# Variable to count occurrences of "30/40"
MATCH_COUNT=0

# Run the Docker script 100 times
for i in {1..100}; do
  echo "Running iteration $i..."
  
  # Run the Docker script and capture the output
  OUTPUT=$($DOCKER_SCRIPT)

  # Check if "30/40" is in the output
  if echo "$OUTPUT" | grep -q "30/40"; then
    echo "Found '30/40' in iteration $i"
    ((MATCH_COUNT++))
  fi
done

# Print the total number of matches
echo "Total occurrences of '30/40': $MATCH_COUNT"

