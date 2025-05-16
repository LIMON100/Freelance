package com.skyautonet.drsafe.util.srt

import java.io.BufferedReader
import java.io.FileInputStream
import java.io.InputStreamReader
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets
import java.util.regex.Pattern

/**
 * Created by Hussain on 26/07/24.
 */


object SRTParser {

    private val PATTERN_TIME: Pattern = Pattern.compile("([\\d]{2}:[\\d]{2}:[\\d]{2},[\\d]{3}).*([\\d]{2}:[\\d]{2}:[\\d]{2},[\\d]{3})")
    private val PATTERN_NUMBERS: Pattern = Pattern.compile("(\\d+)")
    private val DEFAULT_CHARSET: Charset = StandardCharsets.UTF_8

    private const val REGEX_REMOVE_TAGS = "<[^>]*>"

    private const val PATTERN_TIME_REGEX_GROUP_START_TIME = 1
    private const val PATTERN_TIME_REGEX_GROUP_END_TIME = 2

    /**
     *
     * This method is responsible for parsing a STR file.
     *
     * This method will not have any new line and also will not make the use of nodes see: Node [SRTParser.getSubtitlesFromFile]}
     *
     * Metodo responsavel por fazer parse de um arquivos de legenda. <br></br>
     * Obs. O texto nao vai conter quebra de linhas e nao é utilizado Node [SRTParser.getSubtitlesFromFile]}
     * @param path
     * @return
     */
    fun getSubtitlesFromFile(path: String?): ArrayList<Subtitle>? {
        return getSubtitlesFromFile(path, false, false)
    }

    /**
     *
     * This method is responsible for parsing a STR file.
     *
     * This method will not have any new line and also will not make the use of nodes see: Node [SRTParser.getSubtitlesFromFile]}
     *
     * Metodo responsavel por fazer parse de um arquivos de legenda. <br></br>
     * Obs. O texto nao vai conter quebra de linhas e nao é utilizado Node [SRTParser.getSubtitlesFromFile]}
     * @param path
     * @return
     */
    fun getSubtitlesFromFile(path: String?, keepNewlinesEscape: Boolean): ArrayList<Subtitle>? {
        return getSubtitlesFromFile(path, keepNewlinesEscape, false)
    }

    /**
     *
     * This method is responsible for parsing a STR file.
     *
     * This method will not have any new line and also will not make the use of nodes see: Node [SRTParser.getSubtitlesFromFile]}
     * Note that you can configure if you want to make the use of Nodes: by setting the parameter usingNodes to true
     *
     * Metodo responsavel por fazer parse de um arquivos de legenda. <br></br>
     *
     * @param path
     * @param keepNewlinesEscape
     * @param usingNodes
     * @return
     */
    fun getSubtitlesFromFile(
        path: String?,
        keepNewlinesEscape: Boolean,
        usingNodes: Boolean
    ): ArrayList<Subtitle>? {
        var subtitles: ArrayList<Subtitle>? = null
        var subtitle: Subtitle
        var srt: StringBuilder

        try {
            BufferedReader(
                InputStreamReader(
                    FileInputStream(path),
                    DEFAULT_CHARSET
                )
            ).use { bufferedReader ->
                subtitles = ArrayList()
                subtitle = Subtitle()
                srt = StringBuilder()
                while (bufferedReader.ready()) {
                    var line = bufferedReader.readLine()

                    var matcher = PATTERN_NUMBERS.matcher(line)

                    if (matcher.find()) {
                        subtitle.id = matcher.group(1)?.toInt() ?: -1 // index
                        line = bufferedReader.readLine()
                    }

                    matcher = PATTERN_TIME.matcher(line)

                    if (matcher.find()) {
                        subtitle.startTime =
                            matcher.group(PATTERN_TIME_REGEX_GROUP_START_TIME) // start time
                        subtitle.timeIn = SRTUtils.textTimeToMillis(subtitle.startTime)
                        subtitle.endTime =
                            matcher.group(PATTERN_TIME_REGEX_GROUP_END_TIME) // end time
                        subtitle.timeOut = SRTUtils.textTimeToMillis(subtitle.endTime)
                    }

                    var aux: String
                    while ((bufferedReader.readLine()
                            .also { aux = it }) != null && !aux.isEmpty()
                    ) {
                        srt.append(aux)
                        if (keepNewlinesEscape) srt.append("\n")
                        else {
                            if (!line!!.endsWith(" ")) // for any new lines '\n' removed from BufferedReader
                                srt.append(" ")
                        }
                    }

                    srt.delete(srt.length - 1, srt.length) // remove '\n' or space from end string

                    line = srt.toString()
                    srt.setLength(0) // Clear buffer

                    if (line.isNotEmpty()) {
                        line = line.replace(REGEX_REMOVE_TAGS.toRegex(), "")
                    }


                    subtitle.text = line
                    subtitles!!.add(subtitle)

                    if (usingNodes) {
                        subtitle.nextSubtitle = Subtitle()
                        subtitle = subtitle.nextSubtitle!!
                    } else {
                        subtitle = Subtitle()
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return subtitles
    }
}