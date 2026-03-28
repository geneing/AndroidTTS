package com.AndroidTTS.engine

import android.content.Context
import android.content.res.AssetManager
import android.util.Log
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf

import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import com.StandaloneTTS.*

object TtsEngine {
    var tts: OfflineTts? = null

    // https://en.wikipedia.org/wiki/ISO_639-3
    // Example:
    // eng for English,
    // deu for German
    // cmn for Mandarin
    var lang: String? = null


    val speedState: MutableState<Float> = mutableFloatStateOf(1.0F)
    val speakerIdState: MutableState<String> = mutableStateOf("")

    var speed: Float
        get() = speedState.value
        set(value) {
            speedState.value = value
        }

    var speakerId: String
        get() = speakerIdState.value
        set(value) {
            speakerIdState.value = value
        }

    private var modelDir: String? = null
    private var ruleFsts: String? = null
    private var ruleFars: String? = null
    private var lexicon: String? = null
    private var dataDir: String? = null
    private var dictDir: String? = null
    private var assets: AssetManager? = null

    init {
        modelDir = "models"
        ruleFsts = null
        ruleFars = null
        lexicon = null
        dataDir = "models/espeak-ng-data"
        dictDir = null
        lang = "eng"
    }


    fun createTts(context: Context) {
        Log.i(TAG, "Init AndroidTTS")
        if (tts == null) {
            initTts(context)
        }
    }

    private fun initTts(context: Context) {

        if (dataDir != null) {
            val newDir = copyDataDir(context, modelDir!!)
            modelDir = "$newDir/$modelDir"
            dataDir = "$newDir/$dataDir"
            if (dictDir != null) {
                dictDir = "$modelDir/dict"
            }
            ruleFars = "$modelDir/en_tn_True_deterministic_cased__tokenize.far,$modelDir/en_tn_True_deterministic_verbalizer.far,$modelDir/en_tn_post_processing.far"
        }

        val config = getOfflineTtsConfig(
            modelDir = modelDir!!,
            lexicon = lexicon ?: "",
            dataDir = dataDir ?: "",
            dictDir = dictDir ?: "",
            ruleFsts = ruleFsts ?: "",
            ruleFars = ruleFars ?: ""
        )

        tts = OfflineTts(context = context, config = config)
    }


    private fun copyDataDir(context: Context, dataDir: String): String {
        Log.i(TAG, "data dir is $dataDir")
        copyAssets(context, dataDir)

        val newDataDir = context.filesDir.absolutePath
        Log.i(TAG, "newDataDir: $newDataDir")
        return newDataDir
    }

    private fun copyAssets(context: Context, path: String) {
        val assets: Array<String>?
        try {
            assets = context.assets.list(path)
            if (assets!!.isEmpty()) {
                copyFile(context, path)
            } else {
                val fullPath = "${context.filesDir}/$path"
                val dir = File(fullPath)
                dir.mkdirs()
                for (asset in assets.iterator()) {
                    val p: String = if (path == "") "" else "$path/"
                    copyAssets(context, p + asset)
                }
            }
        } catch (ex: IOException) {
            Log.e(TAG, "Failed to copy $path. $ex")
        }
    }

    private fun copyFile(context: Context, filename: String) {
        try {
            val istream = context.assets.open(filename)
            val newFilename = context.filesDir.toString() + "/" + filename
            val ostream = FileOutputStream(newFilename)
            // Log.i(TAG, "Copying $filename to $newFilename")
            val buffer = ByteArray(1024)
            var read = 0
            while (read != -1) {
                ostream.write(buffer, 0, read)
                read = istream.read(buffer)
            }
            istream.close()
            ostream.flush()
            ostream.close()
        } catch (ex: Exception) {
            Log.e(TAG, "Failed to copy $filename, $ex")
        }
    }
}
