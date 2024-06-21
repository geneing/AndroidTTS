// Copyright (c)  2023  Xiaomi Corporation
package com.StandaloneTTS

import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtLoggingLevel
import ai.onnxruntime.OrtSession
import ai.onnxruntime.providers.NNAPIFlags

import android.content.res.AssetManager
import android.content.res.Resources
import android.util.Log
import com.StandaloneTTS.onnx.R
import java.io.File
import java.nio.FloatBuffer
import java.nio.IntBuffer
import java.nio.LongBuffer
import java.nio.file.Files
import java.nio.file.Paths
import java.util.EnumSet
import kotlin.time.ExperimentalTime
import kotlin.time.measureTimedValue

data class OfflineTtsConfig(
    var numThreads: Int = 1,
    var debug: Boolean = false,
    var provider: String = "nnapi", //"cpu",
    var ruleFsts: String = "",
    var ruleFars: String = "",
    var maxNumSentences: Int = 2,
    var lexicon: String = "",
    var tokensFileName: String,
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
    private lateinit var env : OrtEnvironment
    private lateinit var sessionOptions : OrtSession.SessionOptions
//    private var ortSession : OrtSession
    private lateinit var ortEncoderSession : OrtSession
    private lateinit var ortDecoderSession : OrtSession

    init {

//        token2id = getTokenMap(config.model.vits.tokens)
        initEspeak(config.tokensFileName, config.dataDir)

        env = OrtEnvironment.getEnvironment()
        sessionOptions = OrtSession.SessionOptions()
        sessionOptions.setLoggerId("TTSOnnx")
//        sessionOptions.setSessionLogLevel(OrtLoggingLevel.ORT_LOGGING_LEVEL_VERBOSE)
//        sessionOptions.addNnapi() //EnumSet.of(NNAPIFlags.USE_FP16)
//        sessionOptions.addArmNN(false)
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
                Log.e("StandaloneTTS", "Error parsing token file: $e")
            }
        }
        return tokenMap
    }

    private fun encoder( tokenVector: LongArray, sid: Int ): OrtSession.Result {
        val shape = longArrayOf( 1, tokenVector.size.toLong() )
        val inputTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(tokenVector), shape)
        val inputLengths = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(tokenVector.size.toLong())), longArrayOf(1))
        val scales = OnnxTensor.createTensor(env, FloatBuffer.wrap(floatArrayOf(config.noiseScale, config.lengthScale, config.noiseScaleW)), longArrayOf(3))
        val sidTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(sid.toLong())), longArrayOf(1))

        val inputVector = mapOf( "input" to inputTensor, "input_lengths" to inputLengths, "scales" to scales, "sid" to sidTensor )
        val encoderOutput = ortEncoderSession.run(inputVector)
        return encoderOutput
    }
;
    @OptIn(ExperimentalTime::class)
    fun generate(
        text: String,
        sid: Int = 1,
        speed: Float = 1.0f
    ): GeneratedAudio {

        Log.d("StandaloneTTS", "text: $text")
        val (normText, normalizationTime) = measureTimedValue{ normalizeText(text) }
        Log.d("StandaloneTTS", "normalizationTime: ${normalizationTime.inWholeMilliseconds}ms: $normText")
        val (tokenIds, tokenizationTime) = measureTimedValue { convertTextToTokenIds(normText, "en-us") }
        Log.d("StandaloneTTS", "tokenizationTime: ${tokenizationTime.inWholeMilliseconds}ms: num tokens: ${tokenIds.size}")

        for( tokenVector in tokenIds ) {
            val (encoderOutput, encodingTime) = measureTimedValue { encoder(tokenVector, sid) }
            Log.d("StandaloneTTS", "encodingTime: ${encodingTime.inWholeMilliseconds}ms: tokenVector size: ${tokenVector.size}")

            val z : OnnxTensor = encoderOutput.get(0) as OnnxTensor
            val y_mask: OnnxTensor = encoderOutput.get(1) as OnnxTensor
            val dec_args: OnnxTensor = encoderOutput.get(2) as OnnxTensor

            val n_frames = z.info.shape[2]
            val range = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(0, n_frames)), longArrayOf(2))

            val inputVectorDecoder = mapOf( "z" to z, "y_mask" to y_mask, "range" to range, "g" to dec_args )

            val (decoderOutput, decodingTime) = measureTimedValue { ortDecoderSession.run(inputVectorDecoder) }
            val samples = ((decoderOutput?.get(0)?.value) as? Array<*>)!!.filterIsInstance<Array<FloatArray>>()
            val generatedAudio = GeneratedAudio( samples = samples[0][0], sampleRate = config.sampleRate )
            Log.d("StandaloneTTS", "decodingTime: ${decodingTime.inWholeMilliseconds}ms, sound duration: ${1000.0f*generatedAudio.samples.size/(1.0f*generatedAudio.sampleRate)}")

            encoderOutput.close()
            decoderOutput.close()

            return generatedAudio
        }
        return GeneratedAudio(
            samples = floatArrayOf(0.0f) as FloatArray,
            sampleRate = 1 as Int
        )
    }

    @OptIn(ExperimentalTime::class)
    fun generateWithCallback(
        text: String,
        sid: Int = 0,
        speed: Float = 1.0f,
        callback: (samples: FloatArray) -> Unit
    ): Unit {
        val padding: Long = 10
        val chunk: Long = 100
        var start: Long = 0

        Log.d("StandaloneTTS", "text: $text")
        val (normText, normalizationTime) = measureTimedValue{ normalizeText(text) }
        Log.d("StandaloneTTS", "normalizationTime: ${normalizationTime.inWholeMilliseconds}ms: $normText")
        val (tokenIds, tokenizationTime) = measureTimedValue { convertTextToTokenIds(normText, "en-us") }
        Log.d("StandaloneTTS", "tokenizationTime: ${tokenizationTime.inWholeMilliseconds}ms: num tokens: ${tokenIds.size}")

        for( tokenVector in tokenIds ) {
            val (encoderOutput, encodingTime) = measureTimedValue { encoder(tokenVector, sid) }
            Log.d("StandaloneTTS", "encodingTime: ${encodingTime.inWholeMilliseconds}ms: tokenVector size: ${tokenVector.size}")

            val z : OnnxTensor = encoderOutput.get(0) as OnnxTensor
            val y_mask: OnnxTensor = encoderOutput.get(1) as OnnxTensor
            val dec_args: OnnxTensor = encoderOutput.get(2) as OnnxTensor

            val n_frames = z.info.shape[2]

            while( start < n_frames ) {
                val firstFrame = maxOf(start-padding, 0)
                val endFrame = minOf( start + chunk, n_frames )
                val numFramesThisChunk = endFrame - firstFrame

                val range = OnnxTensor.createTensor(
                    env,
                    LongBuffer.wrap(longArrayOf(firstFrame, numFramesThisChunk)),
                    longArrayOf(2)
                )
                val inputVectorDecoder =
                    mapOf("z" to z, "y_mask" to y_mask, "range" to range, "g" to dec_args)

                val (decoderOutput, decodingTime) = measureTimedValue {
                    ortDecoderSession.run(
                        inputVectorDecoder
                    )
                }
                var samples =
                    ((decoderOutput?.get(0)?.value) as? Array<*>)!!.filterIsInstance<Array<FloatArray>>()[0][0]
                samples = samples.sliceArray(IntRange((256*padding).toInt(), samples.size-1))

                Log.d(
                    "StandaloneTTS",
                    "decodingTime: ${decodingTime.inWholeMilliseconds}ms, PerFrameTime: ${(decodingTime.inWholeMilliseconds).toFloat()/numFramesThisChunk}ms, sound duration: ${1000.0f * samples.size / (1.0f * config.sampleRate)}"
                )

                val callbackTime = measureTimedValue { callback(samples) }
                Log.d("StandaloneTTS", "callbackTime: ${callbackTime.duration.inWholeMilliseconds}ms")
                start = endFrame
                decoderOutput.close()
            }


            encoderOutput.close()
        }
    }

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
    private external fun initEspeak( tokensPath: String, dataDir: String ): Unit
    private fun normalizeText(text: String): String {
        return text
    }

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

//    companion object {
//        init {
//            System.loadLibrary("sherpa-onnx-jni")
//        }
//    }
    companion object {
        init {
            System.loadLibrary("espeak_lib")
            System.loadLibrary("espeak-ng")
            System.loadLibrary("ucd")
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
        tokensFileName = "$modelDir/tokens.txt",
        dataDir = dataDir,
        dictDir = dictDir,
    )
}

