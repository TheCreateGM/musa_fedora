# Standalone out-of-tree probe for the mtgpu XID record path.
#
# It only performs path lookups through the kernel's start_creating_path()
# API (the same API the vendor driver uses for its event-report directories)
# and prints the return value.  No file is created, modified or removed.
#
#   make
#   pkexec insmod ./xidprobe.ko
#   dmesg | grep XIDPROBE2
#   pkexec rmmod xidprobe
obj-m += xidprobe.o
