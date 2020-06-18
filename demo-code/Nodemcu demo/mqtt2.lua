print("Running file mqtt")
tmr.delay(100000)

gpio12 = 6
gpio.mode(gpio12, gpio.OUTPUT)
gpio.write(gpio12, gpio.LOW)

gpio13 = 7
gpio.mode(gpio13, gpio.OUTPUT)
gpio.write(gpio13, gpio.LOW)

-- init mqtt client with keepalive timer 120sec
m = mqtt.Client("nodemcu", 120, "swqzxzhr", "9JMcxVeUNz3t")

-- setup Last Will and Testament (optional)
-- Broker will publish a message with qos = 0, retain = 0, data = "offline" 
-- to topic "/lwt" if client don't send keepalive packet
m:lwt("/lwt", "offline", 0, 0)

m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)

-- on publish message receive event
m:on("message", function(conn, topic, data)
  print(topic .. ":" )
  if topic == "Light1" then
    if data == "ON" then
        print("received message: ON@light1")
        gpio.write(gpio12, gpio.HIGH)

    else
        print("receive OFF liked data on light1")
        gpio.write(gpio12,gpio.LOW)
    end
    
  else
    if topic == "Light2" then
        if data == "ON" then
            print("received message: ON@light2")
            gpio.write(gpio13, gpio.HIGH)

        else
            print("receive OFF liked data on light2")
            gpio.write(gpio13,gpio.LOW)
        end
    end
    
  end 
  
end)


m:connect("m11.cloudmqtt.com", 19311, 0, function(conn)
    print("connected")

    m:subscribe({["Light1"]=0, ["Light2"]=0}, function(conn) 
        print("subscribe Light 1 and 2 success")
    end)

end)