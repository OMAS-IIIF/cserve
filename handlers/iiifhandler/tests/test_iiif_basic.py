import pytest
import os
import pprint

def test_get_iiif(manager):
    """
    Test getting an image file using IIIF syntax

    :param manager: Test manager from conftest.py
    :return: None
    """
    response = manager.get('iiif/test_01.tif/full/max/0/default.jpg')
    assert response.status_code == 200
