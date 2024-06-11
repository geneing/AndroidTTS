// Copyright (c)  2023  Xiaomi Corporation
package com.StandaloneTTS

import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtSession
import ai.onnxruntime.providers.NNAPIFlags

import android.content.res.AssetManager
import android.content.res.Resources
import android.util.Log
import com.StandaloneTTS.onnx.R
import java.io.File
import java.nio.FloatBuffer
import java.nio.LongBuffer
import java.nio.file.Files
import java.nio.file.Paths
import java.util.EnumSet

data class OfflineTtsConfig(
    var numThreads: Int = 1,
    var debug: Boolean = false,
    var provider: String = "nnapi", //"cpu",
    var ruleFsts: String = "",
    var ruleFars: String = "",
    var maxNumSentences: Int = 2,
    var lexicon: String = "",
    var tokens: String,
    var dataDir: String = "",
    var dictDir: String = "",
    var modelDir: String = "",
    var noiseScale: Float = 0.667f,
    var noiseScaleW: Float = 0.8f,
    var lengthScale: Float = 1.0f,
    val sampleRate: Int = 22050,
)

class GeneratedAudio(
    val samples: FloatArray,
    val sampleRate: Int,
) {
//    fun save(filename: String) =
//        saveImpl(filename = filename, samples = samples, sampleRate = sampleRate)
//
//    private external fun saveImpl(
//        filename: String,
//        samples: FloatArray,
//        sampleRate: Int
//    ): Boolean
}

class OfflineTts(
    assetManager: AssetManager? = null,
    resources: Resources,
    var config: OfflineTtsConfig,
) {
//    private var ptr: Long
//    private var token2id: Map<Char, Long>
    private val env : OrtEnvironment
    private val sessionOptions : OrtSession.SessionOptions
//    private var ortSession : OrtSession
    private var ortEncoderSession : OrtSession
    private var ortDecoderSession : OrtSession

    init {

//        token2id = getTokenMap(config.model.vits.tokens)

        env = OrtEnvironment.getEnvironment()
        sessionOptions = OrtSession.SessionOptions()
        sessionOptions.addNnapi(EnumSet.of(NNAPIFlags.USE_FP16))

//        val model = readModel(config.model.vits.model)
//        ortSession = env.createSession(model, sessionOptions)

        val encoder = readModel(resources, R.raw.encoder)
        ortEncoderSession = env.createSession(encoder, sessionOptions)

        val decoder = readModel(resources, R.raw.decoder)
        ortDecoderSession = env.createSession(decoder, sessionOptions)
    }

    private fun readModel( path: String ): ByteArray {

//        val modelID = R.raw.encoder
//        return resources.openRawResource(modelID).readBytes()
        return Files.readAllBytes(Paths.get(path))
    }

    private fun readModel( resources:Resources, modelID: Int ): ByteArray {
        return resources.openRawResource(modelID).readBytes()
    }

    fun getTokenMap(tokenFile: String): Map<Char, Long> {
        val tokenMap = mutableMapOf<Char, Long>()

        //read from tokenFile entries with format "<char> <id>"
        File(tokenFile).forEachLine {
            try {
                tokenMap[it[0]] = it.split(" ")[1].toLong()
            } catch (e: Exception) {
                Log.e("SherpaOnnx", "Error parsing token file: $e")
            }
        }
        return tokenMap
    }

//    fun sampleRate() = getSampleRate(ptr)
//
//    fun numSpeakers() = getNumSpeakers(ptr)

//    private fun generateAudio( tokenVector: LongArray, sid: Int ): FloatArray {
//        val shape = longArrayOf( 1, tokenVector.size.toLong() )
//        val inputTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(tokenVector), shape)
//        val inputLengths = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(tokenVector.size.toLong())), longArrayOf(1))
//        val scales = OnnxTensor.createTensor(env, FloatBuffer.wrap(floatArrayOf(config.model.vits.noiseScale, config.model.vits.lengthScale, config.model.vits.noiseScaleW)), longArrayOf(3))
//        val sidTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(sid.toLong())), longArrayOf(1))
//
//        val inputVector = mapOf( "input" to inputTensor, "input_lengths" to inputLengths, "scales" to scales, "sid" to sidTensor )
//        val outputTensor = ortSession.run(inputVector)
//        val samples : FloatArray
//
//        outputTensor.use {
//            @Suppress("UNCHECKED_CAST")
//            samples =
//                ((outputTensor?.get(0)?.value) as Array<Array<Array<FloatArray>>>)[0][0][0]
//        }
//        return samples
//    }

    private fun encoder( tokenVector: LongArray, sid: Int ): OnnxTensor {
        val shape = longArrayOf( 1, tokenVector.size.toLong() )
        val inputTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(tokenVector), shape)
        val inputLengths = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(tokenVector.size.toLong())), longArrayOf(1))
        val scales = OnnxTensor.createTensor(env, FloatBuffer.wrap(floatArrayOf(config.noiseScale, config.lengthScale, config.noiseScaleW)), longArrayOf(3))
        val sidTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(sid.toLong())), longArrayOf(1))

        val inputVector = mapOf( "input" to inputTensor, "input_lengths" to inputLengths, "scales" to scales, "sid" to sidTensor )
        val encoderOutput = ortEncoderSession.run(inputVector)
        val encoderOutputTensor : OnnxTensor

        encoderOutput.use {
            encoderOutputTensor = encoderOutput.get(0) as OnnxTensor
            val shape = encoderOutputTensor.info.shape

        }

        return encoderOutputTensor
    }
;
    fun generate(
        text: String,
        sid: Int = 0,
        speed: Float = 1.0f
    ): GeneratedAudio {
        Log.d("SherpaOnnx", "text: $text")
        val normText = normalizeText(text)
        Log.d("SherpaOnnx", "normText: $normText")
        val tokenIds = convertTextToTokenIds(normText, "en-us")

        for( tokenVector in tokenIds ) {
            Log.d("SherpaOnnx", "tokenVector size: $tokenVector.size")
            val vals = encoder(tokenVector, sid)

//            val samples = generateAudio( tokenVector, sid )
//            val generatedAudio = GeneratedAudio( samples = samples, sampleRate = sampleRate() )
//            return generatedAudio
        }
        return GeneratedAudio(
            samples = floatArrayOf(0.0f) as FloatArray,
            sampleRate = 1 as Int
        )
    }

    fun generateWithCallback(
        text: String,
        sid: Int = 0,
        speed: Float = 1.0f,
        callback: (samples: FloatArray) -> Unit
    ): GeneratedAudio {
        val objArray = generateWithCallbackImpl(
            text = text,
            sid = sid,
            speed = speed,
            callback = callback
        )
        return GeneratedAudio(
            samples = objArray[0] as FloatArray,
            sampleRate = objArray[1] as Int
        )
    }

//    fun generate(
//        text: String,
//        sid: Int = 0,
//        speed: Float = 1.0f
//    ): GeneratedAudio {
//        val objArray = generateImpl(ptr, text = text, sid = sid, speed = speed)
//        return GeneratedAudio(
//            samples = objArray[0] as FloatArray,
//            sampleRate = objArray[1] as Int
//        )
//    }
//
//    fun generateWithCallback(
//        text: String,
//        sid: Int = 0,
//        speed: Float = 1.0f,
//        callback: (samples: FloatArray) -> Unit
//    ): GeneratedAudio {
//        val objArray = generateWithCallbackImpl(
//            ptr,
//            text = text,
//            sid = sid,
//            speed = speed,
//            callback = callback
//        )
//        return GeneratedAudio(
//            samples = objArray[0] as FloatArray,
//            sampleRate = objArray[1] as Int
//        )
//    }

    fun allocate(assetManager: AssetManager? = null) {
    }

    fun free() {
    }

    protected fun finalize() {
    }

    fun release() = finalize()

//    private external fun newFromAsset(
//        assetManager: AssetManager,
//        config: OfflineTtsConfig,
//    ): Long
//
//    private external fun newFromFile(
//        config: OfflineTtsConfig,
//    ): Long
//
//    private external fun delete(ptr: Long)
//    private external fun getSampleRate(ptr: Long): Int
//    private external fun getNumSpeakers(ptr: Long): Int
//
    private external fun normalizeText(text: String): String
    private external fun convertTextToTokenIds(text: String, voice: String): List< LongArray >

    // The returned array has two entries:
    //  - the first entry is an 1-D float array containing audio samples.
    //    Each sample is normalized to the range [-1, 1]
    //  - the second entry is the sample rate
//    private external fun generateImpl(
//        ptr: Long,
//        text: String,
//        sid: Int = 0,
//        speed: Float = 1.0f
//    ): Array<Any>

    private external fun generateWithCallbackImpl(
        text: String,
        sid: Int = 0,
        speed: Float = 1.0f,
        callback: (samples: FloatArray) -> Unit
    ): Array<Any>

//    companion object {
//        init {
//            System.loadLibrary("sherpa-onnx-jni")
//        }
//    }
    companion object {
        init {
            System.loadLibrary("espeak-ng")
        }
    }
}

fun getOfflineTtsConfig(
    modelDir: String,
    lexicon: String,
    dataDir: String,
    dictDir: String,
    ruleFsts: String,
    ruleFars: String
): OfflineTtsConfig {
    return OfflineTtsConfig(
        numThreads = 4,
        debug = false,
        provider = "nnapi", //"cpu",
        ruleFsts = ruleFsts,
        ruleFars = ruleFars,
        maxNumSentences = 2,
        lexicon = lexicon,
        tokens = "$modelDir/tokens.txt",
        dataDir = dataDir,
        dictDir = dictDir,
    )
}

