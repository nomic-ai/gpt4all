#!/bin/bash

# Display header
echo "=========================================================="
echo " ██████  ██████  ████████ ██   ██  █████  ██      ██      "
echo "██       ██   ██    ██    ██   ██ ██   ██ ██      ██      "
echo "██   ███ ██████     ██    ███████ ███████ ██      ██      "
echo "██    ██ ██         ██         ██ ██   ██ ██      ██      "
echo " ██████  ██         ██         ██ ██   ██ ███████ ███████ "
echo " └─> https://github.com/nomic-ai/gpt4all"

# Function to detect macOS architecture and set the binary filename
detect_mac_arch() {
  local mac_arch
  mac_arch=$(uname -m)
  case "$mac_arch" in
    arm64)
      os_type="M1 Mac/OSX"
      binary_filename="gpt4all-lora-quantized-OSX-m1"
      ;;
    x86_64)
      os_type="Intel Mac/OSX"
      binary_filename="gpt4all-lora-quantized-OSX-intel"
      ;;
    *)
      echo "Unknown macOS architecture"
      exit 1
      ;;
  esac
}

# Detect operating system and set the binary filename
case "$(uname -s)" in
  Darwin*)
    detect_mac_arch
    ;;
  Linux*)
    if grep -q Microsoft /proc/version; then
      os_type="Windows (WSL)"
      binary_filename="gpt4all-lora-quantized-win64.exe"
    else
      os_type="Linux"
      binary_filename="gpt4all-lora-quantized-linux-x86"
    fi
    ;;
  CYGWIN*|MINGW32*|MSYS*|MINGW*)
    os_type="Windows (Cygwin/MSYS/MINGW)"
    binary_filename="gpt4all-lora-quantized-win64.exe"
    ;;
  *)
    echo "Unknown operating system"
    exit 1
    ;;
esac
echo "================================"
echo "== You are using $os_type."


# Change to the chat directory
cd chat

# List .bin files and prompt user to select one
bin_files=(*.bin)
echo "== Available .bin files:"
for i in "${!bin_files[@]}"; do
  echo "   [$((i+1))] ${bin_files[i]}"
done

# Function to get user input and validate it
get_valid_user_input() {
  local input_valid=false

  while ! $input_valid; do
    echo "==> Please enter a number:"
    read -r user_selection
    if [[ $user_selection =~ ^[0-9]+$ ]] && (( user_selection >= 1 && user_selection <= ${#bin_files[@]} )); then
      input_valid=true
    else
      echo "Invalid input. Please enter a number between 1 and ${#bin_files[@]}."
    fi
  done
}

get_valid_user_input
selected_bin_file="${bin_files[$((user_selection-1))]}"

# Run the selected .bin file with the appropriate command
./"$binary_filename" -m "$selected_bin_file"
