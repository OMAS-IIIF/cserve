from inspect import getmembers

import psutil
import pytest
import subprocess
import os
import shutil
import sys
import threading
import time
import requests

from pprint import pprint


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


class CserverProcessManager:

    def __init__(self, cserver_exe, cserver_handlerdir):
        self.cserver_exe = cserver_exe
        self.cserver_config = {
            "CSERVE_HANDLERDIR": cserver_handlerdir,
            "CSERVE_PORT": "8080",
            "CSERVE_SSLPORT": "8443",
            "CSERVE_SSLCERT": './testserver/certificate/certificate.pem',
            "CSERVE_SSLKEY": './testserver/certificate/key.pem',
            "CSERVE_JWTKEY": 'UP4014, the biggest steam engine',
            "CSERVE_TMPDIR": "./testserver/tmp",
            "CSERVE_LUA_INCLUDE_PATH": "./testserver/scripts",
            "CSERVE_NTHREADS": "4",
            "CSERVE_KEEPALIVE": "5",
            "CSERVE_MAXPOSTSIZE": "1M",
            "CSERVE_LOGLEVEL": "TRACE",
            "SCRIPTHANDLER_SCRIPTDIR": "./testserver/scripts",
            "SCRIPTHANDLER_ROUTES": "GET:/servervariables:servervariables.lua;"
                                    "GET:/filefunctions:filefunctions.lua;"
                                    "GET:/misc:misc.lua;"
                                    "POST:/upload:upload.lua;"
                                    "GET:/sqlite3:sqlite3.lua",
            "FILEHANDLER_DOCROOT": "./testserver/docroot",
            "FILEHANDLER_ROUTES": "GET:/:C++;"
                                  "PUT:/:C++;"
                                  "POST:/:C++",
            "PINGHANDLER_ECHO": "PINGPONG"
        }
        self.cserver_ready = False
        self.inlines = []
        #
        # remove all files in ./tmp
        #
        for root, dirs, files in os.walk('./testserver/tmp'):
            for f in files:
                os.unlink(os.path.join(root, f))
            for d in dirs:
                shutil.rmtree(os.path.join(root, d))

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
        self.thread = threading.Thread(target = get_output)
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
            print("GAGAGAGAGAGGAGAGAGAGAGA")
        else:
            print(f"Cserver ready after {cnt*0.2} seconds")

    def stop_cserver(self) -> None:
        """
        Stop the cserver
        :return: None
        """
        self.cserver_process.terminate()

    def get_server_output(self):
        return "".join(self.inlines)

    def get(self, *args, **kwargs):
        largs = list(args)
        largs[0] = 'http://localhost:8080/' + largs[0]
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
        largs[0] = 'https://localhost:8443/' + largs[0]
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
        largs[0] = 'http://localhost:8080/' + largs[0]
        nargs = tuple(largs)
        try:
            response = requests.get(*nargs, **kwargs)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.json()

    def sget_json(self, *args, **kwargs):
        largs = list(args)
        largs[0] = 'https://localhost:8443/' + largs[0]
        nargs = tuple(largs)
        try:
            response = requests.get(*nargs, **kwargs, verify=False)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.json()

    def get_output(self):
        return "".join(self.inlines)

    def upload(self, route, filepath, mimetype):
        response = ""
        basename = os.path.basename(filepath)
        files = {'file': (basename, open(filepath, 'rb'), mimetype)}
        route = 'http://localhost:8080/' + route
        try:
            response = requests.post(route, files=files)
            response.raise_for_status()
        except:
            raise CserverTestError("POST request to {} failed: {}".format(route, response.json()["message"]))
        return response.json()

class CserverTestError(Exception):
    """Indicates an error in a Sipi test."""
    def __init__(self, *args):
        super(CserverTestError, self).__init__(args)

