adb root
adb shell setenforce 0
adb shell "echo 0 > /proc/sys/kernel/kptr_restrict"
