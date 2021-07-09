success, errmsg = server.setBuffer()

if not success then
    server.log("server.setBuffer() failed: " .. errmsg, server.loglevel.LOG_ERR)
    send_error(500, "buffer could not be set correctly")
    return
end

result = {
    status = "OK",
    config = {
        userid = config.userid,
        port = config.port,
        ssl_port = config.ssl_port,
        docroot = config.docroot,
        tmpdir = config.tmodir,
        scriptdir = config.scriptdir,
        nthreads = config.nthreads,
        keep_alive = config.keep_alive,
        max_post_size = config.max_post_size,
        loglevel = config.loglevel
    },
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
        header = server.header
    }
}

local success, jsonresult = server.table_to_json(result)
if not success then
    server.log("server.table_to_json failed: " .. errmsg, server.loglevel.LOG_ERR)
    send_error(500, "Could not convert to JSON")
    return
end
server.sendHeader('Content-type', 'application/json')
server.sendStatus(200)
server.print(jsonresult)
return
