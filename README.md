# NiosII-LCFR
Low Cost Frequency Relay implemented on Cyclone II for CompSys723

Instructions for Running 

1. Open new workspace in NIOS II 

2. Import lcfr-u and lcfr-u_bsp projects 

3. Compile/build the lcfr-u project 

4. Connect power, VGA, keyboard and the USB blaster to the board 

5. Press run in NIOS and select hardware 

Interface 

- Use switches 0-4 for controlling the loads.  

- Highest priority load starts from 0 and goes to 4. 

- Button 1 is used to switch to maintenance mode. 

- Using the numbers and dot enables changing the config values displayed on screen. 

- The entered values will cycle through each one when the enter button is pressed. 

- Pressing tab discards the currently entered value and passes to the next. 

- Backspacing clears the currently entered value. 

- Vertical white line through the two graphs shows the last 5 values on the right. The graph shows last 100 values in total. 

- Maximum 8 digits can be entered 