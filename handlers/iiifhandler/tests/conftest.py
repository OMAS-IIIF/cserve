import filecmp
import shlex
import tempfile
from inspect import getmembers
from typing import Any, Dict

import psutil
import pytest
import subprocess
import os
import re
import shutil
import sys
import threading
import time
import requests
import glob

from PIL import Image, ImageChops

from pprint import pprint

from requests import Response


def pytest_addoption(parser):
    parser.addoption(
        "--cserver", action="store", default="notset", help="The absolut path to the cserver executable"
    )
    parser.addoption(
        "--handlerdir", action="store", default="notset", help="The absolut path to the handler directory"
    )



@pytest.fixture(scope="session")
def manager(request):
    cserver_exe = request.config.getoption('--cserver')
    cserver_handlerdir = request.config.getoption('--handlerdir')
    manager = CserverProcessManager(cserver_exe, cserver_handlerdir)
    manager.start_cserver()
    yield manager
    manager.stop_cserver()
    manager.cleanup()


class CserverProcessManager:
    iiif_port: str
    iiif_ssl_port: str
    iiif_url: str
    iiif_surl: str
    iiif_imgroot: str
    iiif_route: str
    iiif_validator_command: str

    def __init__(self, cserver_exe: str, cserver_handlerdir: str):
        """
        Initialize the test environment

        :param cserver_exe: Path to the executable
        :param cserver_handlerdir: Path to the directory containing all the handler
        """
        self.iiif_port = "8080"
        self.iiif_ssl_port = "8443"
        self.iiif_url = 'http://localhost:{}'.format(self.iiif_port)
        self.iiif_surl = 'https://localhost:{}'.format(self.iiif_ssl_port)
        self.iiif_imgroot = "./iiiftestserver/imgroot"
        self.iiif_route = "iiif"
        self.iiif_validator_command = "iiif-validate.py -s localhost:{} -p {} -i 67352ccc-d1b0-11e1-89ae-279075081939.jp2 --version=3.0 -v".format(
            self.iiif_port, self.iiif_route)
        self.cserver_logfile = os.path.abspath("cserver.log")
        self.cserver_exe = cserver_exe
        self.cserver_config = {
            "CSERVE_HANDLERDIR": cserver_handlerdir,
            "CSERVE_PORT": self.iiif_port,
            "CSERVE_SSLPORT": self.iiif_ssl_port,
            "CSERVE_INITSCRIPT": "./iiiftestserver/config/iiif.init.lua",
            "CSERVE_SSLCERT": './iiiftestserver/certificate/certificate.pem',
            "CSERVE_SSLKEY": './iiiftestserver/certificate/key.pem',
            "CSERVE_JWTKEY": 'UP4014, the biggest steam engine',
            "CSERVE_TMPDIR": "./iiiftestserver/tmp",
            "CSERVE_LUA_INCLUDE_PATH": "./iiiftestserver/scripts",
            "CSERVE_NTHREADS": "4",
            "CSERVE_KEEPALIVE": "5",
            "CSERVE_MAXPOST": "12M",
            "CSERVE_LOGLEVEL": "TRACE",
            "SCRIPTHANDLER_SCRIPTDIR": "./iiiftestserver/scripts",
            "SCRIPTHANDLER_ROUTES": "GET:/misc:misc.lua;",
            "FILEHANDLER_DOCROOT": "./iiiftestserver/docroot",
            "FILEHANDLER_ROUTES": "GET:/fileserv:C++;"
                                  "PUT:/fileserv:C++;"
                                  "POST:/fileserv:C++",
            "PINGHANDLER_ECHO": "PINGPONG",
            "IIIFHANDLER_IMGROOT": self.iiif_imgroot,
            "IIIFHANDLER_ROUTES": "GET:/{}:/C++;"
                                  "GET:/iiifhandlervariables:iiifhandlervariables.lua;"
                                  "GET:/test_exif_gps:test_exif_gps.lua;"
                                  "POST:/upload:upload.lua;".format(self.iiif_route),
            "IIIFHANDLER_PREFIX_AS_PATH": "true",
            "IIIFHANDLER_IIIF_SPECIALS": "test=call_lua_testit"
        }
        self.compare_command = "compare -metric {} {} {} null:"
        self.compare_out_re = re.compile(r"^(\d+) \(([0-9.]+)\).*$")
        self.cserver_ready = False
        self.inlines = []
        #
        # remove all files in ./tmp
        #
        for root, dirs, files in os.walk('./iiiftestserver/tmp'):
            for f in files:
                os.unlink(os.path.join(root, f))
            for d in dirs:
                shutil.rmtree(os.path.join(root, d))

    def cleanup(self):
        """Cleanup files generated by the tests"""
        fileList = glob.glob(self.iiif_imgroot + '/_*')
        for filePath in fileList:
            os.remove(filePath)

    def start_cserver(self):
        #
        # First we stop any existing Cserver process. This could happen if a previous test run crashed.
        #
        for proc in psutil.process_iter():
            try:
                if proc.name() == "cserver":
                    proc.terminate()
                    proc.wait()
            except psutil.NoSuchProcess:
                pass

        # Remove any cserver log file from a previous run.
        try:
            os.remove(self.cserver_logfile)
        except OSError:
            pass

        #
        # let's start the subprocess with the cserver
        #
        self.cserver_process = subprocess.Popen(self.cserver_exe,
                                                env=self.cserver_config,
                                                stdout=subprocess.PIPE,
                                                stderr=subprocess.STDOUT,
                                                universal_newlines=True)

        def get_output() -> None:
            """
            This local function is used to collect the output from the subprocess.
            :return: None
            """
            while True:
                line = self.cserver_process.stdout.readline().strip(" \n\r")
                if line:
                    print(line)
                    if "Cserver ready" in line:
                        self.cserver_ready = True
                    self.inlines.append(line)
                else:
                    return
        #
        # start the thread collecting the output of the subprocess
        #
        self.thread = threading.Thread(target=get_output)
        self.thread.daemon = True
        self.thread.start()
        #
        # wait for Cserver to be ready...
        #
        cnt = 1
        while not self.cserver_ready and cnt < 10:
            time.sleep(0.2)
            cnt += 1
        if cnt >= 10:
            raise CserverTestError(f"Cserver did not start after {cnt*0.2} seconds")
        else:
            print(f"Cserver ready after {cnt*0.2} seconds")

    def stop_cserver(self) -> None:
        """
        Stop the cserver
        :return: None
        """
        self.cserver_process.terminate()
        self.cserver_ready = False
        self.write_cserver_log()

    def get_server_output(self) -> str:
        return "".join(self.inlines)

    def iiif_imgroot_path(self, relative_path: str) -> str:
        """
            Converts a path relative to data-dir into an absolute path.
        """
        return os.path.join(self.iiif_imgroot, relative_path)

    def get_raw(self, *args, **kwargs) -> Response:
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)
        response = requests.get(*nargs, **kwargs)
        return response

    def get(self, *args, **kwargs):
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)

        response = ""
        try:
            response = requests.get(*nargs, **kwargs)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response

    def sget(self, *args, **kwargs):
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)

        response = ""
        try:
            response = requests.get(*nargs, **kwargs, verify=False)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response

    def get_json(self, *args, **kwargs):
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)
        try:
            response = requests.get(*nargs, **kwargs)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.json()

    def sget_json(self, *args, **kwargs):
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)
        try:
            response = requests.get(*nargs, **kwargs, verify=False)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.json()

    def get_status_code(self, *args, **kwargs) -> int:
        largs = list(args)
        largs[0] = "/".join([self.iiif_url, self.iiif_route, largs[0]])
        nargs = tuple(largs)

        response = requests.get(*nargs, **kwargs)
        return response.status_code

    def get_route(self,  route: str, headers: Dict[str,str] = None):
        route = 'http://localhost:8080/' + route
        response = requests.get(route, headers=headers)
        return response.status_code

    def get_route_json(self,  route: str, headers: Dict[str,str] = None):
        route = 'http://localhost:8080/' + route
        try:
            response = requests.get(route, headers=headers)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(route, response.json()["message"]))
        return response.json()

    def download_file(self, iiifpath: str, suffix: str = None, headers=None) -> str:
        url = "/".join([self.iiif_url, self.iiif_route, iiifpath])
        response = requests.get(url, headers=headers, stream=True)
        response.raise_for_status()
        temp_fd, temp_file_path = tempfile.mkstemp(suffix=suffix)
        temp_file = os.fdopen(temp_fd, mode="wb")

        for chunk in response.iter_content(chunk_size=8192):
            temp_file.write(chunk)

        temp_file.close()
        return temp_file_path

    def compare_iiif_bytes(self, iiifpath: str, referencepath: str, headers=None) -> bool:
        expected_file_basename, expected_file_extension = os.path.splitext(referencepath)
        downloaded_file_path = self.download_file(iiifpath, headers=headers, suffix=expected_file_extension)
        if filecmp.cmp(downloaded_file_path, referencepath):
            os.remove(downloaded_file_path)
            return True
        else:
            return False

    def compare_iiif_images(self, iiifpath: str, referencepath: str, headers=None) -> bool:
        expected_file_basename, expected_file_extension = os.path.splitext(referencepath)
        downloaded_file_path = self.download_file(iiifpath, headers=headers, suffix=expected_file_extension)

        im1 = Image.open(referencepath)
        im2 = Image.open(downloaded_file_path)
        diff = ImageChops.difference(im1, im2)
        if diff.getbbox():
            return False
        else:
            return True
        # compare_process_args = shlex.split(
        #     self.compare_command.format(metric, referencepath, downloaded_file_path))
        # compare_process = subprocess.run(compare_process_args,
        #                                  stdout=subprocess.PIPE,
        #                                  stderr=subprocess.STDOUT,
        #                                  universal_newlines=True)
        #
        #
        # compare_out_str = compare_process.stdout
        # compare_out_regex_match = self.compare_out_re.match(compare_out_str)
        # assert compare_out_regex_match is not None, "Couldn't parse comparison result: {}".format(compare_out_str)
        # return int(compare_out_regex_match.group(1))


    def upload(self, route: str, filepath: str, mimetype: str, data: Dict = None) -> Any:
        response = {}
        basename = os.path.basename(filepath)
        files = {'file': (basename, open(filepath, 'rb'), mimetype)}
        route = 'http://localhost:8080/' + route
        try:
            response = requests.post(route, data=data, files=files)
            response.raise_for_status()
        except:
            raise CserverTestError("POST request to {} failed: {}".format(route, response.json()["message"]))
        return response.json()

    def get_image_info(self, url_path, headers=None):
        """
        Downloads a temporary image file, gets information about it using ImageMagick's 'identify'
        program with the '-verbose' option, and returns the resulting output.

        url_path: a path that will be appended to the Sipi base URL to make the request.
        headers: an optional dictionary of request headers.
        """

        downloaded_file_path = self.download_file(url_path, headers=headers)
        info_process_args = shlex.split("identify -verbose {}".format(downloaded_file_path))
        info_process = subprocess.run(info_process_args,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      universal_newlines=True)
        return info_process.stdout

    def run_iiif_validator(self):
        """Runs the IIIF validator. If validation fails, writes IIFHandler's output to cserve.log and raises an exception."""

        validator_process_args = shlex.split(self.iiif_validator_command)
        validator_process = subprocess.run(validator_process_args,
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT,
                                           universal_newlines=True)

        if validator_process.returncode != 0:
            raise CserverTestError(
                "IIIF validation failed:\n{}".format(validator_process.stdout))


    def write_cserver_log(self) -> None:
        """Writes cserver output to a log file."""
        with open(self.cserver_logfile, "w") as file:
            file.write(self.get_server_output())

class CserverTestError(Exception):
    """Indicates an error in a Sipi test."""
    def __init__(self, *args):
        super(CserverTestError, self).__init__(args)

def pytest_itemcollected(item):
    """Outputs test class and function docstrings, if provided, when each test is run."""

    par = item.parent.obj
    node = item.obj
    pref = par.__doc__.strip() if par.__doc__ else par.__class__.__name__
    component = par.component
    suf = node.__doc__.strip() if node.__doc__ else node.__name__
    if pref or suf:
        item._nodeid = "TEST: {}: {} should {}\n".format(pref, component, suf)
