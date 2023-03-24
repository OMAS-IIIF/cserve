
function iiif_preflight(prefix, identifier, cookie)
    if prefix == 'auth' then
        filepath = iiifhandler.imgroot .. "/" .. identifier
        return {
            type = 'login',
            cookieUrl = 'https://localhost/iiif-cookie.html',
            tokenUrl = 'https://localhost/iiif-token.php',
            confirmLabel =  'Login to OMAS IIIF',
            description = 'This Example requires a demo login!',
            failureDescription = '<a href="http://example.org/policy">Access Policy</a>',
            failureHeader = 'Authentication Failed',
            header = 'Please Log In',
            label = 'Login to OMAS IIIF',
        }, filepath
    end

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