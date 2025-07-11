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
