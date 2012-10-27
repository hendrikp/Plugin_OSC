OSC Plugin for CryEngine SDK
============================
OSC - [Open Sound Control](http://opensoundcontrol.org) protocol integration for CryEngine

This plugin relies on [oscpkt](http://gruntthepeon.free.fr/oscpkt) a very lightweight OSC implementation.

About OSC
---------
Open Sound Control (OSC) is a content format for messaging among computers, sound synthesizers,
and other multimedia devices that are optimized for modern networking technology.
Bringing the benefits of modern networking technology to the world of electronic musical instruments,
OSC's advantages include interoperability, accuracy, flexibility, and enhanced organization and documentation.

Useful Stuff
------------
* [Touch-OSC](http://hexler.net/software/touchosc) for iOS/Android (connect CryEngine with your phone)
* [OSC-Datamonitor](http://www.kasperkamperman.com/blog/osc-datamonitor) monitors the OSC datastream (find out the data types and message names)
* [OSCeleton](https://github.com/Sensebloom/OSCeleton) for Kinect skeleton data
* [many more...](http://en.wikipedia.org/wiki/Open_Sound_Control#Implementations)

Installation / Integration
==========================
Extract the files to your Cryengine SDK Folder so that the Code and BinXX/Plugins directory match up.

The plugin manager will automatically load up the plugin when the game/editor is restarted or if you directly load it.

Flownodes
=========
Please note there can be only one connection node for each IP and port. Use the signaler
plugin to reuse connections in different flowgraphs.

Connection
----------
* ```OSC_Plugin:Connection``` Connection data source or destination
  * In ```Init``` Connects or starts listening (will fire ```InitAll``` output)
  * In ```Close``` Disconnect or close (resets ```InitAll``` output)
  * In ```sHost``` host/ip to bind/connect
  * In ```nPort``` port to listen/connect
  * In ```nType``` Server (Receive) or Client (Send)
  * Out ```InitAll``` connect all ```Receive:Message``` or ```Send:Packet``` that should use this connection

Receiving Data (UDP Server)
---------------------------
* ```OSC_Plugin:Receive:Message``` Registers a message that can be received
  * In ```Init``` Registers the message with the connected ```Connection```
  * In ```sMessage``` Messagetext/identifier/path
  * Out ```InitNext``` connect the first ```Receive:Value:*``` of this message (the order is important)

* ```OSC_Plugin:Receive:Value:Float32``` Registers a message parameter that will be read
  * In ```Init``` Registers the value with the connected ```Receive:Message``` or via ```Receive:Value:*```
  * Out ```InitNext``` connect the next ```Receive:Value:*``` of this message (the order is important)
  * Out ```Value``` outputs the value if received 

* ```OSC_Plugin:Receive:Value:Any``` Use this to skip over parameters with unsupported or unkown type
  * In ```Init``` Registers the value with the connected ```Receive:Message``` or via ```Receive:Value:*```
  * Out ```InitNext``` connect the next ```Receive:Value:*``` of this message (the order is important)
  * no Value

* ```OSC_Plugin:Receive:Value:Int32``` same as Float32
* ```OSC_Plugin:Receive:Value:Int64``` same as Float32
* ```OSC_Plugin:Receive:Value:Double64``` same as Float32
* ```OSC_Plugin:Receive:Value:String``` same as Float32
* ```OSC_Plugin:Receive:Value:Bool``` same as Float32

Sending Data (UDP Client)
-------------------------
* ```OSC_Plugin:Send:Packet``` Register a packet that can be sent (define at least one Bundle if you have more then one message)
  * In ```Init``` registers the packet in  the connected ```Connection```
  * In ```Send``` triggers a manual send on this packets content
  * In ```bAutoSend``` Activate automatic sending on value change inside this packet
  * Out ```InitNext``` connect the first ```Send:Bundle``` or ```Send:Message``` that is inside this packet

* ```OSC_Plugin:Send:BundleStart``` Start a Bundle inside a packet (Bundles can be nested)
  * In ```Init``` Start a new Bundle inside the packet/bundle
  * Out ```InitNext``` connect the next ```Send:Value:*```, ```Send:Message``` or ```Send:Bundle*```  of this packet (the order is important)

* ```OSC_Plugin:Send:BundleEnd``` End the Bundle
  * In ```Init``` End the current Bundle inside the packet/bundle
  * Out ```InitNext``` connect the next ```Send:Value:*```, ```Send:Message``` or ```Send:Bundle*```  of this packet (the order is important)

* ```OSC_Plugin:Send:Message``` Register a message in a packet
  * In ```Init``` registers the message in  the connected ```Send:Packet```, ```Send:Bundle``` or via ```Send:Value:*```
  * In ```sMessage``` Messagetext/identifier/path
  * Out ```InitNext``` connect the first ```Send:Value``` of this message

* ```OSC_Plugin:Send:Value:Float32```  Registers a message parameter that will be sent
  * In ```Init``` receive the value from ```Send:Message```
  * In ```Value``` if triggered with a new value it will send the packet if auto send is active, if not then it will save the value and it will get sent when ```Send:Packet```->```Send``` is triggered.
  * Out ```InitNext``` connect the next ```Send:Value:*```, ```Send:Message``` or ```Send:Bundle*```  of this packet (the order is important)

* ```OSC_Plugin:Send:Value:Int32``` same as Float32
* ```OSC_Plugin:Send:Value:Int64``` same as Float32
* ```OSC_Plugin:Send:Value:Double64``` same as Float32
* ```OSC_Plugin:Send:Value:String``` same as Float32
* ```OSC_Plugin:Send:Value:Bool``` same as Float32
