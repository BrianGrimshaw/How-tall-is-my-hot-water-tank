# How-tall-is-my-hot-water-tank
Using height of hot water tank to estimate how much hot water is left

From https://www.fueltankshop.co.uk/megaflow/b46<br>
Looks like my tank is made from duplex stainless steel<br>
From https://www.theworldmaterial.com/duplex-2205-stainless-steel-uns-s32205-s31803-alloy-material/<br>
Coefficient of expansion is 14 (10-6/k)<br>
Tank is 1540mm tall external - allow 50mm insulation on the bottom and 25mm pipe out of the top which is included in the expansion, so 1515mm tall, so from all cold (20°C) to all hot 60°C) tank will change by 0.85mm<br>
Initially tried measuring using potentiometer, but couldn't figure out how to mount it, so used hall effect sensor<br>
Honeywell SS495A sensor and Eclipse N825 magnet<br>
https://cpc.farnell.com/honeywell/ss495a/sensor-hall-effect-ratiometric/dp/SN36529<br>
https://cpc.farnell.com/eclipse-magnetics/n825/neodymium-disc-magnet-8-x-3mm/dp/TL18509<br>
Magnet glued to pipe clamp<br>
https://www.diy.com/departments/talon-natural-plastic-pipe-clip-dia-22mm-pack-of-5/5055332204201_BQ.prd

I'd like the height to be available on the LAN, so used Arduino R4 Wifi<br>

Wall bracket for sensor uses coach bolts as adjustable feet
![IMG_2740](https://github.com/user-attachments/assets/70f0c40f-c208-4b0d-911b-80ad6ab77730)

Hung from a convenient piece of wood
![IMG_2741](https://github.com/user-attachments/assets/f2cd0837-8206-4f25-a7fc-f662361daef8)

Magnet glued to pipe clip
![IMG_2920](https://github.com/user-attachments/assets/7c343996-f1a7-4eb3-bd04-d1018a3a92e1)

Sensor and magnet in place
![IMG_2922](https://github.com/user-attachments/assets/22bf98c9-1cac-41d4-9ff5-b3bf98376b81)

Arduino R4 wifi with the LED matrix to display remaining hot water
![IMG_4221](https://github.com/user-attachments/assets/99762364-fa67-4d2a-b295-aadb2a31c529)
