# Coursework

Client/Server application to see user screens on the remote host.
- Client has to make a "print-screen" every 60 seconds.
- All screens must be stored in the client app folder.
- Each screen name consists of the date when it has been taken + the number of screens.
Ex: 21.07.2018_15.png 21.07.2018_16.png ...etc
- Server can request screens by those names.
- File must be transferred to the server via the 5555 port.

## Solution
Netshot is the proposed solution for this task. It is a cross-platform application for client-server communication. It allows the server to start screenshot capturing on the client host, get the list of available screenshots and download them. 

## How to compile
Netshot can be compiled using the ```make``` tool. It will compile the netshot application in the ```bin``` directory (```bin/netshot.out``` for Linux and ```bin\netshout.exe``` for Windows systems).
You might like to use the Msys2 or Cygwin tools for the Windows system. 
It's also possible that on the UNIX-like systems you'd need to install the ```libx11-dev``` library. 
**For Ubuntu:**
```sudo apt install libx11-dev```
## How to use
### Signature
For Windows:
```netshot.exe [address] [-l] [-i interface] [-p port] [-d display]```
For Linux:
```./netshot.out [address] [-l] [-i interface] [-p port] [-d display]```
- ```[address]``` - Address of a server to connect. The address must be specified unless the ```-l``` option is given.
- ```[-l]``` - Used to specify that netshot should listen for the incoming connections rather than initiate a connection to a remote host—sets netshot to the server mode.
- ```[-i interface]``` - Specifies the interface's IP, which is used to listen for the new connections; specifying the interface when the ```-l``` flag is used is obligatory. It is an error to use this option without the ```-l``` flag.
- ```[-p port]``` - Source or destination port. When the ```-l``` flag is specified, ```-p``` stand for a port to listen on. Otherwise, it specifies the server port to connect. Generally, the port must be specified with or without the ```-l``` flag.
- ```[-d display]``` - Used to select the display interface on which screen capturing will be performed; if it is not set, the default display is used (the one, set as the ```DISPLAY``` environmental variable for Linux). It should not be specified in listening mode.
 

### Methods
There are five methods available in the **interactive server mode**:

##### PRINT - Prints the given message to the console
 - Parameters: **MESSAGE:** *<text>*
 - Return value: None

##### RUN_SCAP - starts screen capturing
- Parameters: **INTERVAL**: *<seconds>*
- Return value: *None*

##### STOP_SCAP - stop screen capturing
 - Parameters: *None*
 - Return value: *None*

##### LIST_SCAP - returns the list of available screenshots
 - Parameters: *None*
 - Return value: Sets the srfc-response payload with filenames, separated with the newline character.

##### GETFILE_SCAP - returns the requested screenshot
- Parameters: **NAME:** *<filename>*
- Return value: Sets the srfc-response payload with the requested image in the binary format. 

## How it works
The proposed solution is cross-platform (can be compiled for UNIX-like and Windows systems). For screen capturing, the X Window System protocol client library (**Xlib** ) was used for Linux and **Windows GDI** component for Windows systems. For platform-independent, asynchronous and bi-directional network communication, the **SRFC protocol was designed**, and the **SRFC-oriented network library was implemented**.

### Screen capturing
For Linux systems, screen capturing is performed using the [Xlib](https://www.x.org/releases/X11R7.5/doc/libX11/libX11.html). **Xlib** is an X Window System protocol client library in the C programming language. It contains functions for interacting with an X server. Xlib was chosen because **all Linux desktop systems are built on top of it**. For Windows-based implementation, screen capturing is done using the built-in **Windows GDI** component.

### Networking 
For the client-server communication, the **SRFC** (**S**imple **R**emote **F**unction **C**all) protocol was designed, and the appropriate library for asynchronous client-server communication exploiting the designed SRFC protocol was implemented. The structure of SRFC messages is depicted below:

<br><br> 
![Messages structure](/doc/img/Protocol [BC C_C++ 2022] Task 1.drawio.png)
<br><br> 

The implemented SRFC-Library offers high-level functionality for platform-independent asynchronous and bi-directional communication. **To use the full capabilities of SRFC, you should directly utilise the proposed functionality.**
By default, the server is launched in the **interactive mode**, which allows interactive request/response building, sending, receiving and saving. However, the capabilities of interactive mode are significantly cut off. I.e., it can't work with the binary data and non-ASCII-7 encodings. Also, working with several connections simultaneously in this mode is impossible. Additionally, method parameters can't contain non-alphanumeric symbols. Hence, it should be used only for debugging and demonstrating purposes. To use all capabilities, utilise the implemented SRFC functionality.
### Screenshots format
Screenshots are stored in the **Portable PixMap format** (.ppm), which is the [Netpbm](https://en.wikipedia.org/wiki/Netpbm#File_formats) format with the P6 Type. Saved images can be viewed, for example, using [online Netpbm viewer](http://paulcuth.me.uk/netpbm-viewer/)

## Known issues:
- ```--help``` and ```--version``` commands are not yet implemented
- Non-IPv4 addresses are not supported yet
- Non-ASCII7 encodings are not supported in the interactive server mode 


