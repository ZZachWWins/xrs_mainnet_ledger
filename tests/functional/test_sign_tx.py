import struct
import hashlib
from conftest import (
    CLA,
    INS_SIGN_TX,
    P1_START,
    P1_MORE,
    P2_LAST,
    P2_MORE,
    SW_OK,
    SW_DENY,
    SW_TX_PARSING_FAIL,
    bip44_path_bytes,
)

SYSTEM_PROGRAM_ID = bytes(32)
SYSTEM_TRANSFER_IX = 2


def build_transfer_message(from_pubkey: bytes, to_pubkey: bytes, lamports: int) -> bytes:
    assert len(from_pubkey) == 32
    assert len(to_pubkey) == 32

    recent_blockhash = bytes.fromhex(
        "0000000000000000000000000000000000000000000000000000000000000001"
    )

    num_signatures = 1
    num_readonly_signed = 0
    num_readonly_unsigned = 1

    header = struct.pack("BBB", num_signatures, num_readonly_signed, num_readonly_unsigned)

    num_accounts = 3
    account_keys = from_pubkey + to_pubkey + SYSTEM_PROGRAM_ID

    ix_program_idx = 2
    ix_account_indices = bytes([0, 1])
    ix_data = struct.pack("<I", SYSTEM_TRANSFER_IX) + struct.pack("<Q", lamports)

    num_instructions = 1

    message = header
    message += encode_compact_u16(num_accounts)
    message += account_keys
    message += recent_blockhash
    message += encode_compact_u16(num_instructions)
    message += bytes([ix_program_idx])
    message += encode_compact_u16(len(ix_account_indices))
    message += ix_account_indices
    message += encode_compact_u16(len(ix_data))
    message += ix_data

    return message


def encode_compact_u16(value: int) -> bytes:
    if value < 0x80:
        return bytes([value])
    elif value < 0x4000:
        return bytes([
            (value & 0x7F) | 0x80,
            (value >> 7) & 0x7F,
        ])
    else:
        return bytes([
            (value & 0x7F) | 0x80,
            ((value >> 7) & 0x7F) | 0x80,
            (value >> 14) & 0x03,
        ])


def build_sign_tx_payload(message: bytes, account: int = 0) -> list:
    path_data = bip44_path_bytes(account=account)
    first_chunk = path_data + message

    max_chunk = 255
    chunks = []
    offset = 0
    while offset < len(first_chunk):
        end = min(offset + max_chunk, len(first_chunk))
        chunks.append(first_chunk[offset:end])
        offset = end

    return chunks


def test_sign_transfer_accepted(backend, firmware, navigator, test_name):
    from_key = bytes(range(32))
    to_key = bytes(range(32, 64))
    lamports = 1_000_000_000

    message = build_transfer_message(from_key, to_key, lamports)
    chunks = build_sign_tx_payload(message)

    for i, chunk in enumerate(chunks[:-1]):
        response = backend.exchange(
            cla=CLA, ins=INS_SIGN_TX, p1=P1_START if i == 0 else P1_MORE, p2=P2_MORE, data=chunk
        )
        assert response.status == SW_OK

    with backend.exchange_async(
        cla=CLA, ins=INS_SIGN_TX, p1=P1_MORE if len(chunks) > 1 else P1_START, p2=P2_LAST,
        data=chunks[-1]
    ) as exchange:
        if firmware.name in ("STAX", "FLEX"):
            navigator.navigate_until_text_and_compare(
                path=None,
                test_case_name=test_name,
                text="Sign",
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

    signature = response.data
    assert len(signature) == 64, f"Expected 64-byte Ed25519 signature, got {len(signature)}"


def test_sign_transfer_rejected(backend, firmware, navigator, test_name):
    from_key = bytes(range(32))
    to_key = bytes(range(32, 64))
    lamports = 500_000_000

    message = build_transfer_message(from_key, to_key, lamports)
    chunks = build_sign_tx_payload(message)

    for i, chunk in enumerate(chunks[:-1]):
        response = backend.exchange(
            cla=CLA, ins=INS_SIGN_TX, p1=P1_START if i == 0 else P1_MORE, p2=P2_MORE, data=chunk
        )
        assert response.status == SW_OK

    with backend.exchange_async(
        cla=CLA, ins=INS_SIGN_TX, p1=P1_MORE if len(chunks) > 1 else P1_START, p2=P2_LAST,
        data=chunks[-1]
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


def test_sign_empty_message(backend):
    path_data = bip44_path_bytes()
    response = backend.exchange(
        cla=CLA, ins=INS_SIGN_TX, p1=P1_START, p2=P2_LAST, data=path_data
    )
    assert response.status == SW_TX_PARSING_FAIL


def test_sign_malformed_message(backend):
    path_data = bip44_path_bytes()
    garbage = bytes([0xFF] * 10)
    response = backend.exchange(
        cla=CLA, ins=INS_SIGN_TX, p1=P1_START, p2=P2_LAST, data=path_data + garbage
    )
    assert response.status == SW_TX_PARSING_FAIL
