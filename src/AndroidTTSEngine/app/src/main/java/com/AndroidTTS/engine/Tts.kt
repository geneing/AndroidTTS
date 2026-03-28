// Copyright (c)  2023  Xiaomi Corporation
package com.StandaloneTTS

import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtLoggingLevel
import ai.onnxruntime.OrtSession
import ai.onnxruntime.OrtSession.SessionOptions

import android.content.Context
import android.content.res.AssetManager
import android.content.res.Resources
import android.util.Log
import android.util.LruCache
import com.AndroidTTS.engine.R
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.decodeFromStream
import java.io.File
import java.nio.FloatBuffer
import java.nio.LongBuffer
import java.nio.file.Files
import java.nio.file.Paths
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
    val sampleRate: Int = 24000,
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
    val context: Context,
    var config: OfflineTtsConfig,
) {
    private var voiceCache: LruCache<String, List<List<FloatArray>>>

    //    private var ptr: Long
//    private var token2id: Map<Char, Long>
    private lateinit var env : OrtEnvironment
    private lateinit var sessionOptions : OrtSession.SessionOptions
//    private var ortSession : OrtSession
    private lateinit var ortEncoderSession : OrtSession
    private lateinit var ortDecoderSession : OrtSession
    private lateinit var normalizer: Normalizer
    private lateinit var vocab : Map<Char, Int>

    init {
        voiceCache = LruCache<String, List<List<FloatArray>>>(4) // Max 4 items in the cache

        initEspeak(config.tokensFileName, config.dataDir)
        normalizer = Normalizer(config.ruleFars)

        env = OrtEnvironment.getEnvironment()
        sessionOptions = OrtSession.SessionOptions()
        sessionOptions.setLoggerId("TTSOnnx")
        sessionOptions.setSessionLogLevel(OrtLoggingLevel.ORT_LOGGING_LEVEL_VERBOSE)
        sessionOptions.addXnnpack( mapOf("intra_op_num_threads" to "6") );
        sessionOptions.setSessionLogLevel(OrtLoggingLevel.ORT_LOGGING_LEVEL_VERBOSE)
        sessionOptions.setOptimizationLevel(SessionOptions.OptLevel.ALL_OPT)

        val pathToModel = context.filesDir.path + "/models/kokoro-quant.onnx"
        ortEncoderSession = env.createSession(pathToModel, sessionOptions)
        Log.i("AndroidTTS", "Loaded: ${ortEncoderSession}")
    }

    @kotlinx.serialization.ExperimentalSerializationApi
    private fun getVoiceFromJson( name: String ): List<List<FloatArray>> {
        val voicesJsonInputStream = File(context.filesDir.path + "/models/voices.json").inputStream()
        val voicesMap = Json.decodeFromStream<Map<String, List<List<FloatArray>>>>(voicesJsonInputStream)
        val voiceVector = voicesMap[name]!!

        return voiceVector
    }

    @kotlinx.serialization.ExperimentalSerializationApi
    private fun getVoice(name: String): List<List<FloatArray>> {
        return voiceCache.get(name) ?: getVoiceFromJson(name).also { voiceCache.put(name, it) }
    }

    private fun readModel( path: String ): ByteArray {

//        val modelID = R.raw.encoder
//        return resources.openRawResource(modelID).readBytes()
        return Files.readAllBytes(Paths.get(path))
    }

    private fun readModel( resources:Resources, modelID: Int ): java.io.InputStream {
        return resources.openRawResource(modelID)
    }

    private fun readJson( resources: Resources, jsonID: Int ): String {
        return resources.openRawResource(jsonID).bufferedReader().use { it.readText() }
    }

    fun numSpeakers(): Int {
        return 1
    }
    fun sampleRate(): Int {
        return config.sampleRate
    }
    fun getTokenMap(tokenFile: String): Map<Char, Long> {
        val tokenMap = mutableMapOf<Char, Long>()

        //read from tokenFile entries with format "<char> <id>"
        File(tokenFile).forEachLine {
            try {
                tokenMap[it[0]] = it.split(" ")[1].toLong()
            } catch (e: Exception) {
                Log.e("AndroidTTS", "Error parsing token file: $e")
            }
        }
        return tokenMap
    }

    private fun encoder( tokenVector: LongArray, voice: List<List<FloatArray>>, speed: Float ): OrtSession.Result {
        val shape = longArrayOf( 1, tokenVector.size.toLong() )
        val currentStyleVector = voice[tokenVector.size][0]
        val inputTensor = OnnxTensor.createTensor(env, LongBuffer.wrap(tokenVector), shape)
        val styleTensor = OnnxTensor.createTensor(env, FloatBuffer.wrap(currentStyleVector), longArrayOf(1, currentStyleVector.size.toLong()))

        val speedTensor = OnnxTensor.createTensor(env, FloatBuffer.wrap(floatArrayOf(speed)), longArrayOf(1))

        val inputVector = mapOf( "tokens" to inputTensor, "style" to styleTensor, "speed" to speedTensor )
        val encoderOutput = ortEncoderSession.run(inputVector)
        return encoderOutput
    }
;
    @OptIn(ExperimentalTime::class)
    fun generate(
        text: String,
        sid: String = "af",
        speed: Float = 1.0f
    ): GeneratedAudio {

        Log.d("AndroidTTS", "text: $text")
        val (normText, normalizationTime) = measureTimedValue{ normalizeText(text) }
        Log.d("AndroidTTS", "normalizationTime: ${normalizationTime.inWholeMilliseconds}ms: $normText")
        val (tokenIds, tokenizationTime) = measureTimedValue { convertTextToTokenIds(normText, "en-us") }
        Log.d("AndroidTTS", "tokenizationTime: ${tokenizationTime.inWholeMilliseconds}ms: num tokens: ${tokenIds.size}")

        for( tokenVector in tokenIds ) {
            val (encoderOutput, encodingTime) = measureTimedValue { encoder(tokenVector = tokenVector, voice = getVoice(sid), speed=speed) }
            Log.d("AndroidTTS", "encodingTime: ${encodingTime.inWholeMilliseconds}ms: tokenVector size: ${tokenVector.size}")

//            val z : OnnxTensor = encoderOutput.get(0) as OnnxTensor
//            val y_mask: OnnxTensor = encoderOutput.get(1) as OnnxTensor
//            val dec_args: OnnxTensor = encoderOutput.get(2) as OnnxTensor
//
//            val n_frames = z.info.shape[2]
//            val range = OnnxTensor.createTensor(env, LongBuffer.wrap(longArrayOf(0, n_frames)), longArrayOf(2))
//
//            val inputVectorDecoder = mapOf( "z" to z, "y_mask" to y_mask, "range" to range, "g" to dec_args )
//
//            val (decoderOutput, decodingTime) = measureTimedValue { ortDecoderSession.run(inputVectorDecoder) }
//            val samples = ((decoderOutput?.get(0)?.value) as? Array<*>)!!.filterIsInstance<Array<FloatArray>>()
            val samples = ((encoderOutput.get(0)?.value) as? FloatArray) !!

            val generatedAudio = GeneratedAudio( samples = samples, sampleRate = config.sampleRate )
//            Log.d("AndroidTTS", "decodingTime: ${decodingTime.inWholeMilliseconds}ms, sound duration: ${1000.0f*generatedAudio.samples.size/(1.0f*generatedAudio.sampleRate)}")
            Log.d("AndroidTTS", "decodingTime: sound duration: ${1000.0f*generatedAudio.samples.size/(1.0f*generatedAudio.sampleRate)}")

            encoderOutput.close()
//            decoderOutput.close()

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
        sid: String = "af",
        speed: Float = 1.0f,
        callback: (samples: FloatArray) -> Unit
    ): Unit {
        val padding: Long = 10
        val chunk: Long = 100
        var start: Long = 0

        Log.d("AndroidTTS", "text: $text")
        val (normText, normalizationTime) = measureTimedValue{ normalizeText(text) }
        Log.d("AndroidTTS", "normalizationTime: ${normalizationTime.inWholeMilliseconds}ms: $normText")
        val (tokenIds, tokenizationTime) = measureTimedValue { convertTextToTokenIds(normText, "en-us") }
        Log.d("AndroidTTS", "tokenizationTime: ${tokenizationTime.inWholeMilliseconds}ms: num tokens: ${tokenIds.size}")

        for( tokenVector in tokenIds ) {
            val (encoderOutput, encodingTime) = measureTimedValue { encoder(tokenVector = tokenVector, voice = getVoice(sid), speed=speed) }
            Log.d("AndroidTTS", "encodingTime: ${encodingTime.inWholeMilliseconds}ms: tokenVector size: ${tokenVector.size}")

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
                    "AndroidTTS",
                    "decodingTime: ${decodingTime.inWholeMilliseconds}ms, PerFrameTime: ${(decodingTime.inWholeMilliseconds).toFloat()/numFramesThisChunk}ms, sound duration: ${1000.0f * samples.size / (1.0f * config.sampleRate)}"
                )

                val callbackTime = measureTimedValue { callback(samples) }
                Log.d("AndroidTTS", "callbackTime: ${callbackTime.duration.inWholeMilliseconds}ms")
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

    private external fun initEspeak( tokensPath: String, dataDir: String ): Unit
    private fun normalizeText(text: String): String {
        return normalizer.normalize(text)
    }

    private external fun convertTextToTokenIds(text: String, voice: String): List< LongArray >

    class Normalizer constructor( farList: String) {
        private var ptr: Long = 0
        init{
            ptr = initNormalizer(farList)
        }
        fun normalize(text: String): String {
            return normalizeImpl(ptr, text)
        }

        inner class C {
            protected fun finalize() {
                cleanupNormalizer(ptr)
            }
        }
        private external fun initNormalizer(farList: String): Long
        private external fun normalizeImpl(ptr: Long, text: String): String
        private external fun cleanupNormalizer(ptr: Long): Unit
    }

    companion object {
        init {
            System.loadLibrary("openfst_lib")
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

