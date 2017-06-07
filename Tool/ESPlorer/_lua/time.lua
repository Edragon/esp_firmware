print('time.lua ver 1.0')
function getHTTPreq()
   print('send GET to http server...')
   conn=net.createConnection(net.TCP, 0) 
   conn:on("receive", function(conn, payload)
     print("got answer")
     print(string.sub(payload,string.find(payload,"Date: ")+6,string.find(payload,"Date: ")+35))
     conn:close()
   end) 
     conn:connect(80,"64.233.161.94") 
     conn:send("HEAD / HTTP/1.1\r\n"..
               "Accept: */*\r\n"..
               "User-Agent: Mozilla/4.0 (compatible; esp8266 NodeMcu Lua;)\r\n"..
               "\r\n") 
end
tmr.alarm(0, 1000, 1, function() 
   if wifi.sta.getip()==nil then
      print("connecting to AP...") 
   else
      print('ip: ',wifi.sta.getip())
      getHTTPreq()
      tmr.stop(0)
   end
end)
