//
// Created by ba on 1/26/22.
//

#include "android_version_utils.h"
#include <sys/system_properties.h>

int babyte::getSDKVersion() {
    char version[128] = "0";
    __system_property_get("ro.build.version.sdk", version);
    return atoi(version);
}