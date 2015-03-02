wifi.setmode(wifi.STATION)
wifi.sta.config("net365.mobi", "")
wifi.sta.connect()

tmr.alarm(1, 1000, 1, function()
    if wifi.sta.status() ~= 5 then
        print("\wifi connecting...")
        print(wifi.sta.status())
    else
        print("wifi connected")
        print(wifi.sta.status())
        tmr.stop(1)
    end
end)
