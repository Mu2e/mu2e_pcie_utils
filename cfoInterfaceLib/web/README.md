DTC Web Interface v0.2.1
Eric Flumerfelt, FNAL RSI
12/18/2014

===============================================================================
===                               DESCRIPTION                               ===
===============================================================================

The DTC Web Interface allows for browser-based control of the DTC PCIe card.
Users are authenticated using their KCA certificate, and authorized from a 
Kerberos .k5login file. The interface includes direct register IO, Register
R/W operations through named "LED-style" indicators, and monitoring of PCIe
rate through SVG plots, which are updated in real-time at 1s intervals.

The Web Server also sends PCIe rate data to Ganglia via the gmetric.node
plugin.


===============================================================================
===                                 FILES                                   ===
===============================================================================

Server:
 -- serverbase.js: Main HTTPS server file. Performs user authentication and
                   authorization. Translates HTTP GET and POST requests into
                   DTCDriver.js calls.
 -- DTCDriver.js:  Javascript interface to the C++ DTCInterfaceLib library.
                   Performs DTC operations and translates results into
                   Javascript objects. Has several convienience functions,
                   such as GetRegDump() and GetSend/RecieveStatistics()
 -- DTC.node:      Node.js module built from SWIG-wrapped C++ header file.
                   Interfaces with the DTC UNIX driver to read and write
                   DTC registers.
 -- gmetric.node:  Node.js module built from SWIG-wrapped send_gmetric library,
                   available in the artdaq-utilities repository. Sends metric
                   data to a local instance of Ganglia.

Client:
 -- d3.v3.min.js, jquery.min.js: Javascript libraries for performing various
                                 client-side tasks.
 -- style.css:   Web style control file, mainly used for formatting SVG graphs
 -- client.html: Defines HTML elements that client.js interacts with
 -- client.js:   Javascript program which requests data from the server and
                 displays this data in the form of "LED-style" indicators and
                 SVG graphs. Runs on the client; very low server-load.


===============================================================================
===                            INSTALLATION                                 ===
===============================================================================

Requires:
  -- DTC Card with Ron's driver installed. (/dev/mu2e must be accessible)
  -- Node.js v10.33 or later (no additional modules required)

Untar in directory of choice. Run `node serverbase.js` to start server.
Direct client to https://`hostname`:8080 (or :9090 if running in "dev" 
subdirectory).

===============================================================================
===                              CHANGELOG                                  ===
===============================================================================
v0.2.1, 12/18/2014: Addition of Ganglia metric logging. SVG graph code has been
                    tweaked so that clipping works correctly. CSS improvements
                    to improve visual appearance of SVG graphs and simplify
                    placement of these graphs within documents.
v0.2a, 12/15/2014: Authorization has been tweaked so that any authenticated
                   user can perform RO operations. (Authentication is by KCA
                   certificate, authorization by .k5login file).
v0.2, 12/15/2014: Includes authorization and authentication functionality, LED
                  indicators, SVG plots courtesy of d3.js and AJAX data 
                  requests: GETs for the plotting and POSTS for register dumps
                  and io.
v0.1, 12/05/2014: Minimum working version of client/server code. Can perform
                  register I/O operations, but that's about it.