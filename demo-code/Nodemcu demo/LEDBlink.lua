local M = {}
    
function M.fast()
    gpio_16 = 0
    gpio.mode(gpio_16, gpio.OUTPUT)
    gpio_blink = 0
        
    tmr.alarm(2, 500, 1, function()
        if gpio_blink == 0 then
            gpio_blink = 1
            gpio.write(gpio_16,gpio.HIGH)
        else
            gpio_blink = 0
            gpio.write(gpio_16,gpio.LOW)
        end 
    end)
      
end

return M