tftpd = ({
urv=nil,
socket=nil,
start=function(this)
    local state=0 -- 0 = idle 1 = put request in progress
    local filename,blocks
    local useFlash = false
    local blocksize = 512
    local function tohex(str)
            str=tostring(str)
            return (str:gsub('.', function (c)
                return string.format('%02X', string.byte(c))
            end))
    end

    local function senderr(conn,msg)
        conn:send(string.char(00,05,00,00) .. msg .. string.char(00))
    end

    function sendack(conn,hbyte,lbyte)
        conn:send(string.char(00,04) .. string.char(hbyte) .. string.char(lbyte) )
    end

    local function sendoptionack(conn,size)
        conn:send(string.char(00,06) .. "blksize" .. string.char(0) .. size  .. string.char(0) )
    end

    local function abort(conn,msg)
        senderr(conn,msg)
        if state == 1 then
            file.close()
            file.remove(filename)
        end
        state = 0
        print("ABORT:" .. msg)
        uart.on("data")
    end

    this.urv=net.createServer(net.UDP) 
    this.urv:on("receive",function(conn,payload) 
        this.socket=conn
--        print(tohex(payload:sub(1,30))) 
        local major,minor=payload:byte(1,2)
        local count = 0
        if state == 0 then
            if minor == 2 then
                filename=payload:match("^..(.-)%z")
                filename, count = string.gsub(filename, "ESP$", "", 1) 
                reqblocksize=payload:match("blksize%z(.-)%z")
                state = 1 -- receiving
                blocks = 0
                if (reqblocksize) then 
                    blocksize = tonumber(reqblocksize)
                    sendoptionack(conn,reqblocksize)
                else
                    sendack(conn,0,0)
                end
                if count > 0 then
                  file.open(filename,"w+")
                  useFlash = true
                  uart.write (0,"F" .. filename .. "|" .. blocksize .. "\n")
                else
                  uart.write (0,"B" .. filename .. "|" .. blocksize .. "\n")
                  useFlash = false
                  uart.on("data", 5, function(data) 
                    if data=="quit\n" then
                        uart.on("data") 
                    end
                    local blocks = tonumber(data)
                    if blocks then
                        local hbyte = math.floor((blocks % 2^16) / 2^8)
                        local lbyte = blocks % 2^8

                        sendack(conn,hbyte,lbyte)
                      end
                  end, 0)
                end
            else
                abort(conn,"Unsupported")
            end
        elseif state == 1 then
            if minor == 3 then
                local major,minor,blockh,blockl,data=payload:match("^(.)(.)(.)(.)(.*)$")
                local rblock = blockh:byte(1) * 256 + blockl:byte(1)
                blocks = blocks + 1
                if blocks > 65535 then
                    blocks = 0
                end
                if rblock == blocks then
--                    print(string.format("Receiving Block %i ",blockh:byte(1) * 256 + blockl:byte(1) ) )
                    if useFlash == true then
                      file.write(data)
                      sendack(conn,blockh:byte(1),blockl:byte(1))
                    else
                      uart.write (0, "D" .. string.format("%05d", rblock) .. "|" .. string.format("%04d", #payload -4) .. "|")
 --                       for b in string.gfind(data, ".") do
 --                       uart.write(0, string.format("%02X", string.byte(b)))
 --                       end
 --                       uart.write(0, "\r\n")
 --                       collectgarbage()
                      uart.write (0, data .. string.rep("X",blocksize - #payload))
 --                     sendack(conn,blockh:byte(1),blockl:byte(1))
                    end
                    if #payload - 4 < blocksize then
                        if useFlash == true then
                          uart.write(0,"E " .. filename .. "\r\n")
                          file.close()
                        else
                          uart.write (0,"E \r\n")
--                          uart.on("data")
                          sendack(conn,blockh:byte(1),blockl:byte(1))
                        end
                        state = 0
                    end
                else
                    if rblock > blocks then
                        abort(conn,"Sorry. missed something. Try again")    
                    else
                        -- print("Skipping duplicate")
                        blocks = blocks - 1
                        if useFlash == true then
                            local hbyte = math.floor((blocks % 2^16) / 2^8)
                            local lbyte = blocks % 2^8
                            sendack(conn,hbyte,lbyte)
                        else -- resend
                            uart.write (0, "D" .. string.format("%05d", rblock) .. "|" .. string.format("%04d", #payload -4) .. "|")
                            uart.write (0, data .. string.rep("X",blocksize - #payload))
                        end
                    end
                end
        else
                abort(conn,"unexpected command. please retry.")
            end
        end
    end) 
    this.urv:listen(69) 
end,
stop=function(this)
    if this.socket then this.socket:close() this.socket=nil end
    this.urv:close() this.urv=nil
end,
remack=function(this,block)
    if this.socket then
        local hbyte = math.floor((block % 2^16) / 2^8)
        local lbyte = block % 2^8
        sendack(this.socket,hbyte,lbyte)
       end
end
})
