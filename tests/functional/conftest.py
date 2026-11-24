import pytest
from pathlib import Path
from ragger.backend import SpeculosBackend, LedgerCommBackend, LedgerWalletBackend
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID

XERIS_APP_DIR = Path(__file__).resolve().parents[2] / "app"

FIRMWARE_PARAMS = {
    Firmware.NANOS: {
        "app": str(XERIS_APP_DIR / "bin" / "app.elf"),
    },
    Firmware.NANOX: {
        "app": str(XERIS_APP_DIR / "bin" / "app.elf"),
    },
    Firmware.NANOSP: {
        "app": str(XERIS_APP_DIR / "bin" / "app.elf"),
    },
    Firmware.STAX: {
        "app": str(XERIS_APP_DIR / "bin" / "app.elf"),
    },
    Firmware.FLEX: {
        "app": str(XERIS_APP_DIR / "bin" / "app.elf"),
    },
}

CLA = 0xE0

INS_GET_VERSION = 0x00
INS_GET_PUBKEY = 0x01
INS_SIGN_TX = 0x02
INS_SIGN_MESSAGE = 0x03

P1_CONFIRM = 0x01
P1_NON_CONFIRM = 0x00
P1_START = 0x00
P1_MORE = 0x80

P2_LAST = 0x00
P2_MORE = 0x80

SW_OK = 0x9000
SW_DENY = 0x6985
SW_WRONG_P1P2 = 0x6A86
SW_INS_NOT_SUPPORTED = 0x6D00
SW_CLA_NOT_SUPPORTED = 0x6E00
SW_BAD_STATE = 0xB007
SW_TX_PARSING_FAIL = 0xB008
SW_TX_HASH_FAIL = 0xB009
SW_BAD_LENGTH = 0x6983


def pytest_addoption(parser):
    parser.addoption(
        "--firmware",
        type=str,
        default="nanos",
        choices=["nanos", "nanox", "nanosp", "stax", "flex"],
        help="Firmware to test",
    )
    parser.addoption(
        "--backend",
        type=str,
        default="speculos",
        choices=["speculos", "ledgercomm", "ledgerwallet"],
        help="Backend to use for tests",
    )
    parser.addoption(
        "--display",
        action="store_true",
        default=False,
        help="Display Speculos window",
    )


@pytest.fixture(scope="session")
def firmware(pytestconfig) -> Firmware:
    fw = pytestconfig.getoption("firmware")
    return {
        "nanos": Firmware.NANOS,
        "nanox": Firmware.NANOX,
        "nanosp": Firmware.NANOSP,
        "stax": Firmware.STAX,
        "flex": Firmware.FLEX,
    }[fw]


@pytest.fixture(scope="session")
def display(pytestconfig) -> bool:
    return pytestconfig.getoption("display")


@pytest.fixture
def backend(firmware, display):
    params = FIRMWARE_PARAMS[firmware]
    args = ["--model", firmware.name.lower(), "--sdk", "1.0"]
    if display:
        args.append("--display")
        args.append("qt")

    with SpeculosBackend(params["app"], firmware, args=args) as b:
        yield b


@pytest.fixture
def navigator(backend, firmware):
    return Navigator(backend, firmware)


@pytest.fixture
def test_name(request):
    return request.node.name


def bip44_path_bytes(account=0, change=0) -> bytes:
    path = [
        0x8000002C,  # 44'
        0x80002328,  # 9000'
        0x80000000 | account,  # account'
        change,
    ]
    result = len(path).to_bytes(1, "big")
    for element in path:
        result += element.to_bytes(4, "big")
    return result
