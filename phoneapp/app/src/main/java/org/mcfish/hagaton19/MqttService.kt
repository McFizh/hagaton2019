package org.mcfish.hagaton19

import java.net.InetAddress

class MqttService(pName: String, pAddr: InetAddress, pPort: Int) {
    var name: String = pName
    var host: InetAddress = pAddr
    var port: Int = pPort
}
