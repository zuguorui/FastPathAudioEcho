package com.zu.fastpathaudioecho

import android.Manifest
import android.content.Context
import android.media.AudioManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Toast
import com.permissionx.guolindev.PermissionX
import kotlinx.android.synthetic.main.activity_main.*
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue

class MainActivity : AppCompatActivity() {

    private var playState: Boolean = false
        set(value) {
            field = value
            if(value)
            {
                btn_start.text = "Stop"
            }
            else
            {
                btn_start.text = "Start"
            }
        }

    private var sampleRate: Int = 0
        set(value) {
            field = value
            tv_sample_rate.text = value.toString()
        }

    private var framesPerBuffer: Int = 0
        set(value) {
            field = value
            tv_frames_per_buffer.text = value.toString()
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        PermissionX.init(this).permissions(Manifest.permission.RECORD_AUDIO).request{
                allGranted, grantedList, deniedList ->
            if (allGranted) {
                Toast.makeText(this, "All permissions are granted", Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(this, "These permissions are denied: $deniedList", Toast.LENGTH_LONG).show()
            }
        }

        val audioManager = getSystemService(Context.AUDIO_SERVICE) as AudioManager
        sampleRate = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE).toInt()
        framesPerBuffer = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER).toInt()

        nInit(sampleRate, framesPerBuffer)

        playState = false

        btn_start.setOnClickListener{
            if(playState)
            {
                nStop()
            }
            else
            {
                nStart()
            }
            playState = !playState
        }

    }

    override fun onDestroy() {
        nDestroy()
        super.onDestroy()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun nInit(sampleRate: Int, framePerBuffer: Int): Boolean
    external fun nDestroy()
    external fun nStart()
    external fun nStop()

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
