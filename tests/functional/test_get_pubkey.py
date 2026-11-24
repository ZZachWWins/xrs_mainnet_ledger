import struct
from conftest import (
    CLA,
    INS_GET_PUBKEY,
    P1_NON_CONFIRM,
    P1_CONFIRM,
    SW_OK,
    SW_DENY,
    bip44_path_bytes,
)


def test_get_pubkey_no_confirm(backend):
    path_data = bip44_path_bytes(account=0)
    response = backend.exchange(
        cla=CLA, ins=INS_GET_PUBKEY, p1=P1_NON_CONFIRM, p2=0, data=path_data
    )
    assert response.status == SW_OK

    data = response.data
    assert len(data) == 32, f"Expected 32-byte Ed25519 public key, got {len(data)}"


def test_get_pubkey_confirm_accepted(backend, firmware, navigator, test_name):
    path_data = bip44_path_bytes(account=0)

    with backend.exchange_async(
        cla=CLA, ins=INS_GET_PUBKEY, p1=P1_CONFIRM, p2=0, data=path_data
    ) as exchange:
        if firmware.name in ("STAX", "FLEX"):
            navigator.navigate_until_text_and_compare(
                path=None,
                test_case_name=test_name,
                text="Confirm",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                path=None,
                test_case_name=test_name,
                text="Approve",
                screen_change_before_first_instruction=True,
            )

    response = exchange.result
    assert response.status == SW_OK
    assert len(response.data) == 32


def test_get_pubkey_confirm_rejected(backend, firmware, navigator, test_name):
    path_data = bip44_path_bytes(account=0)

    with backend.exchange_async(
        cla=CLA, ins=INS_GET_PUBKEY, p1=P1_CONFIRM, p2=0, data=path_data
    ) as exchange:
        if firmware.name in ("STAX", "FLEX"):
            navigator.navigate_until_text_and_compare(
                path=None,
                test_case_name=test_name,
                text="Reject",
                screen_change_before_first_instruction=True,
            )
        else:
            navigator.navigate_until_text_and_compare(
                path=None,
                test_case_name=test_name,
                text="Reject",
                screen_change_before_first_instruction=True,
            )

    response = exchange.result
    assert response.status == SW_DENY


def test_get_pubkey_different_accounts(backend):
    keys = set()
    for account in range(3):
        path_data = bip44_path_bytes(account=account)
        response = backend.exchange(
            cla=CLA, ins=INS_GET_PUBKEY, p1=P1_NON_CONFIRM, p2=0, data=path_data
        )
        assert response.status == SW_OK
        keys.add(response.data.hex())

    assert len(keys) == 3, "Different accounts should derive different keys"
