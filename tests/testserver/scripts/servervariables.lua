---
--- Created by rosenth.
--- DateTime: 10.07.21 22:53
---

result = {
    status = "OK",
    config = config,

    server = {
        method = server.method,
        has_openssl = server.has_openssl,
        client_ip = server.client_ip,
        secure = server.secure,
        client_port = server.client_port,
        host = server.host,
        uri = server.uri,
        get = server.get,
        cookies = server.cookies,
        header = server.header,
        post = server.post,
        uploads = server.uploads,
        content = server.content,
    },
}

local success, jsonresult = server.table_to_json(result)
if not success then
    server.log("server.table_to_json failed: " .. errmsg, server.loglevel.LOG_ERR)
    server.sendStatus(500)
    print.print("server.table_to_json failed: " .. errmsg)
    return false
end
server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonresult)
return true
