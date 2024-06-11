package com.StandaloneTTS

import android.content.res.AssetManager
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.media.MediaPlayer
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.StandaloneTTS.onnx.R
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

const val TAG = "AndroidTTS"



class MainActivity : AppCompatActivity() {
    private lateinit var tts: OfflineTts
    private lateinit var text: EditText
    private lateinit var sid: EditText
    private lateinit var speed: EditText
    private lateinit var generate: Button
    private lateinit var play: Button

    // see
    // https://developer.android.com/reference/kotlin/android/media/AudioTrack
    private lateinit var track: AudioTrack

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        Log.i(TAG, "Start to initialize TTS")
        initTts()
        Log.i(TAG, "Finish initializing TTS")

        Log.i(TAG, "Start to initialize AudioTrack")
        initAudioTrack()
        Log.i(TAG, "Finish initializing AudioTrack")

        text = findViewById(R.id.text)
        sid = findViewById(R.id.sid)
        speed = findViewById(R.id.speed)

        generate = findViewById(R.id.generate)
        play = findViewById(R.id.play)

        generate.setOnClickListener { onClickGenerate() }
        play.setOnClickListener { onClickPlay() }

        sid.setText("1")
        speed.setText("1.0")

        // we will change sampleText here in the CI
        val sampleText = "Of all the problems which have been submitted to my friend, Mister \n" +
                "Sherlock Holmes, for solution during the years of our intimacy, there\n" +
                "were only two which I was the means of introducing to his notice—that\n" +
                "of Mister Hatherley’s thumb, and that of Colonel Warburton’s madness. Of\n" +
                "these the latter may have afforded a finer field for an acute and\n" +
                "original observer, but the other was so strange in its inception and so\n" +
                "dramatic in its details that it may be the more worthy of being placed\n" +
                "upon record, even if it gave my friend fewer openings for those\n" +
                "deductive methods of reasoning by which he achieved such remarkable\n" +
                "results. The story has, I believe, been told more than once in the\n" +
                "newspapers, but, like all such narratives, its effect is much less\n" +
                "striking when set forth _en bloc_ in a single half-column of print than\n" +
                "when the facts slowly evolve before your own eyes, and the mystery\n" +
                "clears gradually away as each new discovery furnishes a step which\n" +
                "leads on to the complete truth. At the time the circumstances made a\n" +
                "deep impression upon me, and the lapse of two years has hardly served\n" +
                "to weaken the effect."

//        val sampleText = "Of all the problems which have been submitted to my friend."
        text.setText(sampleText)
        play.isEnabled = false
    }

    private fun initAudioTrack() {
        val sampleRate = tts.config.sampleRate
        val bufLength = AudioTrack.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_OUT_MONO,
            AudioFormat.ENCODING_PCM_FLOAT
        )
        Log.i(TAG, "sampleRate: ${sampleRate}, buffLength: ${bufLength}")

        val attr = AudioAttributes.Builder().setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .build()

        val format = AudioFormat.Builder()
            .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
            .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
            .setSampleRate(sampleRate)
            .build()

        track = AudioTrack(
            attr, format, bufLength, AudioTrack.MODE_STREAM,
            AudioManager.AUDIO_SESSION_ID_GENERATE
        )
        track.play() //TODO is this needed
    }

    // this function is called from C++
    private fun callback(samples: FloatArray) {
        track.write(samples, 0, samples.size, AudioTrack.WRITE_BLOCKING)
    }

    private fun onClickGenerate() {
        val sidInt = sid.text.toString().toIntOrNull()
        if (sidInt == null || sidInt < 0) {
            Toast.makeText(
                applicationContext,
                "Please input a non-negative integer for speaker ID!",
                Toast.LENGTH_SHORT
            ).show()
            return
        }

        val speedFloat = speed.text.toString().toFloatOrNull()
        if (speedFloat == null || speedFloat <= 0) {
            Toast.makeText(
                applicationContext,
                "Please input a positive number for speech speed!",
                Toast.LENGTH_SHORT
            ).show()
            return
        }

        val textStr = text.text.toString().trim()
        if (textStr.isBlank() || textStr.isEmpty()) {
            Toast.makeText(applicationContext, "Please input a non-empty text!", Toast.LENGTH_SHORT)
                .show()
            return
        }

        track.pause()
        track.flush()
        track.play()

        play.isEnabled = false
        Thread {
            if(false) {
                val audio = tts.generateWithCallback(
                    text = textStr,
                    sid = sidInt,
                    speed = speedFloat,
                    callback = this::callback
                )

                val filename = application.filesDir.absolutePath + "/generated.wav"
                val ok = audio.samples.size > 0  // && audio.save(filename)
                if (ok) {
                    runOnUiThread {
                        play.isEnabled = true
                        track.stop()
                    }
                }
            }
            else {
                val sentences = (textStr.filter{it != '\n'}).split(Regex("""(?<=[.!?\n])\s+"""))
                var total_time = 0L;
                sentences.forEach {
                    val start = System.currentTimeMillis()
                    val audio = tts.generate(
                        text = it,
                        sid = sidInt,
                        speed = speedFloat
                        )
                    total_time += System.currentTimeMillis() - start
                    Log.i(TAG, "textlen: ${it.length}, time: ${System.currentTimeMillis() - start} ms")
                    track.write(audio.samples, 0, audio.samples.size, AudioTrack.WRITE_BLOCKING)
                }
                Log.i(TAG, "Total time: ${total_time} ms")
            }
        }.start()
    }

    private fun onClickPlay() {
        val filename = application.filesDir.absolutePath + "/generated.wav"
        val mediaPlayer = MediaPlayer.create(
            applicationContext,
            Uri.fromFile(File(filename))
        )
        mediaPlayer.start()
    }

    private fun initTts() {
        var modelDir: String?
        var ruleFsts: String?
        var ruleFars: String?
        var lexicon: String?
        var dataDir: String?
        var dictDir: String?
        var assets = application.assets
        var resources = application.resources

        modelDir = "models"
        ruleFsts = null
        ruleFars = null
        lexicon = null
        dataDir = "models/espeak-ng-data"
        dictDir = null

        if (dataDir != null) {
            val newDir = copyDataDir(modelDir!!)
            modelDir = newDir + "/" + modelDir
            dataDir = newDir + "/" + dataDir
        }

        if (dictDir != null) {
            val newDir = copyDataDir(modelDir!!)
            modelDir = newDir + "/" + modelDir
            dictDir = modelDir + "/" + "dict"
            ruleFsts = "$modelDir/phone.fst,$modelDir/date.fst,$modelDir/number.fst"
        }

        val config = getOfflineTtsConfig(
            modelDir = modelDir,
            lexicon = lexicon ?: "",
            dataDir = dataDir ?: "",
            dictDir = dictDir ?: "",
            ruleFsts = ruleFsts ?: "",
            ruleFars = ruleFars ?: "",
        )

        tts = OfflineTts(assetManager = assets, resources = resources, config = config)
    }


    private fun copyDataDir(dataDir: String): String {
        Log.i(TAG, "data dir is $dataDir")
        copyAssets(dataDir)

        val newDataDir = application.getExternalFilesDir(null)!!.absolutePath
        Log.i(TAG, "newDataDir: $newDataDir")
        return newDataDir
    }

    private fun copyAssets(path: String) {
        val assets: Array<String>?
        try {
            assets = application.assets.list(path)
            if (assets!!.isEmpty()) {
                copyFile(path)
            } else {
                val fullPath = "${application.getExternalFilesDir(null)}/$path"
                val dir = File(fullPath)
                dir.mkdirs()
                for (asset in assets.iterator()) {
                    val p: String = if (path == "") "" else path + "/"
                    copyAssets(p + asset)
                }
            }
        } catch (ex: IOException) {
            Log.e(TAG, "Failed to copy $path. $ex")
        }
    }

    private fun copyFile(filename: String) {
        try {
            val istream = application.assets.open(filename)
            val newFilename = application.getExternalFilesDir(null).toString() + "/" + filename
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
