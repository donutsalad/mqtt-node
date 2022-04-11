# ESP32 MQTT Node Firmware Prototype

This is just a prototype for an operating system I plan to write for the ESP32.
I'm going to do a rewrite of everything when I'm comfordable with how this project functions, but I'll be leaving it in place so feel free to fork or take what you like!

The rewrite may be near identical on the first commit or completely different, so if you plan to make a project that's based on this please keep in mind you may have to do a complete re-write to stay up to date, or take over maintaining this yourself!

## Basic Functionality
 - [x] Boot and host a WLAN to prompt user for WiFi details
 - [ ] Prompt user for MQTT details and boot mode
 - [ ] Allow user to store default settings in NVS for handsoff-init
 - [ ] Establish custom desktop server connection and init presence
 - [ ] Establish a command interface with the desktop mqtt server
 - [ ] Pin exposure on a regularly polled basis
 - [ ] Basic use case tasks and commands
 - [ ] OnDemand and OnChange update modes
 - [ ] Full GPIO exposure and customisation (Might migrate during this)
 - [ ] Extended HAL API exposure (Probably migrate before this)


## HTML Subproject
The webserver responces are stored like so `#define TAG "<html>[...]</html>"`, and thusly require a src->inline conversion that removes whitespace and escapes the `"` character. This would be tedius to do by hand so I've written a basic utility in C# to converts the web src.

In order to run the utility just cd into the html folder, `dotnet run` and follow the prompts.

Below are a few caveats:
 - You will likely need to `sudo` to write into the ../main directory if you code in a dev container
 - The parser doesn't strip extended spaces, only tabs - thusly src must be tab indented
 - I haven't checked it on windows/macOS machines, but it should work since I used Path.Combine, YMMV
