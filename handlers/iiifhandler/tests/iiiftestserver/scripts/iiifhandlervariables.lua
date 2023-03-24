---
--- Created by rosenth.
--- DateTime: 10.07.21 22:53
---
require "send_response"

result = {
    status = "OK",
    iiifhandler = iiifhandler
}

send_success(result)
return true
