--
-- Created by IntelliJ IDEA.
-- User: smoke
-- Date: 2/28/15
-- Time: 4:59 PM
-- To change this template use File | Settings | File Templates.
--
BROKER_IP = "test.mosquitto.org"
-- BROKER_IP = "178.62.67.131"
-- BROKER_IP = "10.220.2.9"
-- BROKER_IP = "10.220.2.18"
-- BROKER_IP = "192.168.43.9"
BROKER_PORT = 1883
BROKER_USER = ""
BROKER_PASS = ""
BROKER_CLIENT = "diy-smart-home-esp-0"
BROKER_TOPIC = "hiveitsmart/swplovdiv/esp-humidity"

m = mqtt.Client(BROKER_CLIENT, 120, BROKER_USER, BROKER_PASS)
m:on("connect", function(con) print ("[mqtt] connected") end)
m:on("offline", function(con) print ("[mqtt] offline") end)

m:on("message", function(conn, topic, data)
    print("[mqtt] " .. topic .. ":" )
    if data ~= nil then
        print("[mqtt] " .. data)
    end
end)

local PIN = 4 --  data pin, GPIO2
local temperaturePollOn = false;
local dht22 = require("dht22")

m:connect(BROKER_IP, BROKER_PORT, 0, function(conn)
    print("[mqtt] connected")

    m:subscribe(BROKER_TOPIC, 0, function(client, topic, message)
        print("[mqtt] subscribe success")
        local topicSub = string.sub(topic, string.len(BROKER_TOPIC) + 1)

        if topicSub == '/ping' then
            m:publish(BROKER_TOPIC..'/ping',"pong",0,0, function(conn) print("[mqtt] sent") end)
        elseif topicSub == '/temperature' then
            dht22.read(PIN)
            local t = dht22.getTemperature()
            m:publish(BROKER_TOPIC..'/temperature',t,0,0, function(conn) print("[mqtt] sent") end)
        elseif topicSub == '/temperature/poll/start' then
            do
                if (temperaturePollOn == true) then
                    do return end
                end

                local t

                temperaturePollOn = true
                local alarmerId = 2

                tmr.alarm(alarmerId, 1000, 1, function()
                    if (temperaturePollOn ~= true) then
                        tmr.stop(alarmerId)
                        temperaturePollOn = false
                        do return end
                    end

                    dht22.read(PIN)
                    t = dht22.getTemperature()

                    m:publish(BROKER_TOPIC..'/temperature/poll', t, 0, 0, function(conn) print("[mqtt] sent") end)
                end)

            end
        elseif topicSub == '/temperature/poll/stop' then
            temperaturePollOn = false
        elseif topicSub == '/humidity' then
            dht22.read(PIN)
            local h = dht22.getHumidity()

            m:publish(BROKER_TOPIC..'/humidity',h,0,0, function(conn) print("[mqtt] sent") end)
        end
    end)

end)