# WifiPrint
<u>Note</u>: _this is just a concept, it does kind of work but it's not easier than sending an email_

You don't care about the story behind this? Here's the [Usage](#usage)

## Why?
### The problem
If someone who doesn't own a printer wants to print something, they usually go to some kind of shop that lets you send them a document to print. The shop requires the client to send an email, but for the average not tech savvy user is "hard" (at least in Italy). I own a shop that between other services has a printing service, and sometimes people comes here to print documents. When the client knows how to send an email, there is always another problem: sometimes they write the wrong email address, sometimes the email takes ages to arrive.

### My solution
I wanted a solution that was easier for the client and faster for the seller, and I thought that connecting to an open WiFi (configured to be able to send only telegram API https requests) via a qr code, that instantly opens a simple web page with a button that lets you select a document, was a good idea.

### Does it actually work?
When I finished writing the code, I tested it with my mum, she has no idea about how to send an email and has average boomer tech knowledge. She didn't find it hard. I got the files and the pre-calculated prices in the telegram chat with the "bot" and all I had to do was print the received document (I didn't really print it).

### The killer issue
Everything works (not yet actually, but I'm working on it), except for one thing: the captive portal login.
What is the captive portal login you ask? It's the default android WiFi login browser, the most important part of this whole project. My plan was to connect to the Access Point, automatically open the browser on the "send the file" page (opens automatically by default on android) and send the file. The issue is that this "browser" (captive portal login) is heavily limited for security reasons, which means that the js code can't open a file choosing dialog or reopen the page on another browser to avoid the first issue.
While the page would still work if opened from another browser, this adds an extra step that increments the complexity of the process making it as "hard" as sending an email.

## About the future of the project
I'll leave all the source code on [github](https://github.com/TheDarkBug/wifiprint), but after finishing to program the "proxy" part (blocking all internet access except for https requests to the telegram API), I'll not update it anymore (unless someone finds a solution to the limited js problem).


## Usage
### Pre-requisites
- I only tested the building process on linux, so use it
- Install cmake, make and some build tools like a compiler (gcc or clang) and I think that's it
- Download the [pico-sdk](https://github.com/raspberrypi/pico-sdk) and set the `$PICO_SDK_PATH` environment variable.
- Install minicom (for debugging purposes)
- Buy a raspberry pi pico w

### Building
Compile the project:
```bash
cmake -B build && {cd build; make; cd ..}
```

### Installation
Connect the pico w to your computer while pressing the BOOT-SELECT button and mount it. Then copy the compiled program to the pico:
```bash
cp build/wifiprint.uf2 /path/to/RPI-RP2
```
And that's it! You can now connect to the AP and enjoy (kind of) the user experience.

### Debugging
The pico has to be connected to your computer:
```bash
sudo minicom -b 115200 -oD /dev/ttyACM0
```
