package org.mcfish.hagaton19

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.support.v7.widget.LinearLayoutManager
import android.util.Log
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    private lateinit var nsdManager: NsdManager
    private lateinit var linearLayoutManager: LinearLayoutManager
    private lateinit var adapter: MqttListAdapter

    private val SERVICE_TYPE = "_mqtt._tcp."
    private var servicesList = ArrayList<MqttService>()

    /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

    private val discoveryListener = object : NsdManager.DiscoveryListener {
        private var TAG = "HAGATON"

        override fun onDiscoveryStarted(regType: String) {
            Log.e(TAG, "Service discovery started")
        }

        override fun onServiceFound(service: NsdServiceInfo) {
            Log.e(TAG, "Service discovery success $service")

            when {
                service.serviceType != SERVICE_TYPE ->
                    Log.e(TAG, "Unknown Service Type: ${service.serviceType}")
                service.serviceName.contains("RabbitMQ") ->
                    nsdManager.resolveService(service, resolveListener)
            }
        }

        override fun onServiceLost(service: NsdServiceInfo) {
            Log.e(TAG, "service lost: $service")
        }

        override fun onDiscoveryStopped(serviceType: String) {
            Log.e(TAG, "Discovery stopped: $serviceType")
        }

        override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {
            Log.e(TAG, "Discovery failed: Error code:$errorCode")
            nsdManager.stopServiceDiscovery(this)
        }

        override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {
            Log.e(TAG, "Discovery failed: Error code:$errorCode")
            nsdManager.stopServiceDiscovery(this)
        }
    }

    /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

    private val resolveListener = object : NsdManager.ResolveListener {
        private var TAG = "HAGATON"

        override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
            Log.e(TAG, "Resolve failed: $errorCode")
        }

        override fun onServiceResolved(serviceInfo: NsdServiceInfo) {

            receivedNewService(MqttService(
                serviceInfo.serviceName,
                serviceInfo.host ,
                serviceInfo.port
            ))

        }
    }

    /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        linearLayoutManager = LinearLayoutManager(this)
        mqttListView.layoutManager = linearLayoutManager

        adapter = MqttListAdapter(servicesList)
        mqttListView.adapter = adapter

        nsdManager = getSystemService(Context.NSD_SERVICE) as NsdManager
        nsdManager.discoverServices(SERVICE_TYPE,
            NsdManager.PROTOCOL_DNS_SD,
            discoveryListener)
    }

    fun receivedNewService(newService: MqttService) {
        runOnUiThread {
            servicesList.add(newService)
            adapter.notifyItemInserted(servicesList.size-1)
        }
    }
}
