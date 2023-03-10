
function pre_flight(prefix,identifier,cookie)
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