package com.skyautonet.drsafe.util.srt

import java.io.BufferedWriter
import java.io.File
import java.io.FileOutputStream
import java.io.OutputStreamWriter
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets
import java.util.regex.Pattern

/**
 * Created by Hussain on 26/07/24.
 */


object SRTUtils {
    private val DEFAULT_CHARSET: Charset = StandardCharsets.UTF_8

    private const val MILLIS_IN_SECOND: Long = 1000
    private const val MILLIS_IN_MINUTE = MILLIS_IN_SECOND * 60 // 60000
    private const val MILLIS_IN_HOUR = MILLIS_IN_MINUTE * 60 // 3600000

    private val PATTERN_TIME: Pattern =
        Pattern.compile("([\\d]{2}):([\\d]{2}):([\\d]{2}),([\\d]{3})")

    private const val PATTER_TIME_GROUP_HOURS = 1
    private const val PATTER_TIME_GROUP_MINUTES = 2
    private const val PATTER_TIME_GROUP_SECONDS = 3
    private const val PATTER_TIME_GROUP_MILLISECONDS = 4

    private const val SCAPE_TIME_TO_TIME = " --> "


    /**
     * This method is going to convert a String (time) input to milliseconds
     * Metodo responsavel por converter uma String com o formato de tempo HH:mm:ss,SSS em millis
     * @param time
     * @return texto convertido em millis
     * @throws Exception
     */
    @Throws(Exception::class)
    fun textTimeToMillis(time: String?): Long {
        if (time == null) throw NullPointerException("Time should not be null")

        val matcher = PATTERN_TIME.matcher(time)
        if (time.isEmpty() || !matcher.find()) throw Exception("incorrect time format...")

        var msTime: Long = 0
        val hours = matcher.group(PATTER_TIME_GROUP_HOURS).toShort()
        val min = matcher.group(PATTER_TIME_GROUP_MINUTES).toByte()
        val sec = matcher.group(PATTER_TIME_GROUP_SECONDS).toByte()
        val millis = matcher.group(PATTER_TIME_GROUP_MILLISECONDS).toShort()

        if (hours > 0) msTime += hours * MILLIS_IN_HOUR
        if (min > 0) msTime += min * MILLIS_IN_MINUTE
        if (sec > 0) msTime += sec * MILLIS_IN_SECOND

        return msTime + millis
    }

    /**
     *
     * Metodo responsavel por converter millis em texto formato HH:mm:ss,SSS
     * @param millisToText
     * @return
     */
    fun millisToText(millisToText: Long): String {
        val millisToSeconds = millisToText.toInt() / 1000
        var hours = (millisToSeconds / 3600).toLong()
        var minutes = ((millisToSeconds % 3600) / 60).toLong()
        var seconds = (millisToSeconds % 60).toLong()
        var millis = millisToText % 1000

        if (hours < 0) hours = 0
        if (minutes < 0) minutes = 0
        if (seconds < 0) seconds = 0
        if (millis < 0) millis = 0

        return String.format("%02d:%02d:%02d,%03d", hours, minutes, seconds, millis)
    }

    /**
     * Método responsavel por converter millisIn e millisOut em texto formato HH:mm:ss,SSS --> HH:mm:ss,SSS
     * @param millisIn
     * @param millisOut
     * @return
     */
    fun millisToText(millisIn: Long, millisOut: Long): String {
        return millisToText(millisIn) + SCAPE_TIME_TO_TIME + millisToText(millisOut)
    }

    /**
     * Metodo responsavel por buscar um Subtitle em uma lista a partir do tempo passado **timeMillis**
     * @param listSubtitles
     * @param timeMillis
     * @return um Subtitle ou null no caso de nada encontrado
     */
    fun findSubtitle(listSubtitles: ArrayList<Subtitle>?, timeMillis: Long): Subtitle? {
        if (listSubtitles == null || listSubtitles.isEmpty()) return null

        // most likely is first index
        if (timeMillis < 1000) return listSubtitles[0]

        for (i in listSubtitles.indices) {
            var sub = listSubtitles[i]
            if (inTime(sub, timeMillis)) return sub

            if (sub.nextSubtitle != null && sub.nextSubtitle!!.timeIn >= timeMillis) return sub.nextSubtitle
            else if (listSubtitles.size <= i + 1) // check if is the last element
                continue

            // get next element to test
            sub = listSubtitles[i + 1]
            if (sub.timeIn >= timeMillis) return sub
        }
        return null
    }

    /**
     * Metodo responsavel por buscar um Subtitle a partir de um [Subtitle], utilizando node<br></br>
     * Obs. Deve ser configurado no load do arquivo para utilizar Node #[SRTParser.getSubtitlesFromFile]
     * @param subtitle
     * @param timeMillis
     * @return
     */
    fun findSubtitle(subtitle: Subtitle?, timeMillis: Long): Subtitle? {
        if (subtitle == null) return null

        var subAux: Subtitle = subtitle
        while ((subAux.nextSubtitle.also { subAux = it!! }) != null) if (inTime(
                subAux,
                timeMillis
            )
        ) return subAux

        return null
    }

    /**
     * This method is going to check if a given subtitle is between the given timeMillis of your player
     * Method responsavel por testar se um subtititulo est� dentro do tempo buscado.
     * @param subtitle
     * @param timeMillis
     * @return
     */
    private fun inTime(subtitle: Subtitle, timeMillis: Long): Boolean {
        return timeMillis >= subtitle.timeIn && timeMillis <= subtitle.timeOut
    }

    /**
     * This method will sync your srt file adding or subtracting the value in @param timeInMillis
     * Método responsavel por realizar a sincronização do subtitulo e escrever um novo arquivo com o novo tempo sincronizado no arquivo
     * @param listSubtitles
     * @param timeInMillis
     * @return
     */
    fun speedSynchronization(
        listSubtitles: ArrayList<Subtitle>?,
        timeInMillis: Long,
        fileOut: File?
    ): Boolean {
        if (listSubtitles == null || listSubtitles.isEmpty() || timeInMillis == 0L || fileOut == null) return false

        try {
            FileOutputStream(fileOut).use { fos ->
                OutputStreamWriter(fos, DEFAULT_CHARSET).use { osw ->
                    BufferedWriter(osw).use { bos ->
                        for (subtitle in listSubtitles) {
                            bos.write(subtitle.id.toString())
                            bos.newLine()
                            bos.write(
                                millisToText(
                                    subtitle.timeIn + timeInMillis,
                                    subtitle.timeOut + timeInMillis
                                )
                            )
                            bos.newLine()
                            bos.write(subtitle.text)
                            bos.newLine()
                        }
                        bos.flush()
                        return true
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return false
    }
}