/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include <memory>


#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <cstdlib>
#endif

#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Cserve.h"
#include "IIIFCache.h"
#include "IIIFError.h"

static const char file_[] = __FILE__;

namespace cserve {

    typedef struct AListEle_ {
        std::string canonical;
        time_t access_time;
        off_t fsize;

        bool operator<(const AListEle_ &str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }

        bool operator>(const AListEle_ &str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }

        bool operator==(const AListEle_ &str) const {
            return (difftime(access_time, str.access_time) == 0.);
        }
    } AListEle;


    IIIFCache::IIIFCache(const std::string &cachedir_p, long long max_cachesize_p, unsigned max_nfiles_p,
                         float cache_hysteresis_p) : _cachedir(cachedir_p), max_cachesize(max_cachesize_p),
                                                     max_nfiles(max_nfiles_p), cache_hysteresis(cache_hysteresis_p) {

        if (access(_cachedir.c_str(), R_OK | W_OK | X_OK) != 0) {
            throw IIIFError(file_, __LINE__, "Cache directory not available", errno);
        }

        std::string cachefilename = _cachedir + "/.iiifcache";
        cachesize = 0;
        nfiles = 0;

        Server::logger()->info("Cache at \"{}\" (cachesize={} nfiles={} hysteresis={})", _cachedir.c_str(), max_cachesize,
                               max_nfiles, cache_hysteresis);
        std::ifstream cachefile(cachefilename, std::ofstream::in | std::ofstream::binary);

        struct dirent **namelist;

        if (!cachefile.fail()) {
            cachefile.seekg(0, cachefile.end);
            std::streampos length = cachefile.tellg();
            cachefile.seekg(0, cachefile.beg);
            int n = length / sizeof(IIIFCache::FileCacheRecord);
            Server::logger()->info("Reading cache file...");

            for (int i = 0; i < n; i++) {
                IIIFCache::FileCacheRecord fr;
                cachefile.read((char *) &fr, sizeof(IIIFCache::FileCacheRecord));
                std::string accesspath = _cachedir + "/" + fr.cachepath;

                if (access(accesspath.c_str(), R_OK) != 0) {
                    //
                    // we cannot find the file – probably it has been deleted => skip it
                    //
                    Server::logger()->debug("Cache could'nt find file \"{}\" on disk!", fr.cachepath);
                    continue;
                }

                CacheRecord cr;
                cr.img_w = fr.img_w;
                cr.img_h = fr.img_h;

                SubImageInfo ti;
                for (int i = 0; i < fr.nresolutions; ++i) {
                    ti.reduce = fr.resolutions[sizeof(SubImageInfo)*i];
                    ti.width = fr.resolutions[sizeof(SubImageInfo)*i + 1];
                    ti.height = fr.resolutions[sizeof(SubImageInfo)*i + 2];
                    ti.tile_width = fr.resolutions[sizeof(SubImageInfo)*i + 3];
                    ti.tile_height = fr.resolutions[sizeof(SubImageInfo)*i + 4];
                    cr.resolutions.push_back(ti);
                }
                cr.origpath = fr.origpath;
                cr.cachepath = fr.cachepath;
                cr.mtime = fr.mtime;
                cr.access_time = fr.access_time;
                cr.fsize = fr.fsize;
                cachesize += fr.fsize;
                nfiles++;
                cachetable[fr.canonical] = cr;
                Server::logger()->info("File \"{}\" adding to cache", cr.cachepath);
            }
        }

        //
        // now we looking for files that are not in the list of cached files
        // and we delete them
        //
        int n = scandir(_cachedir.c_str(), &namelist, nullptr, alphasort);

        if (n < 0) {
            perror("scandir");
        } else {
            while (n--) {
                if (namelist[n]->d_name[0] == '.') continue; // files beginning with "." are not removed
                std::string file_on_disk = namelist[n]->d_name;
                bool found = false;

                for (const auto &ele : cachetable) {
                    if (ele.second.cachepath == file_on_disk) found = true;
                }

                if (!found) {
                    std::string ff = _cachedir + "/" + file_on_disk;
                    Server::logger()->info("File \"{}\" not in cache file! Deleting...", file_on_disk);
                    remove(ff.c_str());
                }

                free(namelist[n]);
            }

            free(namelist);
        }

        for (const auto &ele : cachetable) {
            try {
                (void) sizetable.at(ele.second.origpath);
            } catch (const std::out_of_range &oor) {
                IIIFCache::SizeRecord tmp_cr = {ele.second.img_w, ele.second.img_h, ele.second.resolutions, ele.second.mtime};
                sizetable[ele.second.origpath] = tmp_cr;
            }
        }
    }
    //============================================================================

    IIIFCache::~IIIFCache() {
        Server::logger()->debug("Closing cache...");
        std::string cachefilename = _cachedir + "/.iiifcache";
        std::ofstream cachefile(cachefilename, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

        if (!cachefile.fail()) {
            for (const auto &ele : cachetable) {
                IIIFCache::FileCacheRecord fr;
                fr.img_w = ele.second.img_w;
                fr.img_h = ele.second.img_h;
                fr.nresolutions = ele.second.resolutions.size();
                for (int i = 0; i < fr.nresolutions; ++i) {
                    fr.resolutions[sizeof(SubImageInfo)*i] = ele.second.resolutions[i].reduce;
                    fr.resolutions[3*i + 1] = ele.second.resolutions[i].width;
                    fr.resolutions[3*i + 2] = ele.second.resolutions[i].height;
                    fr.resolutions[3*i + 3] = ele.second.resolutions[i].tile_width;
                    fr.resolutions[3*i + 4] = ele.second.resolutions[i].tile_height;
                }
                (void) snprintf(fr.canonical, 256, "%s", ele.first.c_str());
                (void) snprintf(fr.origpath, 256, "%s", ele.second.origpath.c_str());
                (void) snprintf(fr.cachepath, 256, "%s", ele.second.cachepath.c_str());
                fr.mtime = ele.second.mtime;
                fr.fsize = ele.second.fsize;
                fr.access_time = ele.second.access_time;
                cachefile.write((char *) &fr, sizeof(IIIFCache::FileCacheRecord));
                Server::logger()->debug("Writing \"{}\" to cache file...", ele.second.cachepath);
            }
        }
        cachefile.close();
    }
    //============================================================================

#if defined(HAVE_ST_ATIMESPEC)

    int IIIFCache::tcompare(struct timespec &t1, struct timespec &t2)
#else
    int IIIFCache::tcompare(time_t &t1, time_t &t2)
#endif
    {
#if defined(HAVE_ST_ATIMESPEC)
        if (t1.tv_sec > t2.tv_sec) {
            return 1;
        } else if (t1.tv_sec < t2.tv_sec) {
            return -1;
        } else {
            if (t1.tv_sec == t2.tv_sec) {
                if (t1.tv_nsec > t2.tv_nsec) {
                    return 1;
                } else if (t1.tv_nsec < t2.tv_nsec) {
                    return -1;
                } else {
                    return 0;
                }
            }
        }
#else
        if (t1 > t2) {
            return 1;
        }
        else if (t1 < t2) {
            return -1;
        }
        else {
            return 0;
        }

#endif
        return 0; // should never been reached – just for the compiler to suppress a warning...
    }
    //============================================================================

    static bool _compare_access_time_asc(const AListEle &e1, const AListEle &e2) {
        double d = difftime(e1.access_time, e2.access_time);
        return (d < 0.0);
    }
    //============================================================================

    static bool _compare_access_time_desc(const AListEle &e1, const AListEle &e2) {
        double d = difftime(e1.access_time, e2.access_time);
        return (d > 0.0);
    }
    //============================================================================

    static bool _compare_fsize_asc(const AListEle &e1, const AListEle &e2) {
        return (e1.fsize < e2.fsize);
    }
    //============================================================================

    static bool _compare_fsize_desc(const AListEle &e1, const AListEle &e2) {
        return (e1.fsize > e2.fsize);
    }
    //============================================================================

    int IIIFCache::purge(bool use_lock) {
        if ((max_cachesize == 0) && (max_nfiles == 0)) return 0; // allow cache to grow indefinitely! dangerous!!
        int n = 0;

        if (((max_cachesize > 0) && (cachesize >= max_cachesize)) || ((max_nfiles > 0) && (nfiles >= max_nfiles))) {
            std::vector<AListEle> alist;
            std::unique_lock<std::mutex> locking_mutex_guard(locking, std::defer_lock);
            if (use_lock) locking_mutex_guard.lock();

            for (const auto &ele : cachetable) {
                AListEle al = {ele.first, ele.second.access_time, ele.second.fsize};
                alist.push_back(al);
            }

            sort(alist.begin(), alist.end(), _compare_access_time_asc);

            long long cachesize_goal = max_cachesize * cache_hysteresis;
            int nfiles_goal = max_nfiles * cache_hysteresis;

            for (const auto &ele : alist) {
                Server::logger()->debug("Purging from cache \"{}\"...", cachetable[ele.canonical].cachepath);
                std::string delpath = _cachedir + "/" + cachetable[ele.canonical].cachepath;

                try {
                    int cnt = blocked_files.at(delpath);
                    Server::logger()->warn("Couldn't remove cache file for {}: file in use ({})!", ele.canonical, cnt);
                } catch (const std::out_of_range &ex) {
                    ::unlink(delpath.c_str());
                    cachesize -= cachetable[ele.canonical].fsize;
                    --nfiles;
                    ++n;
                    (void) cachetable.erase(ele.canonical);
                }
                if ((max_cachesize > 0) && (cachesize < cachesize_goal)) break;
                if ((max_nfiles > 0) && (nfiles < nfiles_goal)) break;
            }
        }

        return n;
    }
    //============================================================================

    std::string IIIFCache::check(const std::string &origpath_p, const std::string &canonical_p, bool block_file) {
        struct stat fileinfo;
        IIIFCache::CacheRecord fr;

        if (stat(origpath_p.c_str(), &fileinfo) != 0) {
            throw IIIFError(file_, __LINE__, "Couldn't stat file \"" + origpath_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        struct timespec mtime = fileinfo.st_mtimespec;
#else
        time_t mtime = fileinfo.st_mtime;
#endif

        std::string res;

        std::lock_guard<std::mutex> locking_mutex_guard(locking);
        try {
            fr = cachetable.at(canonical_p);
        } catch (const std::out_of_range &oor) {
            return res; // return empty string, because we didn't find the file in cache
        }

        //
        // get the current time (seconds since Epoch)
        //
        time_t at;
        time(&at);
        cachetable[canonical_p].access_time = at;// update the access time!

        if (tcompare(mtime, fr.mtime) > 0) { // original file is newer than cache, we have to replace it...
            return res; // return empty string, means "replace the file in the cache!"
        } else {
            res = _cachedir + "/" + fr.cachepath;
            if (block_file) {
                blocked_files[res]++;
            }
            return res;
        }
    }
    //============================================================================

    void IIIFCache::deblock(const std::string &res) {
        std::lock_guard<std::mutex> locking_mutex_guard(locking);
        blocked_files[res]--;
        if (blocked_files[res] < 1) {
            blocked_files.erase(res);
        }
    }

    /*!
     * Creates a new cache file with a unique name.
     *
     * \return the name of the file.
     */
    std::string IIIFCache::getNewCacheFileName(void) {
        std::string filename = _cachedir + "/cache_XXXXXXXXXX";
        char *c_filename = &filename[0];
        int tmp_fd = mkstemp(c_filename);

        if (tmp_fd == -1) {
            throw IIIFError(file_, __LINE__, std::string("Couldn't create cache file ") + filename, errno);
        }

        close(tmp_fd);
        return filename;
    }
    //============================================================================

    void IIIFCache::add(
            const std::string &origpath_p,
            const std::string &canonical_p,
            const std::string &cachepath_p,
            size_t img_w_p,
            size_t img_h_p,
            const std::vector<SubImageInfo> &resolutions) {
        size_t pos = cachepath_p.rfind('/');
        std::string cachepath;

        if (pos != std::string::npos) {
            cachepath = cachepath_p.substr(pos + 1);
        } else {
            cachepath = cachepath_p;
        }

        struct stat fileinfo{};
        IIIFCache::CacheRecord fr;
        IIIFCache::SizeRecord sr;

        fr.img_w = sr.img_w = img_w_p;
        fr.img_h = sr.img_h = img_h_p;
        fr.resolutions = resolutions;
        sr.resolutions = resolutions;
        fr.origpath = origpath_p;
        fr.cachepath = cachepath;

        if (stat(cachepath_p.c_str(), &fileinfo) != 0) {
            throw IIIFError(file_, __LINE__, "Couldn't stat file \"" + origpath_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        fr.mtime = fileinfo.st_mtimespec;
#else
        fr.mtime = fileinfo.st_mtime;
#endif

        //
        // get the current time (seconds since Epoch)
        //
        time_t at;
        time(&at);
        fr.access_time = at;
        fr.fsize = fileinfo.st_size;

        //
        // we check if there is already a file with the same canonical name. If so,
        // we remove it
        //
        std::lock_guard<std::mutex> locking_mutex_guard(locking);
        try {
            IIIFCache::CacheRecord tmp_fr = cachetable.at(canonical_p);
            std::string toremove = _cachedir + "/" + tmp_fr.cachepath;
            ::unlink(toremove.c_str());
            cachesize -= tmp_fr.fsize;
            --nfiles;
        } catch (const std::out_of_range &oor) {
            ; // do nothing...
        }

        purge(false);

        cachetable[canonical_p] = fr;
        cachesize += fr.fsize;

        IIIFCache::SizeRecord tmp_cr = {img_w_p, img_h_p, resolutions};
        sizetable[origpath_p] = tmp_cr;

        ++nfiles;
    }
    //============================================================================

    bool IIIFCache::remove(const std::string &canonical_p) {
        IIIFCache::CacheRecord fr;
        std::lock_guard<std::mutex> locking_mutex_guard(locking);

        std::string delpath = _cachedir + "/" + cachetable[canonical_p].cachepath;
        try {
            int cnt = blocked_files.at(delpath);
            Server::logger()->warn("Couldn't remove cache for \"{}\": file in use ({})!", canonical_p, cnt);
            return false;
        } catch (const std::out_of_range &ex) {
            ;
        }
        Server::logger()->debug("Delete from cache \"{}\"...", cachetable[canonical_p].cachepath);
        ::remove(delpath.c_str());
        cachesize -= cachetable[canonical_p].fsize;
        cachetable.erase(canonical_p);
        --nfiles;

        return true;
    }
    //============================================================================

    void IIIFCache::loop(ProcessOneCacheFile worker, void *userdata, SortMethod sm) {
        std::vector<AListEle> alist;

        for (const auto &ele : cachetable) {
            AListEle al = {ele.first, ele.second.access_time, ele.second.fsize};
            alist.push_back(al);
        }

        switch (sm) {
            case SORT_ATIME_ASC: {
                sort(alist.begin(), alist.end(), _compare_access_time_asc);
                break;
            }

            case SORT_ATIME_DESC: {
                sort(alist.begin(), alist.end(), _compare_access_time_desc);
                break;
            }

            case SORT_FSIZE_ASC: {
                sort(alist.begin(), alist.end(), _compare_fsize_asc);
                break;
            }

            case SORT_FSIZE_DESC: {
                sort(alist.begin(), alist.end(), _compare_fsize_desc);
                break;
            }
        }

        int i = 1;

        for (const auto &ele : alist) {
            worker(i, ele.canonical, cachetable[ele.canonical], userdata);
            i++;
        }
    }
    //============================================================================

    bool IIIFCache::getSize(
            const std::string &origname_p,
            size_t &img_w,
            size_t &img_h,
            std::vector<SubImageInfo> &resolutions) {
        struct stat fileinfo{};
        if (stat(origname_p.c_str(), &fileinfo) != 0) {
            throw IIIFError(file_, __LINE__, "Couldn't stat file \"" + origname_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        struct timespec mtime = fileinfo.st_mtimespec;
#else
        time_t mtime = fileinfo.st_mtime;
#endif

        try {
            IIIFCache::SizeRecord sr = sizetable.at(origname_p);
            if (tcompare(mtime, sr.mtime) > 0) { // original file is newer than cache, we have to replace it..
                std::lock_guard<std::mutex> locking_mutex_guard(locking);

                sizetable.erase(origname_p);
                return false; // means "replace the file in the cache"
            }

            img_w = sr.img_w;
            img_h = sr.img_h;
            resolutions = sr.resolutions;
        } catch (const std::out_of_range &oor) {
            return false;
        }

        return true;
    }
    //============================================================================
}
