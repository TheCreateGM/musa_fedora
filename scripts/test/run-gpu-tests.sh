#!/usr/bin/env bash
# Refuse to report a GPU test until the vendor driver and runtime are validated.
set -euo pipefail

echo "GPU_TEST_STATUS: NOT RUN"
echo "Reason: the MTT S30 has no validated driver/runtime on this host."
echo "The in-tree mtgpu module builds, but has not been installed, loaded, or tested."
echo "No device-code program has been executed on the GPU."
exit 3
