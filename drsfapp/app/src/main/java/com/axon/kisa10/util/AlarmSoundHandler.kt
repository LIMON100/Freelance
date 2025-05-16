package com.axon.kisa10.util

import android.content.Context
import android.media.AudioManager
import android.media.SoundPool
import com.kisa10.R

/**
 * Created by Hussain on 15/07/24.
 */
class AlarmSoundHandler(private val context : Context) {

    companion object {
        enum class SoundType {
            NONE, SPEED_LIMIT, CURSOR,
        }
    }

    private val speedLimit = SoundPool(1, AudioManager.STREAM_MUSIC, 0)
    private val cursor = SoundPool(1, AudioManager.STREAM_MUSIC, 0)

    private var speedLimitId = 0
    private var cursorId = 0

    fun loadSounds() {
        speedLimitId = speedLimit.load(context, R.raw.speedlimit, 1)
        cursorId = cursor.load(context, R.raw.cursor1, 1)
    }

    fun unLoadSounds() {
        cursor.release()
        speedLimit.release()
    }

    private fun playSpeedLimitSound() {
        speedLimit.play(speedLimitId, 1f, 1f, 0, 0, 1f)
    }

    private fun playCursorSound() {
        cursor.play(cursorId, 1f, 1f, 0, 0, 1f)
    }

    fun playSound(soundId : SoundType) = when(soundId) {
        SoundType.SPEED_LIMIT -> playSpeedLimitSound()
        SoundType.CURSOR -> playCursorSound()
        SoundType.NONE -> Unit
    }
}