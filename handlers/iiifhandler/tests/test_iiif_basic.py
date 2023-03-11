import pytest
import os
import pprint

class TestServer:
    component = "The IIIF server"

    def test_cserver_running(self, manager):
        """test if cserver is running...."""
        assert manager.cserver_ready

    def test_iiif_parsing(self, manager):
        """return 400 for invalid IIIF URL's"""
        # assert manager.get_status_code("iiif//lena512.jp2") == 400 # TODO!!!!
        #assert manager.get_status_code("iiif/lena512.jp2/max/0/default.jpg") == 400
        #assert manager.get_status_code("iiif/lena512.jp2/full/max/default.jpg") == 400
        #assert manager.get_status_code("iiif/lena512.jp2/full/max/!/default.jpg") == 400
        #assert manager.get_status_code("iiif/lena512.jp2/full/max/0/jpg") == 400

    def test_get_iiif(self, manager):
        """get an image file using IIIF syntax"""
        response = manager.get('iiif/test_01.tif/full/max/0/default.jpg')
        assert response.status_code == 200

    def test_iiif_bytes(self, manager):
        """return an unmodified JPG file"""
        assert manager.compare_iiif_bytes("iiif/Leaves.jpg/full/max/0/default.jpg",
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
            'id': 'http://localhost:8080//iiif/test_01.tif',
            'preferredFormats': ['jpg', 'tif', 'jp2', 'png'],
            'profile': 'level2',
            'protocol': 'http://iiif.io/api/image',
            'sizes': [
                {'height': 400, 'width': 600},
                {'height': 200, 'width': 300}
            ],
            'type': 'ImageService3',
            'width': 1200
        }
        response = manager.get("iiif/test_01.tif")
        assert response.json() == expected_response

    def test_iiif_restricted(self, manager):
        """return restricted size"""
        image_info = manager.get_image_info(
            "iiif/RestrictLeaves.jpg/full/max/0/default.jpg")

        page_geometry = [line.strip().split()[-1] for line in image_info.splitlines()
                         if line.strip().startswith("Page geometry:")][0]
        assert page_geometry == "128x128+0+0"

    def test_iiif_deny(self, manager):
        """return status 401"""
        status_code = manager.get_status_code('iiif/DenyLeaves.jpg/full/max/0/default.jpg')
        assert status_code == 401

    def test_file_allow(self, manager):
        """get file from IIIF directory"""
        iiifurl = "iiif/{}/file".format("_test.csv")
        assert manager.compare_iiif_bytes("iiif/test_allow.csv/file", manager.iiif_imgroot_path("test_allow.csv"))

    def test_file_deny(self, manager):
        """deny get file from IIIF directory"""
        status_code = manager.get_status_code('iiif/test_deny.csv/file')
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
        difference = manager.compare_iiif_images("iiif/_IMG_8207.jp2/full/max/0/default.tif", "./data/IMG_8207.tiff", "PAE")
        assert difference == 0

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
        iiifurl = "iiif/{}/file".format("_test.csv")
        assert manager.compare_iiif_bytes(iiifurl, "./data/test.csv")
