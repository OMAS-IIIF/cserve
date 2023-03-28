
class TestBasic:
    component = "The IIIF server"

    def test_iiif_validation(self, manager):
        """pass the IIIF validator's tests"""
        manager.run_iiif_validator()
