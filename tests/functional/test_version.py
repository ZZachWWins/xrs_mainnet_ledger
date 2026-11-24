from conftest import CLA, INS_GET_VERSION, SW_OK


def test_get_version(backend):
    response = backend.exchange(cla=CLA, ins=INS_GET_VERSION, p1=0, p2=0, data=b"")
    assert response.status == SW_OK

    data = response.data
    assert len(data) == 3

    major = data[0]
    minor = data[1]
    patch = data[2]
    assert major == 1
    assert minor == 0
    assert patch == 0


def test_get_version_wrong_cla(backend):
    response = backend.exchange_raw(bytes([0xFF, INS_GET_VERSION, 0x00, 0x00, 0x00]))
    assert response.status != SW_OK
