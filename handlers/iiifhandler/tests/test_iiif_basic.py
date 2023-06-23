import shutil

import pytest
import os
import pprint

class TestBasic:
    component = "The IIIF server"

    def test_cserver_running(self, manager):
        """test if cserver is running...."""
        assert manager.cserver_ready

    def test_iiif_handler_variables(self, manager):
        """return the IIIF handler variables"""
        expected_result = {
            'iiifhandler': {
                'cache_hysteresis': 0.15000000596046448,
                'cachedir': './cache',
                'cachesize': '2e+02MB',
                'file_preflight_name': 'file_preflight',
                'iiif_preflight_name': 'iiif_preflight',
                'imgroot': './iiiftestserver/imgroot',
                'max_num_cache_files': 200,
                'max_tmp_age': 86400,
                'prefix_as_path': True,
                'thumbsize': '!128,128',
                'iiif_specials': {'testit': 'lua_testit'}
            },
            'status': 'OK'
        }

        result = manager.get_route_json("iiifhandlervariables")
        assert result == expected_result

    def test_iiif_parsing(self, manager):
        """return 400 for invalid IIIF URL's"""
        assert manager.get_status_code("iiif//lena512.jp2") == 400  # TODO!!!!
        assert manager.get_status_code("iiif/lena512.jp2/max/0/default.jpg") == 400
        assert manager.get_status_code("iiif/lena512.jp2/full/max/default.jpg") == 400
        assert manager.get_status_code("iiif/lena512.jp2/full/max/!/default.jpg") == 400
        # The following URL is valid IIIF V3 syntax:
        # "iiif": the route
        # "/lena512.jp2/full/max/0/": the prefixes
        # "/jpg": the ID
        # This URL will be redirected to "iiif/lena512.jp2/full/max/0/jpg/info.json"
        assert manager.get_status_code("iiif/lena512.jp2/full/max/0/jpg") == 400  # this is a valid URL, but of course no file

    def test_get_iiif(self, manager):
        """get an image file using IIIF syntax"""
        response = manager.get('test_01.tif/full/max/0/default.jpg')
        assert response.status_code == 200

    def test_get_iiif_subdir(self, manager):
        """get an image file from a subdir using IIIF syntax"""
        response = manager.get('subdir/IMG_9144.jp2/full/max/0/default.jpg')
        assert response.status_code == 200

    def test_iiif_bytes(self, manager):
        """return an unmodified JPG file"""
        assert manager.compare_iiif_bytes("Leaves.jpg/full/max/0/default.jpg",
                                          manager.iiif_imgroot_path("Leaves.jpg"))

    def test_iiif_info(self, manager):
        """return info.json"""
        expected_response = {
            '@context': 'http://iiif.io/api/image/3/context.json',
            'extraFeatures': [
                'baseUriRedirect',
                'canonicalLinkHeader',
                'cors',
                'jsonldMediaType',
                'mirroring',
                'profileLinkHeader',
                'regionByPct',
                'regionByPx',
                'regionSquare',
                'rotationArbitrary',
                'rotationBy90s',
                'sizeByConfinedWh',
                'sizeByH',
                'sizeByPct',
                'sizeByW',
                'sizeByWh',
                'sizeUpscaling'
            ],
            'extraFormats': ['tif', 'jp2'],
            'extraQualities': ['color', 'gray', 'bitonal'],
            'height': 800,
            'id': 'http://localhost:8080/iiif/test_01.tif',
            'preferredFormats': ['jpg', 'tif', 'jp2', 'png'],
            'profile': 'level2',
            'protocol': 'http://iiif.io/api/image',
            'type': 'ImageService3',
            'width': 1200
        }
        response = manager.get("test_01.tif")
        assert response.json() == expected_response

    def test_iiif_specials(self, manager):
        expected_response = {
            "status": "OK",
            "result": {
                "cookie": "",
                "identifier": "test_01.tif",
                "prefix": "",
                "status": "ok",
                "test": {
                    "a": "a",
                    "b": "b",
                    "pi": 3.141590118408203
                },
                "zz": ["XXX", "YYY", 3.141590118408203, 42]
            }
        }
        response = manager.get("test_01.tif/testit")
        assert response.json() == expected_response


    def test_iiif_restricted(self, manager):
        """return restricted size"""
        image_info = manager.get_image_info("RestrictLeaves.jpg/full/max/0/default.jpg")

        page_geometry = [line.strip().split()[-1] for line in image_info.splitlines()
                         if line.strip().startswith("Page geometry:")][0]
        assert page_geometry == "128x128+0+0"

    def test_iiif_deny(self, manager):
        """return status 401"""
        status_code = manager.get_status_code('DenyLeaves.jpg/full/max/0/default.jpg')
        assert status_code == 401

    def test_file_allow(self, manager):
        """get file from IIIF directory"""
        iiifurl = "iiif/{}/file".format("_test.csv")
        assert manager.compare_iiif_bytes("test_allow.csv/file", manager.iiif_imgroot_path("test_allow.csv"))

    def test_file_deny(self, manager):
        """deny get file from IIIF directory"""
        status_code = manager.get_status_code('test_deny.csv/file')
        assert status_code == 401

    def test_iiif_upload_tiff(self, manager):
        """upload of tiff image and conversion to JPX and back"""
        expected_response = {
            'cnt': 1,
            'files': [
                {'consistency': True,
                 'filename': '_IMG_8207.jp2',
                 'filesize': 3275308,
                 'mimetype': 'image/tiff',
                 'origname': 'IMG_8207.tiff'}
            ],
            'status': 'OK'
        }
        response = manager.upload("upload", "./data/IMG_8207.tiff", "image/tiff")
        assert response == expected_response
        assert manager.compare_iiif_images("_IMG_8207.jp2/full/max/0/default.tif", "./data/IMG_8207.tiff")

    def test_iiif_upload_file(self, manager):
        """upload a non-image file"""
        expected_response = {
            'cnt': 1,
            'files': [
                {
                    'consistency': True,
                    'filename': '_test.csv',
                    'filesize': 39707,
                    'mimetype': 'text/csv',
                    'origname': 'test.csv'
                 }
            ],
            'status': 'OK'
        }
        response = manager.upload('upload', "./data/test.csv", "text/csv")
        assert response == expected_response
        assert manager.compare_iiif_bytes("_test.csv/file", "./data/test.csv")

    def test_orientation_topleft(self, manager):
        """rectifying the image orientation to topleft"""
        response = manager.upload("upload", "data/image_orientation.jpg", "image/jpeg")
        filename = response["files"][0]["filename"]
        assert manager.compare_iiif_images("{}/full/max/0/default.tif".format(filename), "./data/image_orientation.tif")

    def test_4bit_palette_png(self, manager):
        """Test reading 4 bit palette png with alpha channel"""
        response = manager.upload("upload", "data/mario.png", "image/png")
        filename = response["files"][0]["filename"]
        assert manager.compare_iiif_images("{}/full/max/0/default.tif".format(filename), "./data/mario.tif")

    def test_upscaling(self, manager):
        assert manager.compare_iiif_images("astronomical_clock.jp2/full/^1000,/0/default.tif", "data/astronomical_clock_upscaled.tif")

    def test_exif_gps(self, manager):
        expected_result = {
            'SubjectDistance': '--undefined--',
            'RecommendedExposureIndex': '--undefined--',
            'BrightnessValue': [117307, 10524],
            'Flash': 16,
            'OffsetTimeOriginal': '+01:00',
            'GainControl': '--undefined--',
            'SensitivityType': '--undefined--',
            'Artist': '--undefined--',
            'ExposureBiasValue': [0, 1],
            'ModifyDate': '--undefined--',
            'Contrast': '--undefined--',
            'ApertureValue': [126503, 50079],
            'Temperature': '--undefined--',
            'TileLength': 512,
            'CameraLabel': '--undefined--',
            'MaxApertureValue': '--undefined--',
            'ISOSpeedLatitudeyyy': '--undefined--',
            'ImageHistory': '--undefined--',
            'OffsetTimeDigitized': '+01:00',
            'YResolution': [72, 1],
            'SpectralSensitivity': '--undefined--',
            'Orientation': 1,
            'XResolution': [72, 1],
            'PageName': '--undefined--',
            'Acceleration': '--undefined--',
            'TileWidth': 512,
            'ISOSpeed': '--undefined--',
            'LightSource': '--undefined--',
            'LensMake': 'Apple',
            'Model': 'iPhone 12 Pro',
            'DateTime': '2022:12:11 13:02:51',
            'ImageUniqueID': '--undefined--',
            'ImageNumber': '--undefined--',
            'FocalPlaneResolutionUnit': '--undefined--',
            'FlashEnergy': '--undefined--',
            'SubSecTime': '839',
            'Software': '16.1.2',
            'CameraSerialNumber': '--undefined--',
            'WaterDepth': '--undefined--',
            'Humidity': '--undefined--',
            'DateTimeDigitized': '2022:12:11 13:02:51',
            'LensSerialNumber': '--undefined--',
            'ResolutionUnit': 2,
            'DateTimeOriginal': '2022:12:11 13:02:51',
            'LensModel': 'iPhone 12 Pro back triple camera 1.54mm f/2.4',
            'FocalLength': [77, 50],
            'OwnerName': '--undefined--',
            'SubjectDistanceRange': '--undefined--',
            'FNumber': [12, 5],
            'DocumentName': '--undefined--',
            'StandardOutputSensitivity': '--undefined--',
            'ISOSpeedLatitudezzz': '--undefined--',
            'ExposureProgram': 2,
            'UniqueCameraModel': '--undefined--',
            'SubSecTimeDigitized': '839',
            'LensInfo': '--undefined--',
            'ExposureTime': [1, 3984],
            'Sharpness': '--undefined--',
            'Saturation': '--undefined--',
            'SceneCaptureType': 0,
            'FocalPlaneXResolution': '--undefined--',
            'FocalPlaneYResolution': '--undefined--',
            'RelatedSoundFile': '--undefined--',
            'Make': 'Apple',
            'CameraElevationAngle': '--undefined--',
            'Pressure': '--undefined--',
            'PageNumber': '--undefined--',
            'HostComputer': 'iPhone 12 Pro',
            'SerialNumber': '--undefined--',
            'SubSecTimeOriginal': '839',
            'ImageDescription': '--undefined--',
            'UserComment': '--undefined--',
            'ISOSpeedRatings': 32,
            'ShutterSpeedValue': [229477, 19187],
            'MeteringMode': 5,
            'Copyright': '--undefined--',
            'BatteryLevel': '--undefined--',
            'OffsetTime': '+01:00',
            'ImageID': '--undefined--',
            'XPosition': '--undefined--',
            'YPosition': '--undefined--'}

        response_json = manager.get_route_json("test_exif_gps")
        assert response_json == expected_result

    def test_iiif_auth_01(self, manager):
        expected_response = {
            '@context': 'http://iiif.io/api/image/3/context.json',
            'extraFeatures': [
                'baseUriRedirect',
                'canonicalLinkHeader',
                'cors',
                'jsonldMediaType',
                'mirroring',
                'profileLinkHeader',
                'regionByPct',
                'regionByPx',
                'regionSquare',
                'rotationArbitrary',
                'rotationBy90s',
                'sizeByConfinedWh',
                'sizeByH',
                'sizeByPct',
                'sizeByW',
                'sizeByWh',
                'sizeUpscaling'
            ],
            'extraFormats': ['tif', 'jp2'],
            'extraQualities': ['color', 'gray', 'bitonal'],
            'height': 800,
            'id': 'http://localhost:8080/iiif/auth/test_01.tif',
            'preferredFormats': ['jpg', 'tif', 'jp2', 'png'],
            'profile': 'level2',
            'protocol': 'http://iiif.io/api/image',
            'service': {
                '@context': 'http://iiif.io/api/auth/1/context.json',
                '@id': 'https://localhost/iiif-cookie.html',
                'confirmLabel': 'Login to OMAS IIIF',
                'description': 'This Example requires a demo login!',
                'failureDescription': '<a href="http://example.org/policy">Access Policy</a>',
                'failureHeader': 'Authentication Failed',
                'header': 'Please Log In',
                'label': 'Login to OMAS IIIF',
                'profile': 'http://iiif.io/api/auth/1/login',
                'service': [
                    {
                        '@id': 'https://localhost/iiif-token.php',
                        'profile': 'http://iiif.io/api/auth/1/token'
                    }
                ]
            },
            'type': 'ImageService3',
            'width': 1200
        }

        response = manager.get_raw('auth/test_01.tif')
        assert response.status_code == 401
        assert expected_response == response.json()
