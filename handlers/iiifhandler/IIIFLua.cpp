#include <string>
#include <iostream>
#include <cstring>

#include <Connection.h>
#include <Parsing.h>

#include "IIIFCache.h"
#include "IIIFImage.h"
#include "IIIFLua.h"
#include "IIIFError.h"
//#include "IIIFHandler.h"

namespace cserve {

    char iiifhandler_token[] = "__iiifhandler";

    static const char SIMAGE[] = "IIIFImage";

    typedef struct {
        IIIFImage *image;
        std::string *filename;
    } SImage;

    static std::shared_ptr<IIIFCache> cache_getter(lua_State *L) {
        lua_getglobal(L, iiifhandler_token);
        auto *iiif_handler = (IIIFHandler *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stack
        return iiif_handler->cache();
    }

    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_size(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        unsigned size = cache->getCachesize();
        lua_pushinteger(L, size);
        return 1;
    }

    /*!
     * Get the maximal size of the cache
     * LUA: cache.max_size = cache.max_size()
     */
    static int lua_cache_max_size(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        unsigned maxsize = cache->getMaxCachesize();
        lua_pushinteger(L, maxsize);
        return 1;
    }

    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_nfiles(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        unsigned size = cache->getNfiles();
        lua_pushinteger(L, size);
        return 1;
    }

    /*!
     * Get the size of the cache
     * LUA: cache_size = cache.size()
     */
    static int lua_cache_max_nfiles(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        unsigned size = cache->getMaxNfiles();
        lua_pushinteger(L, size);
        return 1;
    }

    /*!
     * Get path to cache dir
     * LUA: cache_path = cache.path()
     */
    static int lua_cache_path(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        std::string cpath = cache->getCacheDir();
        lua_pushstring(L, cpath.c_str());
        return 1;
    }

    static void
    add_one_cache_file(int index, const std::string &canonical, const IIIFCache::CacheRecord &cr, void *userdata) {
        lua_State *L = (lua_State *) userdata;

        lua_pushinteger(L, index);
        lua_createtable(L, 0, 4); // table1

        lua_pushstring(L, "canonical");
        lua_pushstring(L, canonical.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "origpath");
        lua_pushstring(L, cr.origpath.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "cachepath");
        lua_pushstring(L, cr.cachepath.c_str());
        lua_rawset(L, -3);

        lua_pushstring(L, "size");
        lua_pushinteger(L, cr.fsize);
        lua_rawset(L, -3);

        struct tm *tminfo;
        tminfo = localtime(&cr.access_time);
        char timestr[100];
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tminfo);
        lua_pushstring(L, "last_access");
        lua_pushstring(L, timestr);
        lua_rawset(L, -3);

        lua_rawset(L, -3);

        return;
    }

    static int lua_cache_filelist(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        int top = lua_gettop(L);
        std::string sortmethod;
        if (top == 1) {
            sortmethod = std::string(lua_tostring(L, 1));
        }
        lua_pop(L, top);

        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }

        lua_createtable(L, 0, 0); // table1
        if (sortmethod == "AT_ASC") {
            cache->loop(add_one_cache_file, (void *) L, IIIFCache::SortMethod::SORT_ATIME_ASC);
        } else if (sortmethod == "AT_DESC") {
            cache->loop(add_one_cache_file, (void *) L, IIIFCache::SortMethod::SORT_ATIME_DESC);
        } else if (sortmethod == "FS_ASC") {
            cache->loop(add_one_cache_file, (void *) L, IIIFCache::SortMethod::SORT_FSIZE_ASC);
        } else if (sortmethod == "FS_DESC") {
            cache->loop(add_one_cache_file, (void *) L, IIIFCache::SortMethod::SORT_FSIZE_DESC);
        } else {
            cache->loop(add_one_cache_file, (void *) L);
        }

        return 1;
    }

    static int lua_delete_cache_file(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        int top = lua_gettop(L);
        if (cache == nullptr) {
            lua_pop(L, top);
            lua_pushnil(L);
            return 1;
        }
        std::string canonical;
        if (top == 1) {
            canonical = std::string(lua_tostring(L, 1));
            lua_pop(L, 1);
            lua_pushboolean(L, cache->remove(canonical));
        } else {
            lua_pop(L, top);
            lua_pushboolean(L, false);
        }
        return 1;
    }

    static int lua_purge_cache(lua_State *L) {
        std::shared_ptr<IIIFCache> cache = cache_getter(L);
        if (cache == nullptr) {
            lua_pushnil(L);
            return 1;
        }
        int n = cache->purge(true);
        lua_pushinteger(L, n);
        return 1;
    }

    static const luaL_Reg cache_methods[] = {{"size",       lua_cache_size},
                                             {"max_size",   lua_cache_max_size},
                                             {"nfiles",     lua_cache_nfiles},
                                             {"max_nfiles", lua_cache_max_nfiles},
                                             {"path",       lua_cache_path},
                                             {"filelist",   lua_cache_filelist},
                                             {"delete",     lua_delete_cache_file},
                                             {"purge",      lua_purge_cache},
                                             {0,            0}};


    static SImage *toSImage(lua_State *L, int index) {
        auto *img = (SImage *) lua_touserdata(L, index);
        if (img == nullptr) {
            lua_pushstring(L, "Type error: Not userdata object");
            lua_error(L);
        }
        return img;
    }
    //=========================================================================

    static SImage *checkSImage(lua_State *L, int index) {
        luaL_checktype(L, index, LUA_TUSERDATA);
        auto img = (SImage *) luaL_checkudata(L, index, SIMAGE);
        if (img == nullptr) {
            lua_pushstring(L, "Type error: Expected an IIIFImage");
            lua_error(L);
        }
        return img;
    }
    //=========================================================================

    static SImage *pushSImage(lua_State *L, const SImage &simage) {
        auto *img = (SImage *) lua_newuserdata(L, sizeof(SImage));
        *img = simage;
        luaL_getmetatable(L, SIMAGE);
        lua_setmetatable(L, -2);
        return img;
    }
    //=========================================================================

    /*
     * Lua usage:
     *    img = SipiImage.new("filename")
     *    img = SipiImage.new("filename",
     *    {
     *      region=<iiif-region-string>,
     *      size=<iiif-size-string>,
     *      reduce=<integer>,
     *      original=origfilename},
     *      hash="md5"|"sha1"|"sha256"|"sha384"|"sha512"
     *    })
     */
    static int SImage_new(lua_State *L) {
        lua_getglobal(L, luaconnection);
        Connection *conn = (Connection *) lua_touserdata(L, -1);
        lua_remove(L, -1); // remove from stacks
        int top = lua_gettop(L);

        if (top < 1) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.new(): No filename given");
            return 2;
        }

        int pagenum = 0;
        std::shared_ptr<IIIFRegion> region;
        std::shared_ptr<IIIFSize> size;
        std::string original;
        HashType htype = HashType::sha256;
        std::string imgpath;

        if (lua_isinteger(L, 1)) {
            std::vector<Connection::UploadedFile> uploads = conn->uploads();
            int tmpfile_id = static_cast<int>(lua_tointeger(L, 1));
            try {
                imgpath = uploads.at(tmpfile_id - 1).tmpname; // In Lua, indexes are 1-based.
                original = uploads.at(tmpfile_id - 1).origname;
            } catch (const std::out_of_range &oor) {
                lua_settop(L, 0); // clear stack
                lua_pushboolean(L, false);
                lua_pushstring(L, "'IIIFImage.new()': Could not read data of uploaded file. Invalid index?");
                return 2;
            }
        }
        else if (lua_isstring(L, 1)) {
            imgpath = lua_tostring(L, 1);
        }
        else {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.new(): filename must be string or index");
            return 2;
        }

        if (top == 2) {
            if (lua_istable(L, 2)) {
                //lua_pop(L,1); // remove filename from stack
            } else {
                lua_pop(L, top);
                lua_pushboolean(L, false);
                lua_pushstring(L, "IIIFImage.new(): Second parameter must be table");
                return 2;
            }

            lua_pushnil(L);

            while (lua_next(L, 2) != 0) {
                if (lua_isstring(L, -2)) {
                    const char *param = lua_tostring(L, -2);

                    if (strcmp(param, "pagenum") == 0) {
                        if (lua_isnumber(L, -1)) {
                            pagenum = lua_tointeger(L, -1);
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in pagenum parameter");
                            return 2;
                        }
                    } else if (strcmp(param, "region") == 0) {
                        if (lua_isstring(L, -1)) {
                            region = std::make_shared<IIIFRegion>(lua_tostring(L, -1));
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in region parameter");
                            return 2;
                        }
                    } else if (strcmp(param, "size") == 0) {
                        if (lua_isstring(L, -1)) {
                            size = std::make_shared<IIIFSize>(lua_tostring(L, -1));
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in size parameter");
                            return 2;
                        }
                    } else if (strcmp(param, "reduce") == 0) {
                        if (lua_isnumber(L, -1)) {
                            size = std::make_shared<IIIFSize>(static_cast<int>(lua_tointeger(L, -1)));
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in reduce parameter");
                            return 2;
                        }
                    } else if (strcmp(param, "original") == 0) {
                        if (lua_isstring(L, -1)) {
                            const char *tmpstr = lua_tostring(L, -1);
                            original = tmpstr;
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in original parameter");
                            return 2;
                        }
                    } else if (strcmp(param, "hash") == 0) {
                        if (lua_isstring(L, -1)) {
                            const char *tmpstr = lua_tostring(L, -1);
                            std::string hashstr = tmpstr;
                            if (hashstr == "md5") {
                                htype = HashType::md5;
                            } else if (hashstr == "sha1") {
                                htype = HashType::sha1;
                            } else if (hashstr == "sha256") {
                                htype = HashType::sha256;
                            } else if (hashstr == "sha384") {
                                htype = HashType::sha384;
                            } else if (hashstr == "sha512") {
                                htype = HashType::sha512;
                            } else {
                                lua_pop(L, lua_gettop(L));
                                lua_pushboolean(L, false);
                                lua_pushstring(L, "IIIFImage.new(): Error in hash type");
                                return 2;
                            }
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "IIIFImage.new(): Error in hash parameter");
                            return 2;
                        }
                    } else {
                        lua_pop(L, lua_gettop(L));
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "IIIFImage.new(): Error in parameter table (unknown parameter)");
                        return 2;
                    }
                }

                /* removes value; keeps key for next iteration */
                lua_pop(L, 1);
            }
        }

        lua_pushboolean(L, true); // result code
        SImage simg;
        simg.image = new IIIFImage();
        simg.filename = new std::string(imgpath);
        SImage *img = pushSImage(L, simg);

        try {
            if (!original.empty()) {
                img->image->readOriginal(imgpath, pagenum, region, size, original, htype);
            } else {
                img->image->read(imgpath, pagenum, region, size);
            }
        } catch (IIIFImageError &err) {
            delete img->image;
            img->image = nullptr;
            delete img->filename;
            img->filename = nullptr;
            lua_pop(L, lua_gettop(L));
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "IIIFImage.new(): ";
            ss << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        return 2;
    }
    //=========================================================================

    static int SImage_dims(lua_State *L) {
        size_t nx, ny;

        if (lua_gettop(L) != 1) {
            lua_pop(L, lua_gettop(L));
            lua_pushboolean(L, false);
            lua_pushstring(L, "SipiImage.dims(): Incorrect number of arguments");
            return 2;
        }

        if (lua_isstring(L, 1)) {
            const char *imgpath = lua_tostring(L, 1);
            IIIFImage img;
            IIIFImgInfo info;
            try {
                info = img.getDim(imgpath);
            }
            catch (InfoError &e) {
                lua_pop(L, lua_gettop(L));
                lua_pushboolean(L, false);
                std::stringstream ss;
                ss << "SipiImage.dims(): Couldn't get dimensions";
                lua_pushstring(L, ss.str().c_str());
                return 2;
            }
            catch (IIIFImageError &err) {
                lua_pop(L, lua_gettop(L));
                lua_pushboolean(L, false);
                std::stringstream ss;
                ss << "SipiImage.dims(): " << err;
                lua_pushstring(L, ss.str().c_str());
                return 2;
            }
            nx = info.width;
            ny = info.height;
        } else {
            SImage *img = checkSImage(L, 1);
            if (img == nullptr) {
                lua_pop(L, lua_gettop(L));
                lua_pushboolean(L, false);
                lua_pushstring(L, "SipiImage.dims(): not a valid image");
                return 2;
            }
            nx = img->image->getNx();
            ny = img->image->getNy();
        }

        lua_pop(L, lua_gettop(L));

        lua_pushboolean(L, true);
        lua_createtable(L, 0, 2); // table

        lua_pushstring(L, "nx"); // table - "nx"
        lua_pushinteger(L, nx); // table - "nx" - <nx>
        lua_rawset(L, -3); // table

        lua_pushstring(L, "ny"); // table - "ny"
        lua_pushinteger(L, ny); // table - "ny" - <ny>
        lua_rawset(L, -3); // table

        return 2;
    }
    //=========================================================================

    /*!
     * Lua usage:
     *    SipiImage.mimetype_consistency(img, "image/jpeg", "myfile.jpg")
     */
    static int SImage_mimetype_consistency(lua_State *L) {
        int top = lua_gettop(L);

        // three arguments are expected
        if (top != 3) {
            lua_pop(L, top); // clear stack
            lua_pushboolean(L, false);
            lua_pushstring(L, "SipiImage.mimetype_consistency(): Incorrect number of arguments");
            return 2;
        }

        // get pointer to SImage
        SImage *img = checkSImage(L, 1);

        // get name of uploaded file
        std::string *path = img->filename;

        // get the indicated mimetype and the original filename
        const char *given_mimetype = lua_tostring(L, 2);
        const char *given_filename = lua_tostring(L, 3);

        lua_pop(L, top); // clear stack

        // do the consistency check

        bool check;

        try {
            check = Parsing::checkMimeTypeConsistency(*path, given_filename, given_mimetype);
        } catch (IIIFImageError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "IIIFImage.mimetype_consistency(): " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        lua_pushboolean(L, true); // status
        lua_pushboolean(L, check); // result

        return 2;

    }
    //=========================================================================

    /*!
     * SipiImage.crop(img, <iiif-region>)
     */
    static int SImage_crop(lua_State *L) {
        SImage *img = checkSImage(L, 1);
        int top = lua_gettop(L);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.crop(): Incorrect number of arguments");
            return 2;
        }

        const char *regionstr = lua_tostring(L, 2);
        lua_pop(L, top);
        std::shared_ptr<IIIFRegion> reg;

        try {
            reg = std::make_shared<IIIFRegion>(regionstr);
        } catch (IIIFError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "IIIFImage.crop(): " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        img->image->crop(reg); // can not throw exception!

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
    * SipiImage.scale(img, sizestring)
    */
    static int SImage_scale(lua_State *L) {
        SImage *img = checkSImage(L, 1);
        int top = lua_gettop(L);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.scale(): Incorrect number of arguments");
            return 2;
        }

        const char *sizestr = lua_tostring(L, 2);
        lua_pop(L, top);
        size_t nx, ny;

        try {
            IIIFSize size(sizestr);
            int r;
            bool ro;
            size.get_size(img->image->getNx(), img->image->getNy(), nx, ny, r, ro);
        } catch (IIIFError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "IIIFImage.scale(): " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        img->image->scale(nx, ny);

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
    * SipiImage.rotate(img, number)
    */
    static int SImage_rotate(lua_State *L) {
        int top = lua_gettop(L);

        if ((top < 2) || (top > 3)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.rotate(): Incorrect number of arguments");
            return 2;
        }

        SImage *img = checkSImage(L, 1);

        if (!lua_isnumber(L, 2)) {
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.rotate(): Incorrect  argument for angle");
            return 2;
        }

        float angle = lua_tonumber(L, 2);
        bool mirror{false};
        if (top == 3) {
            if (!lua_isboolean (L, 3)) {
                lua_pushboolean(L, false);
                lua_pushstring(L, "IIIFImage.rotate(): Incorrect  argument for mirror");
                return 2;
            }
            mirror = lua_toboolean(L, 3);
        }
        lua_pop(L, top);

        img->image->rotate(angle, mirror); // does not throw an exception!

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    /*!
     * SipiImage.watermark(img, <wm-file>)
     */
    static int SImage_watermark(lua_State *L) {
        int top = lua_gettop(L);

        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.watermark(): Incorrect arguments");
            return 2;
        }

        const char *watermark = lua_tostring(L, 2);
        lua_pop(L, top);

        try {
            img->image->add_watermark(watermark);
        } catch (IIIFImageError &err) {
            lua_pushboolean(L, false);
            std::stringstream ss;
            ss << "IIIFImage.watermark(): " << err;
            lua_pushstring(L, ss.str().c_str());
            return 2;
        }

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================


    /*!
     * SipiImage.write(img, <filepath> [, compression_parameter])
     */
    static int SImage_write(lua_State *L) {

        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, lua_gettop(L));
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.write(): Incorrect arguments");
            return 2;
        }

        const char *imgpath = lua_tostring(L, 2);

        IIIFCompressionParams comp_params;
        if (lua_gettop(L) > 2) { // we do have compressing parameters
            if (lua_istable(L, 3)) {
                lua_pushnil(L);
                while (lua_next(L, 3) != 0) {
                    if (!lua_isstring(L, -2)) {
                        lua_pop(L, lua_gettop(L));
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "IIIFImage.write(): Incorrect compression value name: Must be string!");
                        return 2;
                    }
                    const char *key = lua_tostring(L, -2);
                    if (!lua_isstring(L, -1)) {
                        lua_pop(L, lua_gettop(L));
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "IIIFImage.write(): Incorrect compression value: Must be string!");
                        return 2;
                    }
                    const char *tmpvalue = lua_tostring(L, -1);
                    std::string value(tmpvalue);
                    if (key == std::string("Sprofile")) {
                        std::set<std::string> validvalues {"PROFILE0", "PROFILE1", "PROFILE2", "PART2",
                                                           "CINEMA2K", "CINEMA4K", "BROADCAST", "CINEMA2S", "CINEMA4S",
                                                           "CINEMASS", "IMF"};
                        if (validvalues.find(value) != validvalues.end()) {
                            comp_params[J2K_Sprofile] = value;
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Sprofile!");
                            return lua_error(L);
                        }
                    } else if (key == std::string("Creversible")) {
                        if (value == "yes" || value == "no") {
                            comp_params[J2K_Creversible] = value;
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Creversible!");
                            return lua_error(L);
                        }
                    } else if (key == std::string("Clayers")) {
                        try {
                            int i = std::stoi(value);
                            std::stringstream ss;
                            ss << i;
                            value = ss.str();
                        } catch (std::invalid_argument &) {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Clayers!");
                            return lua_error(L);
                        } catch(std::out_of_range &) {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Clayers!");
                            return lua_error(L);
                        }
                        comp_params[J2K_Clayers] = value;
                    } else if (key == std::string("Clevels")) {
                        try {
                            int i = std::stoi(value);
                            std::stringstream ss;
                            ss << i;
                            value = ss.str();
                        } catch (std::invalid_argument &) {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Clevels!");
                            return lua_error(L);
                        } catch(std::out_of_range &) {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Clevels!");
                            return lua_error(L);
                        }
                        comp_params[J2K_Clevels] = value;
                    } else if (key == std::string("Corder")) {
                        std::set<std::string> validvalues = {"LRCP", "RLCP", "RPCL", "PCRL", "CPRL"};
                        if (validvalues.find(value) != validvalues.end()) {
                            comp_params[J2K_Corder] = value;
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Corder!");
                            return lua_error(L);
                        }
                    } else if (key == std::string("Cprecincts")) {
                        comp_params[J2K_Cprecincts] = value;
                    } else if (key == std::string("Cblk")) {
                        comp_params[J2K_Cblk] = value;
                    } else if (key == std::string("Cuse_sop")) {
                        if (value == "yes" || value == "no") {
                            comp_params[J2K_Cuse_sop] = value;
                        } else {
                            lua_pop(L, lua_gettop(L));
                            lua_pushstring(L, "IIIFImage.write(): invalid Cuse_sop!");
                            return lua_error(L);
                          }
                    } else if (key == std::string("rates")) {
                        comp_params[J2K_rates] = value;
                    } else if (key == std::string("quality")) {
                        comp_params[JPEG_QUALITY] = value;
                    } else {
                        lua_pop(L, lua_gettop(L));
                        lua_pushstring(L, "IIIFImage.write(): invalid compression parameter!");
                        return lua_error(L);
                    }
                    lua_pop(L, 1);
                }
            }
        }

        std::string filename = imgpath;
        size_t pos_ext = filename.find_last_of(".");
        size_t pos_start = filename.find_last_of("/");
        std::string dirpath;
        std::string basename;
        std::string extension;

        if (pos_start == std::string::npos) {
            pos_start = 0;
        } else {
            dirpath = filename.substr(0, pos_start);
        }

        if (pos_ext != std::string::npos) {
            basename = filename.substr(pos_start, pos_ext - pos_start);
            extension = filename.substr(pos_ext + 1);
        } else {
            basename = filename.substr(pos_start);
        }

        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::string ftype;

        if ((extension == "tif") || (extension == "tiff")) {
            ftype = "tif";
        } else if ((extension == "jpg") || (extension == "jpeg")) {
            ftype = "jpg";
        } else if (extension == "png") {
            ftype = "png";
        } else if ((extension == "j2k") || (extension == "jpx") || (extension == "jp2")) {
            ftype = "jpx";
        } else {
            lua_pop(L, lua_gettop(L));
            lua_pushstring(L, "Image.write(): unsupported file format");
            return lua_error(L);
        }

        if ((basename == "http") || (basename == "HTTP")) {
            lua_getglobal(L, luaconnection); // push onto stack
            Connection *conn = (Connection *) lua_touserdata(L, -1); // does not change the stack
            lua_remove(L, -1); // remove from stack
            img->image->connection(conn);
            try {
                img->image->write(ftype, "HTTP", comp_params);
            } catch (IIIFImageError &err) {
                lua_pop(L, lua_gettop(L));
                lua_pushboolean(L, false);
                lua_pushstring(L, err.to_string().c_str());
                return 2;
            }
        } else {
            try {
                img->image->write(ftype, imgpath, comp_params);
            } catch (IIIFImageError &err) {
                lua_pop(L, lua_gettop(L));
                lua_pushboolean(L, false);
                lua_pushstring(L, err.to_string().c_str());
                return 2;
            }
        }

        lua_pop(L, lua_gettop(L));

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================


    /*!
    * SipiImage.send(img, <format>)
    */
    static int SImage_send(lua_State *L) {
        int top = lua_gettop(L);
        SImage *img = checkSImage(L, 1);

        if (!lua_isstring(L, 2)) {
            lua_pop(L, top);
            lua_pushboolean(L, false);
            lua_pushstring(L, "IIIFImage.send(): Incorrect arguments");
            return 2;
        }

        const char *ext = lua_tostring(L, 2);
        lua_pop(L, top);

        std::string extension = ext;
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::string ftype;

        if ((extension == "tif") || (extension == "tiff")) {
            ftype = "tif";
        } else if ((extension == "jpg") || (extension == "jpeg")) {
            ftype = "jpg";
        } else if (extension == "png") {
            ftype = "png";
        } else if ((extension == "j2k") || (extension == "jpx")) {
            ftype = "jpx";
        } else {
            lua_pushstring(L, "SipiImage.send(): unsupported file format");
            return lua_error(L);
        }

        lua_getglobal(L, luaconnection); // push onto stack
        Connection *conn = (Connection *) lua_touserdata(L, -1); // does not change the stack
        lua_remove(L, -1); // remove from stack

        img->image->connection(conn);

        try {
            img->image->write(ftype, "HTTP");
        } catch (IIIFImageError &err) {
            lua_pushboolean(L, false);
            lua_pushstring(L, err.to_string().c_str());
            return 2;
        }

        lua_pushboolean(L, true);
        lua_pushnil(L);

        return 2;
    }
    //=========================================================================

    // map the method names exposed to Lua to the names defined here
    static const luaL_Reg SImage_methods[] = {{"new",                  SImage_new},
                                              {"dims",                 SImage_dims}, // #myimg
                                              {"write",                SImage_write}, // myimg >> filename
                                              {"send",                 SImage_send}, // myimg
                                              {"crop",                 SImage_crop}, // myimg - "100,100,500,500"
                                              {"scale",                SImage_scale}, // myimg % "500,"
                                              {"rotate",               SImage_rotate}, // myimg * 45.0
                                              {"watermark",            SImage_watermark}, // myimg + "wm-path"
                                              {"mimetype_consistency", SImage_mimetype_consistency},
                                              {0,                      0}};
    //=========================================================================

    static int SImage_gc(lua_State *L) {
        SImage *img = toSImage(L, 1);
        delete img->image;
        delete img->filename;
        return 0;
    }
    //=========================================================================

    static int SImage_tostring(lua_State *L) {
        SImage *img = toSImage(L, 1);
        std::stringstream ss;
        ss << "File: " << *(img->filename);
        ss << *(img->image);
        lua_pushstring(L, ss.str().c_str());
        return 1;
    }
    //=========================================================================

    static const luaL_Reg SImage_meta[] = {{"__gc",       SImage_gc},
                                           {"__tostring", SImage_tostring},
                                           {0,            0}};
    //=========================================================================


    void iiif_lua_globals(lua_State *L, Connection &conn, IIIFHandler &iiif_handler) {
        //
        // filesystem functions
        //
        lua_newtable(L); // table
        luaL_setfuncs(L, cache_methods, 0);
        lua_setglobal(L, "cache");

        lua_getglobal(L, SIMAGE);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
        }
        // stack:  table(SIMAGE)

        luaL_setfuncs(L, SImage_methods, 0);
        // stack:  table(SIMAGE)

        luaL_newmetatable(L, SIMAGE); // create metatable, and add it to the Lua registry
        // stack: table(SIMAGE) - metatable

        luaL_setfuncs(L, SImage_meta, 0);
        lua_pushliteral(L, "__index");
        // stack: table(SIMAGE) - metatable - "__index"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(SIMAGE) - metatable - "__index" - table(SIMAGE)

        lua_rawset(L, -3); // metatable.__index = methods
        // stack: table(SIMAGE) - metatable

        lua_pushliteral(L, "__metatable");
        // stack: table(SIMAGE) - metatable - "__metatable"

        lua_pushvalue(L, -3); // dup methods table
        // stack: table(SIMAGE) - metatable - "__metatable" - table(SIMAGE)

        lua_rawset(L, -3);
        // stack: table(SIMAGE) - metatable

        lua_pop(L, 1); // drop metatable
        // stack: table(SIMAGE)

        lua_setglobal(L, SIMAGE);
        // stack: empty

        lua_pushlightuserdata(L, &iiif_handler);
        lua_setglobal(L, iiifhandler_token);
    }
    //=========================================================================

} // namespace Sipi
