package com.axon.kisa10.util

import android.content.Context
import android.content.SharedPreferences

object SharedPref {
    private var sharedPreferences: SharedPreferences? = null

    const val UPDATE_NOTIFICATION_FIRMWARE: String = "update_notification_firmware"
    const val AUTO_UPDATE_FIRMWARE: String = "auto_update_firmware"
    const val UPDATE_NOTIFICATION_DB: String = "update_notification_db"
    const val AUTO_UPDATE_DB: String = "auto_update_db"
    const val UPDATE_NOTIFICATION_BLE: String = "update_notification_ble"
    const val AUTO_UPDATE_BLE: String = "auto_update_ble"
    const val PREF_OVER_SPEED_ALERT: String = "pref_over_speed_alert"
    const val PREF_RAPID_ACCELERATION_ALERT: String = "pref_rapid_acceleration_alert"
    const val PREF_QUICK_BRAKING_ALERT: String = "pref_quick_braking_alert"
    const val PREF_SUDDEN_STOP_ALERT: String = "pref_sudden_stop_alert"
    const val PREF_LANE_CHANGE_ALERT: String = "pref_lane_change_alert"
    const val PREF_SHARP_TURN_ALERT: String = "pref_sharp_turn_alert"
    const val DEVICE_MAC_ADDRESS_PREFS: String = "device_mac_address_prefs"
    const val DEVICE_NAME_PREFS: String = "device_name_prefs"


    private fun openPref(context: Context) {
        sharedPreferences = context.getSharedPreferences("appname_prefs", Context.MODE_PRIVATE)
    }

    @JvmStatic
    fun getValue(context: Context, key: String?, defaultValue: String?): String? {
        var result: String? = ""
        try {
            openPref(context)
            result = sharedPreferences?.getString(key, defaultValue)
            sharedPreferences = null
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return result
    }

    fun getValue(context: Context, key: String?, defaultValue: Int): Int {
        var result = -1
        try {
            openPref(context)
            result = sharedPreferences!!.getInt(key, defaultValue)
            sharedPreferences = null
        } catch (e: Exception) {
        }
        return result
    }

    fun getValue(context: Context, key: String?, defaultValue: Long?): Long? {
        openPref(context)
        val result = sharedPreferences!!.getLong(key, defaultValue!!)
        sharedPreferences = null
        return result
    }

    @JvmStatic
    fun setValue(context: Context, key: String?, value: String?) {
        try {
            openPref(context)
            var prefsPrivateEditor = sharedPreferences!!.edit()
            prefsPrivateEditor!!.putString(key, value)
            prefsPrivateEditor.apply()
            prefsPrivateEditor = null
            sharedPreferences = null
        } catch (e: Exception) {
        }
    }

    fun setValue(context: Context, key: String?, value: Int) {
        try {
            openPref(context)
            var prefsPrivateEditor = sharedPreferences!!.edit()
            prefsPrivateEditor!!.putInt(key, value)
            prefsPrivateEditor.apply()
            prefsPrivateEditor = null
            sharedPreferences = null
        } catch (e: Exception) {
        }
    }

    fun setValue(context: Context, key: String?, value: Long?) {
        try {
            openPref(context)
            var prefsPrivateEditor = sharedPreferences!!.edit()
            prefsPrivateEditor!!.putLong(key, value!!)
            prefsPrivateEditor.apply()
            prefsPrivateEditor = null
            sharedPreferences = null
        } catch (e: Exception) {
        }
    } /*   public static ArrayList<String> getArraylistSelectedGroup(Context context, String key) {
        ArrayList<String> value = new ArrayList<>();
        try {
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
            if (preferences != null) {
                try {
                    value = (ArrayList<String>) ObjectSerializer.deserialize(preferences.getString(key, ObjectSerializer.serialize(new ArrayList<String>())));
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }catch (Exception e){}
        return value;
    }

    public static boolean setArraylistSelectedGroup(Context context, String key, ArrayList<String> value) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (preferences != null && !TextUtils.isEmpty(key)) {
            Editor editor = preferences.edit();
            try {
                editor.putString(key, ObjectSerializer.serialize(value));
            } catch (IOException e) {
                e.printStackTrace();
            }
            return editor.commit();
        }
        return false;
    }

    public static ArrayList<Integer> getArraylistSelectedStudent(Context context, String key) {
        ArrayList<Integer> value = new ArrayList<>();
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (preferences != null) {
            try {
                value = (ArrayList<Integer>) ObjectSerializer.deserialize(preferences.getString(key, ObjectSerializer.serialize(new ArrayList<Integer>())));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public static boolean setArraylistNotificationNum(Context context, String key, ArrayList<Integer> value) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (preferences != null && !TextUtils.isEmpty(key)) {
            Editor editor = preferences.edit();
            try {
                editor.putString(key, ObjectSerializer.serialize(value));
            } catch (IOException e) {
                e.printStackTrace();
            }
            return editor.commit();
        }
        return false;
    }

    public static HashMap<String, String> getHashMapFriends(Context context, String key) {
        HashMap<String, String> value = new HashMap<>();
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (preferences != null) {
            try {
                value = (HashMap<String, String>) ObjectSerializer.deserialize(preferences.getString(key, ObjectSerializer.serialize(new HashMap<String, String>())));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public static boolean setHashMapFriends(Context context, String key, HashMap<String,String> value) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        if (preferences != null && !TextUtils.isEmpty(key)) {
            Editor editor = preferences.edit();
            try {
                editor.putString(key, ObjectSerializer.serialize(value));
            } catch (IOException e) {
                e.printStackTrace();
            }
            return editor.commit();
        }
        return false;
    }*/
}
