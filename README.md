# [VDT 2023 - IoT] Bài thi esp32 ngày 3.6.2023

## Đề bài
- Lắng nghe topic messages/{device_ID}/status để nhận bản tin điều khiển trạng thái led. Định dạng bản tin `{“led”: {gpio_pin}, “status”:“{status_led}”}.`
- Điều khiển thiết bị bằng button 0 có trên Esp32
    + Nhấn 2 lần: Đảo trạng thái led, gửi bảng tin trạng thái lên server
    + Nhấn 4 lần: Khởi động lại chế độ smartconfig
- Mỗi 60s thì thiết bị sẽ gửi bản tin heartbeat với topic update như mục 2, định dạng bản tin `{“heartbeat”: 1}`

Lưu ý: Mỗi bản tin gửi lên thiết bị sẽ đăng ký sub ở topic `messages/{device_ID}/status` để nhận phản hồi, bản tin phản hồi sẽ có định dạng `{“code”: 1}`, thời gian chờ phản hồi có thể tự điều chỉnh theo ý các bạn, các bản tin không nhận được phản hồi sẽ tự động gửi lại sau 20s.

## Ý tưởng thuật toán

## Kết quả thực nghiệm 
### Thông báo mỗi khi bấm nút 0
```
PJ: Button detect!
```
Tùy vào số lượng mà có thể cho ra kết quả là `PJ: Smart config!` (4 nút), `PJ: led toggle` (ấn 2 lần), hoặc `PJ nothing` trong các trường hợp khác
### Đảo trạng thái led
```
I (103307) PJ: Button detect!
I (103307) PJ: Button detect!
I (103307) PJ: led toggle
I (103447) PJ: MQTT_EVENT_DATA
DATA={"led":2,"status":"off"}
I (103447) PJ: pin: 2, status: 0
I (103447) gpio: GPIO[2]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 1| Intr:0
```

### Gửi bản tin Heartbeat
```
I (123457) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=6
I (123457) PJ: MQTT_EVENT_DATA
DATA={"heartbeat":1}
I (123467) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=6
I (123467) PJ: MQTT_EVENT_DATA
DATA={"code":1}
```
### Restart lại SmartConfig

```
I (61106) smartconfig: Start to find channel...
I (61106) PJ: Scan done
I (65576) smartconfig: TYPE: ESPTOUCH
I (65576) smartconfig: Found channel on 9-0. Start to get ssid and password...
I (65576) PJ: Found channel
I (91436) smartconfig: F|pswd: dihoithangtrung
I (91436) smartconfig: F|ssid: B5-402
I (91436) wifi:ic_disable_sniffer
I (91436) PJ: Got SSID and password
I (91446) PJ: SSID:B5-402
I (91446) PJ: PASSWORD:dihoithangtrung
I (91456) wifi:new:<9,0>, old:<9,0>, ap:<255,255>, sta:<9,0>, prof:1
I (91466) wifi:state: init -> auth (b0)
I (91466) wifi:state: auth -> assoc (0)
I (91486) wifi:state: assoc -> run (10)
I (101486) wifi:state: run -> init (cc00)
I (101486) wifi:new:<9,0>, old:<9,0>, ap:<255,255>, sta:<9,0>, prof:1
I (103546) wifi:new:<9,0>, old:<9,0>, ap:<255,255>, sta:<9,0>, prof:1
I (103546) wifi:state: init -> auth (b0)
I (103556) wifi:state: auth -> assoc (0)
I (103576) wifi:state: assoc -> run (10)
I (103706) wifi:connected with B5-402, aid = 5, channel 9, BW20, bssid = cc:71:90:4c:90:80
I (103706) wifi:security: WPA2-PSK, phy: bgn, rssi: -50
I (103706) wifi:pm start, type: 1

I (103786) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (104626) esp_netif_handlers: sta ip: 192.168.1.3, mask: 255.255.255.0, gw: 192.168.1.1
I (104626) PJ: WiFi Connected to ap
I (107656) PJ: smartconfig over
I (107656) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=7
I (107656) PJ: Other event id:7
I (107716) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=1
I (107726) PJ: MQTT_EVENT_CONNECTED
I (107736) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=3
I (107736) PJ: MQTT_EVENT_SUBSCRIBED, msg_id=61827
I (107736) PJ: Event dispatched from event loop base=MQTT_EVENTS, event_id=6
I (107746) PJ: MQTT_EVENT_DATA
DATA={"code":1}
I (107746) PJ: code response received
I (109116) PJ: Button detect!
I (110126) PJ: Button detect!
I (111086) PJ: Button detect!
I (111966) PJ: Button detect!
I (113966) PJ: Smart config!
I (114016) wifi:state: run -> init (0)
I (114016) wifi:pm stop, total sleep time: 6591739 us / 10307588 us

I (114016) wifi:new:<9,0>, old:<9,0>, ap:<255,255>, sta:<9,0>, prof:1
W (114026) wifi:hmac tx: stop, discard
W (114026) wifi:hmac tx: stop, discard
I (114076) wifi:flush txq
I (114076) wifi:stop sw txq
I (114076) wifi:lmac stop hw txq
I (114076) wifi:mode : sta (34:86:5d:3b:17:94)
I (114126) smartconfig: SC version: V2.9.0
I (118216) wifi:ic_enable_sniffer
I (118226) smartconfig: Start to find channel...
I (118226) PJ: Scan done
I (131516) smartconfig: TYPE: ESPTOUCH
I (131516) smartconfig: Found channel on 9-0. Start to get ssid and password...
I (131516) PJ: Found channel
I (145616) smartconfig: F|pswd: dihoithangtrung
I (145616) smartconfig: F|ssid: B5-402
I (145616) wifi:ic_disable_sniffer
I (145616) PJ: Got SSID and password
I (145616) PJ: SSID:B5-402
I (145626) PJ: PASSWORD:dihoithangtrung
I (145636) wifi:new:<9,0>, old:<9,0>, ap:<255,255>, sta:<9,0>, prof:1
I (145636) wifi:state: init -> auth (b0)
I (145646) wifi:state: auth -> assoc (0)
I (145696) wifi:state: assoc -> run (10)
I (145806) wifi:connected with B5-402, aid = 7, channel 9, BW20, bssid = cc:71:90:4c:90:80
I (145806) wifi:security: WPA2-PSK, phy: bgn, rssi: -45
I (145806) wifi:pm start, type: 1

I (145876) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (146626) esp_netif_handlers: sta ip: 192.168.1.3, mask: 255.255.255.0, gw: 192.168.1.1
```