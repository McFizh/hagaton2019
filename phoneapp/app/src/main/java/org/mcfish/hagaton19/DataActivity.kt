package org.mcfish.hagaton19

import android.os.Bundle
import android.support.v7.app.AppCompatActivity;
import android.util.Log
import kotlinx.android.synthetic.main.activity_data.*
import org.eclipse.paho.android.service.MqttAndroidClient
import org.eclipse.paho.client.mqttv3.*
import java.net.InetAddress


class DataActivity : AppCompatActivity() {

    private lateinit var mqttAndroidClient: MqttAndroidClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_data)

        connStatusField.text = "Connecting.."
        msgDataField.text = ""
        msgTimeField.text = ""


        var hostAddr = intent.getSerializableExtra("host") as InetAddress
        var hostStr = "tcp://"+hostAddr.hostAddress+":1883"

        mqttAndroidClient = MqttAndroidClient(applicationContext, hostStr, "ExampleAndroidClient")
        mqttAndroidClient.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(reconnect: Boolean, serverURI: String) {

                if (reconnect) {
                    Log.e("HAGATON","MQTT reconnected")
                } else {
                    Log.e("HAGATON","MQTT connection established")
                }
            }

            override fun connectionLost(cause: Throwable) {
                Log.e("HAGATON","MQTT connection lost")
            }

            @Throws(Exception::class)
            override fun messageArrived(topic: String, message: MqttMessage) {
            }

            override fun deliveryComplete(token: IMqttDeliveryToken) {
            }
        } )

        val mqttConnectOptions = MqttConnectOptions()
        mqttConnectOptions.isAutomaticReconnect = false
        mqttConnectOptions.isCleanSession = false
        mqttConnectOptions.userName = "test"
        mqttConnectOptions.password = "test".toCharArray()

        try {
            mqttAndroidClient.connect(mqttConnectOptions, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken) {
                    val disconnectedBufferOptions = DisconnectedBufferOptions()
                    disconnectedBufferOptions.isBufferEnabled = true
                    disconnectedBufferOptions.bufferSize = 100
                    disconnectedBufferOptions.isPersistBuffer = false
                    disconnectedBufferOptions.isDeleteOldestMessages = false
                    mqttAndroidClient.setBufferOpts(disconnectedBufferOptions)
                    Log.e("HAGATON","MQTT onSuccess")
                }

                override fun onFailure(asyncActionToken: IMqttToken, exception: Throwable) {
                    Log.e("HAGATON","MQTT onFailure")
                    exception.printStackTrace()
                }
            })

        } catch(ex:MqttException) {
            ex.printStackTrace()
        }


    }

}
