# Boot animation
TARGET_SCREEN_HEIGHT := 1080
TARGET_SCREEN_WIDTH := 1920

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

# Inherit device configuration
$(call inherit-product, device/asus/fugu/aosp_fugu.mk)

DEVICE_PACKAGE_OVERLAYS += device/asus/fugu/overlay-cm

## Device identifier. This must come after all inclusions
PRODUCT_NAME := cm_fugu
PRODUCT_BRAND := google
PRODUCT_MODEL := Nexus Player
PRODUCT_MANUFACTURER := Asus

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=fugu \
    BUILD_FINGERPRINT=google/fugu/fugu:5.1/LMY47D/1743759:user/release-keys \
    PRIVATE_BUILD_DESC="fugu-user 5.1 LMY47D 1743759 release-keys"
