import shutil

import pytest
import os
import pprint

class TestBasic:
    component = "Pyramidal TIFF's testing"

    def test_cserver_running(self, manager):
        """test if cserver is running...."""
        assert manager.cserver_ready

    def test_pyramidal_basic_a(self, manager):
        """test if pyramidal TIFF can be delivered"""
        response = manager.get('tiff_01_rgb_pyramid.tif/full/max/0/default.jpg')
        assert response.status_code == 200

    def test_pyramidal_basic_b(self, manager):
        """test pyramidal TIFF full size"""
        assert manager.compare_iiif_images('tiff_01_rgb_pyramid.tif/full/max/0/default.tif', "data/tiff_01_rgb_uncompressed.tif")

    def test_pyramidal_tiles(self, manager):
        """test the left upper tile"""
        assert manager.compare_iiif_images('tiff_01_rgb_pyramid.tif/0,0,512,512/256,256/0/default.tif', "data/tiff_01_rgb_pyramid_res04.tif")
