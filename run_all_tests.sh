#!/bin/bash

# Set the path to the tests directory
tests_directory="tests"

# Check if the directory exists
if [ ! -d "$tests_directory" ]; then
  echo "Error: $tests_directory directory not found."
  exit 1
fi

# Find all .cc and .c files in the tests directory
test_files=$(find "$tests_directory" -type f \( -name "*.cc" -o -name "*.c" \))

# Check if there are any test files
if [ -z "$test_files" ]; then
  echo "No test files found in $tests_directory."
  exit 1
fi

# Iterate through each test file and run the test
for file in $test_files; do
  # Extract the name of the test file with the extension
  test_name=$(basename "$file")

  # Run the test using the ./run_test script
  echo "Running test: $test_name"
  ./run_test.sh "$test_name"

  # Check the exit status of the last command
  if [ $? -ne 0 ]; then
    echo "Error: Test $test_name failed."
    exit 1
  fi
done

echo "All tests passed successfully."
