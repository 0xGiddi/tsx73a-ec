# Just notes for myself on development

1. Some model config files have 3 codes, what up with that? There does not seem 
   a corresponding extra X_VPD=EC in the config.
2. Not all configs are the same, especially the [System Disk] sections, need to figure out what info to take and how to handle in module. Also, when parsing it had some keys defined multiple times that throw an error in the (really bad please fix) parse script.
3. 44 configs (list below) do not have EC index bytes. Mostt likely the dont have led for every disk but a single LED for all disks (rackmount?) need to fire gout from the original binary how to handle setting the LEDs without an EC index.
4. Power redundency unit? I dont have hardware....


List for bullet 1:
```text
model_Q00Q0_Q00S0_Q00R0_12_12_11.conf
model_Q0410_Q0490_Q04U0_10_10_10.conf
model_Q0411_Q0490_Q04U0_13_10_10.conf
```


List for bullet 3:
```text
model_Q03X0_Q04K0_10_10.18.conf
model_Q03X0_Q04K0_10_10.conf
model_Q0520_Q0580_11_11.conf
model_Q0520_Q0580_23_12.conf
model_Q0520_Q05G0_11_11.conf
model_Q0520_Q05G1_11_11.conf
model_Q0520_Q0670_11_10.conf
model_Q0520_Q0670_23_11.conf
model_Q0520_Q0671_11_10.conf
model_Q0580_Q0671_11_11.conf
model_Q0711_Q0760_10_10.conf
model_Q09A0_Q09C0_10_10.conf
model_Q09A0_Q09C0_12_11.conf
model_Q09B0_Q09I0_10_10.conf
model_QZ480_Q0060_10_10.conf
model_QZ480_Q0060_11_10.conf
model_QZ480_Q0060_20_10.conf
model_QZ481_Q0060_11_10.conf
model_QZ482_Q0060_12_10.conf
model_QZ490_QZ550_12_12.conf
model_QZ490_QZ551_12_12.conf
model_QZ490_QZ560_12_13.conf
model_QZ491_QZ540_11_11.conf
model_QZ492_QZ540_13_12.conf
model_QZ492_QZ540_15_12.conf
model_QZ492_QZ550_13_13.conf
model_QZ492_QZ550_15_13.conf
model_QZ492_QZ551_13_13.conf
model_QZ492_QZ551_15_13.conf
model_QZ492_QZ552_13_13.conf
model_QZ493_QZ540_20_12.conf
model_QZ493_QZ550_20_13.conf
model_QZ493_QZ551_20_13.conf
model_QZ493_QZ552_20_13.conf
model_QZ494_QZ540_21_12.conf
model_QZ494_QZ550_21_14.conf
model_QZ494_QZ551_21_14.conf
model_QZ500_Q0070_10_10.conf
model_QZ500_Q0070_12_10.conf
model_QZ502_Q0070_13_10.conf
model_QZ503_Q0070_14_10.conf
model_QZ503_Q0070_15_10.conf
model_QZ504_Q0070_20_10.conf
model_SAP10_SBO90_10_10.conf
```