package com.axon.kisa10.util

object CheckAutoUpdate {
    var bFirmwareUpdateAble: Boolean = false
    var bBleUpdateAble: Boolean = false
    var bDbUpdateAble: Boolean = false

    @JvmStatic
    fun isNewUpdate(serverVersion: String, deviceVersion: String): Boolean {
        val serverVersionByte = serverVersion.split(".")
        val deviceVersionByte = deviceVersion.split(".")
        if (serverVersionByte.size == 1 && deviceVersionByte.size == 1) {
            val serverInt1 = serverVersionByte[0].trim().toIntOrNull() ?: -1
            val deviceInt1 = deviceVersionByte[0].trim().toIntOrNull() ?: -1
            return serverInt1 > deviceInt1
        } else if (serverVersionByte.size == 2 && deviceVersionByte.size == 2) {
            val serverInt1 = serverVersionByte[0].trim().toIntOrNull() ?: -1
            val deviceInt1 = deviceVersionByte[0].trim().toIntOrNull() ?: -1
            val serverInt2 = serverVersionByte[1].trim().toIntOrNull() ?: -1
            val deviceInt2 = deviceVersionByte[1].trim().toIntOrNull() ?: -1
            return serverInt1 >= deviceInt1 && serverInt2 > deviceInt2
        } else if (serverVersionByte.size == 3 && deviceVersionByte.size == 3) {
            val serverInt1 = serverVersionByte[0].trim().toIntOrNull() ?: -1
            val deviceInt1 = deviceVersionByte[0].trim().toIntOrNull() ?: -1
            val serverInt2 = serverVersionByte[1].trim().toIntOrNull() ?: -1
            val deviceInt2 = deviceVersionByte[1].trim().toIntOrNull() ?: -1
            val serverInt3 = serverVersionByte[2].trim().toIntOrNull() ?: -1
            val deviceInt3 = deviceVersionByte[2].trim().toIntOrNull() ?: -1
            if (serverInt1 > deviceInt1) {
                return true
            } else if (serverInt1 == deviceInt1 && serverInt2 > deviceInt2) {
                return true
            } else if (serverInt1 == deviceInt1 && serverInt2 == deviceInt2 && serverInt3 > deviceInt3) {
                return true
            } else {
                return false
            }
        } else {
            try {
                val deviceVersion0 = deviceVersion.split(".")[0]
                val serverVersion0 = serverVersion.split(".")[0]
                return serverVersion0 > deviceVersion0
            } catch (e : Exception) {
                e.printStackTrace()
                return false
            }
        }
    }

    fun getUpdateAble(sType: String): Boolean {
        var bResult = false
        if (sType == AppConstants.COMMAND_FIRMWARE_READ) {
            bResult = bFirmwareUpdateAble
        } else if (sType == AppConstants.COMMAND_DATABASE_VERSION) {
            bResult = bDbUpdateAble
        } else if (sType == AppConstants.COMMAND_BLE_UPGRADE_READ) {
            bResult = bBleUpdateAble
        }

        return bResult
    }
}
