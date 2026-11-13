"""Render the English functional wiring schematic; no hardware I/O."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math

ROOT=Path(__file__).resolve().parents[1]
S=2
image=Image.new('RGB',(2400,1820),'#f6f4eb')
draw=ImageDraw.Draw(image)
def text(x,y,value,size=16,color='#203128',bold=False):
    font=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',size*S)
    draw.text((x*S,y*S),value,font=font,fill=color)
def box(x,y,w,h,title,lines,fill='#ffffff'):
    draw.rounded_rectangle((x*S,y*S,(x+w)*S,(y+h)*S),12*S,fill=fill,outline='#b9c8b3',width=2*S)
    text(x+16,y+13,title,20,bold=True)
    for index,line in enumerate(lines):text(x+16,y+48+index*23,line,14)
def route(points,color='#39724e',both=False):
    points=[(x*S,y*S) for x,y in points];draw.line(points,fill=color,width=3*S,joint='curve')
    def head(start,end):
        a=math.atan2(end[1]-start[1],end[0]-start[0]);draw.polygon([end,(end[0]-10*S*math.cos(a-.45),end[1]-10*S*math.sin(a-.45)),(end[0]-10*S*math.cos(a+.45),end[1]-10*S*math.sin(a+.45))],fill=color)
    head(points[-2],points[-1])
    if both:head(points[1],points[0])
text(35,18,'RoboPlanter | Current connection schematic',29,bold=True)
text(36,58,'XIAO ESP32-S3 Plus + Robot Board V1.0 / Indicator D1 / BLE printer',15,color='#647168')
box(35,105,280,110,'Robot Phone / YOYO',['Official Skill BLE tool','Pairs with XIAO, not Indicator'])
box(885,105,280,110,'SenseCAP Indicator D1',['Wi-Fi / authenticated HTTP','Internal RP2040: soil on GPIO27'])
box(435,105,330,415,'XIAO + Robot Board',['State, calibration, action results','','SEN54: I2C GPIO5 / GPIO6','Ultrasonic: J10 RX / D7 / GPIO44','Light: SERVO3 / D0 / GPIO1','LED: RGB / GPIO40','Relay: SERVO1 / GPIO42','','GPIO43, D8 and M2 unused'],fill='#e0ebd9')
route([(315,153),(435,153)],'#5077a5',True);text(323,121,'Encrypted BLE',12,color='#5077a5')
route([(885,153),(765,153)],'#8c6bb0',True);text(778,121,'Wi-Fi / HTTP',12,color='#8c6bb0')
box(35,265,280,110,'Grove SEN54',['SDA5 / SCL6; address 0x69','Independent 5 V; signals 3.3 V'])
route([(315,315),(375,315),(375,205),(435,205)],both=True);text(328,291,'I2C',12)
box(35,425,280,110,'Ultrasonic v2.0',['SIG: D7 / GPIO44','3.3 V; GND; white unused'])
route([(315,478),(393,478),(393,229),(435,229)],both=True)
box(35,595,280,110,'Light Sensor v1.2',['OUT: GPIO1','Independent 3.3 V + GND'])
route([(315,645),(415,645),(415,252),(435,252)])
box(885,265,280,110,'WS2813 / 30 pixels',['DIN: GPIO40; suitable 5 V','Green status / white fill light'])
route([(765,275),(860,275),(860,318),(885,318)]);text(778,249,'GPIO40',12)
box(885,425,280,110,'Grove Relay v1.2',['Signal: GPIO42','Normally open load contacts'])
route([(765,298),(825,298),(825,480),(885,480)]);text(832,400,'GPIO42',12)
box(885,595,280,110,'12 V pump',['Prototype label: 150 mA','Independent supply + protection'])
route([(1025,535),(1025,595)],'#bc813c');text(1036,554,'Switched 12 V',12,color='#98612b')
box(435,595,330,110,'EM5826H + cutter',['XIAO sends BLE ESC/POS','Independent supply as labelled'])
route([(600,520),(600,595)],'#5077a5');text(613,547,'BLE; USB / TTL disconnected',12,color='#5077a5')
text(35,748,'Power and signal notes',20,bold=True)
for index,line in enumerate(['SEN54 VCC is separate 5 V. Do not feed 5 V into the Robot Board IIC 3.3 V pin.','Ultrasonic uses one bidirectional SIG on GPIO44. No printer TX wire may share this pin.','Keep common GND where electrically connected. Never put 12 V on Grove or servo power pins.','The two main boards have separate power and no external UART / I2C cable between them.','This is a functional wiring schematic, not a connector orientation or PCB pin-order drawing.']):
    text(35,785+index*22,line,14,color='#526052')
target=ROOT/'web/experience/assets/current-wiring-en.png'
image.save(target)
print(target)
