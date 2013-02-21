README:
---
members: 
Ray Powers, Nichole Minas, Russell Asher

---
assumptions: 
max threads capped at 50

---
description: 
This program will provide a simple and easy to use HTTP/1.0 Web Proxy Cache server.

---
Compile:
make

---
usage:
./proxyserver [port]

---
to set up web browser (firefox) and configure it to use HTTP/1.0:
Firefox
Version 2.0:
1. Select Tools->Options from the menu.
2. Click on the 'Advanced' icon in the Options dialog.
3. Select the 'Network' tab, and click on 'Settings' in the 'Connections' area.
4. Select 'Manual Proxy Configuration' from the options available. In the boxes, enter the
hostname and port where proxy program is running.

Earlier Versions:
1. Select Edit->Preferences from the menu.
2. On the 'General' tab, click 'Connection Settings'.
3. Select 'Manual Proxy Configuration' and enter the hostname and port where your proxy is
running.

To stop using the proxy server, select 'Direct connection to the Internet' in the connection settings
dialog.

Configuring Firefox to use HTTP/1.0
Because Firefox defaults to using HTTP/1.1 and your proxy speaks HTTP/1.0, there are a couple of minor
changes that need to be made to Firefox's configuration. Fortunately, Firefox is smart enough to know
when it is connecting through a proxy, and has a few special configuration keys that can be used to
tweak the browser's behavior.
1. Type 'about:config' in the title bar.
2. In the search/filter bar, type 'network.http.proxy'
3. You should see three keys: network.http.proxy.keepalive,
network.http.proxy.pipelining, and network.http.proxy.version.
4. Set keepalive to false. Set version to 1.0. Make sure that pipelining is set to false.
