export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-android-
make hikey960_defconfig
make -j4 Image.gz
#make hisilicon/hi3660-hikey960.dtb
ret=`mkbootimg --kernel ./arch/arm64/boot/Image.gz --kernel_offset 0x00008000 --ramdisk ~/code/download/bootimage/out/boot.img-ramdisk.gz --ramdisk_offset 0x07b88000 --cmdline "androidboot.hardware=hikey960 console=ttyFIQ0 androidboot.console=ttyFIQ0 firmware_class.path=/system/etc/firmware loglevel=15 buildvariant=userdebug" --tags_offset 0x07988000 --base 0x00078000 -o boot.img`

echo "build result $ret"

fastboot flash boot boot.img reboot
