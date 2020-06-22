# RBTray

RBTray is a small Windows program that runs in the background and allows almost any window to be minimized to the system
tray by:

- Right-Clicking its minimize button
- Shift-Right-Clicking on its title bar
- Using the Windows-Alt-Down hotkey

Note that not all of these methods will work for every window, so please use whichever one works for your needs.

## Downloading / Installing

Please download the zip file for either the 64 bit (x64) binaries or 32 (x86) bit binaries (depending on your Windows
version) from the [Releases page](https://github.com/benbuck/rbtray/releases). If you aren't sure which one to use,
probably you should use the x64 one. It's recommended that you use the latest version.

Extract the contents of the zip file to a folder, for example "C:\Program Files\RBTray". From that folder, double click
RBTray.exe to start it. If you want it to automatically start after you reboot, create a shortcut to RBTray.exe in your
Start menu's Startup group.

If you want it to automatically start after you reboot, create a shortcut to RBTray.exe in your Start menu's Startup
group, which is located at `"%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"`, or by pressing "`WIN` + `R`"
then typing `shell:Startup`.

## Using

To minimize a program to the system tray, you can use any of these methods:

- Right-click with the mouse on the program's minimize button.
- Hold the Shift key while Right-clicking on the program's title bar.
- Pressing Windows-Alt-Down on the keyboard (all at the same time).

This should create an icon for the window in the system tray. To restore the program's window, single-click the
program's icon in the tray. Alternatively, you can Right-click on the tray icon which should bring up a popup menu, then
select Restore Window.

In some cases the first two methods cause problems with other software because of they way they integrate into Windows
using a hook to intercept mouse events. In this case, you can use the `--no-hook` option, which means that only the last
method of using a hotkey will work. Also the RBHook.dll isn't needed since it only exists to support the mouse event
hook.

## Exiting

Right click on any tray icon created by RBTray and click Exit RBTray in the popup menu, or run RBTray.exe with the
`--exit` parameter.

## Credits

For original version, please see the [RBTray SourceForge project page](http://sourceforge.net/projects/rbtray/).

Nikolay Redko: [http://rbtray.sourceforge.net/], [https://github.com/nredko]

J.D. Purcell: [http://www.moitah.net/], [https://github.com/jdpurcell]

Benbuck Nason: [https://github.com/benbuck]

## Legal

RBTray is free, open source, and is distributed under the terms of the
[GNU General Public Licence](http://www.gnu.org/copyleft/gpl.html).

Copyright &copy; 1998-2011 Nikolay Redko, J.D. Purcell

Copyright &copy; 2015 Benbuck Nason
