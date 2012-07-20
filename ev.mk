$(call inherit-product, device/asus/grouper/full_grouper.mk)

# Inherit some common evervolv stuff.
$(call inherit-product, vendor/ev/config/common_full_tablet_wifionly.mk)

#
# Setup device specific product configuration.
#
PRODUCT_NAME    := ev_grouper
PRODUCT_BRAND   := google
PRODUCT_DEVICE  := grouper
PRODUCT_MODEL   := Nexus 7
PRODUCT_MANUFACTURER := asus
PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=nakasi BUILD_FINGERPRINT=google/nakasi/grouper:4.1.1/JRO03D/402395:user/release-keys PRIVATE_BUILD_DESC="nakasi-user 4.1.1 JRO03D 402395 release-keys"

#
# Set up the product codename, build version & MOTD.
#
PRODUCT_CODENAME := Mirus
PRODUCT_VERSION_DEVICE_SPECIFIC := b1

PRODUCT_MOTD :="\n\n\n--------------------MESSAGE---------------------\nThank you for choosing Evervolv for your Google Nexus 7\nPlease visit us at \#evervolv on irc.freenode.net\nFollow @preludedrew for the latest Evervolv updates\nGet the latest rom at evervolv.com\n------------------------------------------------\n"

# Copy compatible bootanimation
# XXX: looks ok but is a teeny bit too small.
PRODUCT_COPY_FILES += \
    vendor/ev/prebuilt/xga/media/bootanimation.zip:system/media/bootanimation.zip

# Hot reboot
PRODUCT_PACKAGE_OVERLAYS += vendor/ev/overlay/hot_reboot
