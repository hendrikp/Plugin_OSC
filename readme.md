OSC Plugin for CryEngine SDK
============================
OSC - Open Sound Control Protocol integration for CryEngine

For redistribution please see license.txt.

This plugin relies on [oscpkt](http://gruntthepeon.free.fr/oscpkt) a very lightweight OSC implementation.

Installation / Integration
==========================
Extract the files to your Cryengine SDK Folder so that the Code and BinXX/Plugins directory match up.

The plugin manager will automatically load up the plugin when the game/editor is restarted or if you directly load it.

Flownodes
=========
Please note there can be only one connection node for each IP and Port.

Details on Usage and Howto video will follow.

Connection
----------
* OSC_Plugin:Connection

Receiving Data (UDP Server)
---------------------------
* OSC_Plugin:Receive:Message

* OSC_Plugin:Receive:Value:Any
* OSC_Plugin:Receive:Value:Float32

* OSC_Plugin:Receive:Value:Int32
* OSC_Plugin:Receive:Value:Int64
* OSC_Plugin:Receive:Value:Double64
* OSC_Plugin:Receive:Value:String
* OSC_Plugin:Receive:Value:Bool

Sending Data (UDP Client)
-------------------------
* OSC_Plugin:Send:Packet

* OSC_Plugin:Send:Message

* OSC_Plugin:Send:BundleStart
* OSC_Plugin:Send:BundleEnd

* OSC_Plugin:Send:Value:Float32

* OSC_Plugin:Send:Value:Int32
* OSC_Plugin:Send:Value:Int64
* OSC_Plugin:Send:Value:Double64
* OSC_Plugin:Send:Value:String
* OSC_Plugin:Send:Value:Bool
