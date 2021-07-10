import psutil
import pytest
import subprocess
import os
import sys
import threading
import time
import requests

from pprint import pprint


def pytest_addoption(parser):
    parser.addoption(
        "--cserver", action="store", default="notset", help="The absolut path to the cserver executable"
    )


@pytest.fixture(scope="session")
def manager(request):
    cserver_exe = request.config.getoption('--cserver')
    manager = CserverProcessManager(cserver_exe)
    manager.start_cserver()
    yield manager
    manager.stop_cserver()


class CserverProcessManager:

    def __init__(self, cserver_exe):
        self.cserver_exe = cserver_exe
        self.cserver_config = {
            "CSERVER_PORT": "8080",
            "CSERVER_SSLPORT": "8443",
            "CSERVER_SSLCERTIFICATE": './testserver/certificate/certificate.pem',
            "CSERVER_SSLKEY": './testserver/certificate/key.pem',
            "CSERVER_JWTKEY": 'UP4014, the biggest steam engine',
            "CSERVER_DOCROOT": "./testserver/docroot",
            "CSERVER_FILEHANDLER_ROUTE": '/',
            "CSERVER_TMPDIR": "./testserver/tmp",
            "CSERVER_SCRIPTDIR": "./testserver/scripts",
            "CSERVER_NTHREADS": "4",
            "CSERVER_KEEPALIVE": "5",
            "CSERVER_MAXPOSTSIZE": "1M",
            "CSERVER_LOGLEVEL": "TRACE",
            "CSERVER_ROUTES": "GET:/servervariables:servervariables.lua"
        }
        self.cserver_ready = False
        self.inlines = []

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
        print("===> stop_cserver")
        self.cserver_process.terminate()

    def get_server_output(self):
        return "".join(self.inlines)

    def get_text(self, *args, **kwargs):
        largs = list(args)
        largs[0] = 'http://localhost:8080/' + largs[0]
        nargs = tuple(largs)

        response = ""
        try:
            response = requests.get(*nargs, **kwargs)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.text

    def sget_text(self, *args, **kwargs):
        largs = list(args)
        largs[0] = 'https://localhost:8443/' + largs[0]
        nargs = tuple(largs)

        response = ""
        try:
            response = requests.get(*nargs, **kwargs, verify=False)
            response.raise_for_status()
        except:
            raise CserverTestError("GET request to {} failed: {}".format(nargs[0], response.json()["message"]))
        return response.text

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

class CserverTestError(Exception):
    """Indicates an error in a Sipi test."""
    def __init__(self, *args):
        super(CserverTestError, self).__init__(args)

