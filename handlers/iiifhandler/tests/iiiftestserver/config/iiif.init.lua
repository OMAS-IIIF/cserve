
function iiif_preflight(prefix,identifier,cookie)
    if config.prefix_as_path then
        filepath = iiifhandler.imgroot .. '/' .. prefix .. '/' .. identifier
    else
        filepath = iiifhandler.imgroot .. '/' .. identifier
    end
    if identifier == "RestrictLeaves.jpg" then
        return {type = 'restrict', size = iiifhandler.thumbsize }, filepath
    elseif identifier == "DenyLeaves.jpg" then
        return "deny", ""
    else
        return "allow", filepath
    end
end

function file_preflight(filepath, cookie)
    if filepath == iiifhandler.imgroot .. '/test_deny.csv' then
        return "deny", ""
    else
        return "allow", filepath
    end
end