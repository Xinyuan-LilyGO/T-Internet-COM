# T-Internet-COM Basic 
**Note:** The project Demo is derived from the official example of ESP-IDF, with a small amount of modification based on different pins and configurations.<br>
If you need advanced learning, please refer to [ESP-IDP](/README2.md) information


# Qucik Start
## Usually use ESP-IDF example has the following steps
> 1. Set chip target.
> 2. Enter the menu to configure the project.
> 3. build project.
> 4. Upload program.


### Detailed steps:
1. Make sure there is an ESP-IDF environment in the computer. If not, please refer to the [official guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for installation.

2. After cloning the project, use the ESP-IDF command to enter the ETH2AP folder, for example:<br>
If the file happens to be in the root directory of drive D
```
cd D:\eth2ap
```

3. Set ESP32 chip target. Enter the menu configuration page.
```
idf.py set-target esp32
idf.py menuconfig
```
- Ensure that the pin settings in the example configuration are consistent with the figure
![](/img/1.png)
![](/img/2.png)

- Use ETH io0 as output

![](/img/3.png)
![](/img/4.png)
![](/img/5.png)
![](/img/6.png)
![](/img/7.png)

- Press `S` to save the settings

4. Start to build the project
```
idf.py build
```

5. Wait for the completion of the project build, upload the firmware to the device
```
idf.py flash
```
