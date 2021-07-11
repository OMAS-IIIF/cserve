---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by rosenth.
--- DateTime: 11.07.21 19:16
---
function send_error(msg)
    server.print(msg)
    server.log(msg, server.loglevel.LOG_ERR)
    server.sendStatus(500)
    return false
end

local filesize
local origname
local mimetype
local consistency
local cnt = 0
local fpath
for fileindex, fileinfo in pairs(server.uploads) do
    filesize = fileinfo.filesize
    origname = fileinfo.origname
    mimetype = fileinfo.mimetype

    success, consistency = server.file_mimeconsistency(fileindex)
    if not success then
        return send_error(errmsg)
    end

    fpath = config.docroot .. '/' .. fileinfo.origname
    success, errmsg = server.copyTmpfile(fileindex, fpath)
    if not success then
        return send_error(errmsg)
    end
    cnt = cnt + 1
end

result = {
    status = 'OK',
    cnt = cnt,
    origname = origname,
    mimetype = mimetype,
    filesize = filesize,
    consistency = consistency
}
local success, jsonresult = server.table_to_json(result)

--
-- remove uploaded file
--
success, errmsg = server.fs.unlink(fpath)
if not success then
    return send_error(errmsg)
end

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
