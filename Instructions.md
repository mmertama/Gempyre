# Instructions

## Tips and Hints

### gempyre.conf

You can add a gempyre.conf file into application resources
```
gempyre_add_resources(PROJECT ${PROJECT_NAME}
	TARGET include/ui_resource.h 
	SOURCES gui/ui.html gui/favicon.ico gempyre.conf)
``` 

In there you can define spesific command line and it's command line. E.g.

```json
{
 "cmd_name": "chromium",
 "cmd_params": "--new-window --app=$URL --window-size=800,800 --name=\"Tilze\"",
 "raspberry-cmd_name": "chromium-browser",
 "raspberry-cmd_params": "--new-window --app=$URL --window-size=400,400 --name=\"Tilze\""
}
```

The command line prefix can be "win", "linux", "macos", "android", "raspberry" or "other";
matching to the current os. 
 
Please note that in case of chormium, "--window-size=width,height" has bugs/features and 
therefore sizing windows size afterwards could be more reliable option:
e.g.

```bash
./Tilze && wmctrl -F -r Tilze -e 0,100,100,400,400
```


or relevant PowerShell alternative in Windows (todo example).

 
