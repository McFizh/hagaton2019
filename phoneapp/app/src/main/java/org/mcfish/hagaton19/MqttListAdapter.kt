package org.mcfish.hagaton19

import android.content.Intent
import android.support.v4.content.ContextCompat.startActivity
import android.support.v7.widget.RecyclerView
import android.view.View
import android.view.ViewGroup
import kotlinx.android.synthetic.main.mqttlist_item_row.view.*

class MqttListAdapter(private val services: ArrayList<MqttService>) :
    RecyclerView.Adapter<MqttListAdapter.ServiceHolder>()  {

    override fun onCreateViewHolder(parent: ViewGroup, position: Int): MqttListAdapter.ServiceHolder {
        val inflatedView = parent.inflate(R.layout.mqttlist_item_row, false)
        return ServiceHolder(inflatedView)
    }

    override fun getItemCount(): Int {
        return services.size
    }

    override fun onBindViewHolder(holder: MqttListAdapter.ServiceHolder, position: Int) {
        val itemService = services[position]
        holder.bindService(itemService)
    }

    class ServiceHolder(v: View) : RecyclerView.ViewHolder(v), View.OnClickListener {
        private var view: View = v
        private var service: MqttService? = null

        init {
            v.setOnClickListener(this)
        }

        override fun onClick(v: View) {
            if( service != null ) {
                var intent = Intent(v.context, DataActivity::class.java)
                intent.putExtra("host",(service as MqttService).host)
                v.context.startActivity(intent)
            }
        }

        fun bindService(service: MqttService) {
            this.service = service

            val addr = service.host.toString() + " : " + service.port

            view.serviceName.text = service.name
            view.serviceAddr.text = addr
        }
    }

}

