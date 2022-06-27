cserve = {
    itest = 1234,
    ftest = 0.123,
    stest = "from lua",
    dstest = "8KB",
    lltest = "OFF",
    lrtest = {
        {
            method = "GET",
            route = "/lualua",
            script = "lualua.lua"
        }
    }
}