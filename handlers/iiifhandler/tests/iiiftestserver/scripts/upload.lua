---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by rosenth.
--- DateTime: 11.07.21 19:16
---
require "send_response"


local filesize
local origname
local mimetype
local cnt = 0
local fpath

local myimg = {}
local newfilename = {}
local iiifurls = {}

if server.secure then
    protocol = 'https://'
else
    protocol = 'http://'
end

files = {}
for imgindex, fileinfo in pairs(server.uploads) do
    local mimetype
    local success, consistency
    success, consistency = server.file_mimeconsistency(imgindex)
    if not success then
        return send_error(500, consistency)
    end

    if not consistency then
        local mimetypeobj
        success, mimetypeobj = server.file_mimetype(imgindex)
        if not success then
            server.log("Couldn't determine mimetype!", server.loglevel.error)
            send_error(500, mimetypeobj)
            return false
        end
        mimetype = mimetypeobj.mimetype
    else
        mimetype = fileinfo["mimetype"]
    end

    local info = {
        filesize = fileinfo.filesize,
        origname = fileinfo.origname,
        mimetype = mimetype,
        consistency = consistency
    }

    if mimetype == "image/tiff" or mimetype == "image/jpeg" or mimetype == "image/png" or mimetype == "image/jpx" or mimetype == "image/jp2" then
        success, myimg[imgindex] = IIIFImage.new(imgindex)
        if not success then
            server.log(myimg[imgindex], server.loglevel.error)
            send_error(500, myimg[imgindex])
            return false
        end

        filename = fileinfo["origname"]
        filebody = filename:match("(.+)%..+")
        newfilename[imgindex] = "_" .. filebody .. '.jp2'
        info["filename"] = newfilename[imgindex]

        --
        -- Set orientation to topleft
        --
        --myimg[imgindex]:topleft()

        --
        -- Create the destination path
        --
        fullfilepath = config.imgroot .. '/' .. newfilename[imgindex]

        --
        -- write the file to the destination
        --
        local status, errmsg = myimg[imgindex]:write(fullfilepath)
        if not status then
            server.print('Error converting image to j2k: ', filename, ' ** ', errmsg)
        end
    else
        fpath = config.imgroot .. '/' .. fileinfo.origname
        info["filename"] = fileinfo.origname
        success, errmsg = server.copyTmpfile(fileindex, fpath)
        if not success then
            return send_error(500, errmsg)
        end
    end
    table.insert(files, info)
    cnt = cnt + 1
end

result = {
    status = 'OK',
    cnt = cnt,
    files = files
}
return send_success(result)
