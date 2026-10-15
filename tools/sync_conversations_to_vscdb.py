#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Antigravity IDE Trajectory & Conversation Sync Utility
------------------------------------------------------
Synchronizes historical conversation databases from `.gemini/antigravity-ide/conversations`
into the Antigravity IDE global state database (`state.vscdb`), making all past conversations
visible and switchable in the IDE UI ("Search all convos..." popup).

Safety:
- Automatically creates a timestamped backup of `state.vscdb` before modification.
- Performs end-to-end roundtrip decode verification before committing.
"""

import os
import sys
import shutil
import sqlite3
import base64
import re
import datetime
from typing import List, Tuple, Dict, Optional, Any

# Ensure stdout handles UTF-8 / Asian characters gracefully on Windows
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass

# ============================================================================
# 1. Pure-Python Protobuf Wire-Format Codec
# ============================================================================

def encode_varint(val: int) -> bytes:
    """Encode an unsigned integer as a Protobuf Varint."""
    res = bytearray()
    while val > 0x7F:
        res.append((val & 0x7F) | 0x80)
        val >>= 7
    res.append(val & 0x7F)
    return bytes(res)

def encode_tag(field_num: int, wire_type: int) -> bytes:
    """Encode field number and wire type into a protobuf tag."""
    return encode_varint((field_num << 3) | wire_type)

def encode_length_delimited(field_num: int, data: Any) -> bytes:
    """Encode a length-delimited protobuf field (string or bytes)."""
    if isinstance(data, str):
        data = data.encode('utf-8')
    return encode_tag(field_num, 2) + encode_varint(len(data)) + data

def encode_varint_field(field_num: int, val: int) -> bytes:
    """Encode a varint protobuf field."""
    return encode_tag(field_num, 0) + encode_varint(val)

def encode_timestamp(sec: int, nano: int = 0) -> bytes:
    """Encode a google.protobuf.Timestamp message."""
    body = encode_varint_field(1, int(sec))
    if nano > 0:
        body += encode_varint_field(2, int(nano))
    return body

def decode_protobuf(data: bytes) -> List[Tuple[int, str, Any]]:
    """Decode raw bytes into a list of (field_number, wire_type_name, value)."""
    idx = 0
    length = len(data)
    fields = []
    while idx < length:
        tag_wire = 0
        shift = 0
        while True:
            if idx >= length:
                break
            b = data[idx]
            idx += 1
            tag_wire |= (b & 0x7F) << shift
            shift += 7
            if not (b & 0x80):
                break
        field_num = tag_wire >> 3
        wire_type = tag_wire & 0x07
        
        if wire_type == 0:  # Varint
            val = 0
            shift = 0
            while True:
                if idx >= length:
                    break
                b = data[idx]
                idx += 1
                val |= (b & 0x7F) << shift
                shift += 7
                if not (b & 0x80):
                    break
            fields.append((field_num, 'varint', val))
        elif wire_type == 1:  # 64-bit fixed
            val = data[idx:idx+8]
            idx += 8
            fields.append((field_num, '64bit', val))
        elif wire_type == 2:  # Length-delimited
            vlen = 0
            shift = 0
            while True:
                if idx >= length:
                    break
                b = data[idx]
                idx += 1
                vlen |= (b & 0x7F) << shift
                shift += 7
                if not (b & 0x80):
                    break
            val = data[idx:idx+vlen]
            idx += vlen
            fields.append((field_num, 'length_delimited', val))
        elif wire_type == 5:  # 32-bit fixed
            val = data[idx:idx+4]
            idx += 4
            fields.append((field_num, '32bit', val))
        else:
            # Corrupted or unsupported wire type
            break
    return fields


# ============================================================================
# 2. Conversation Metadata Extraction
# ============================================================================

def extract_conversation_details(db_path: str) -> Optional[Dict[str, Any]]:
    """
    Extract conversation parameters from a local conversation SQLite database.
    """
    if not os.path.exists(db_path):
        return None
        
    try:
        conn = sqlite3.connect(db_path)
        cur = conn.cursor()
        
        # 1. Trajectory Metadata
        cur.execute("SELECT trajectory_id, cascade_id FROM trajectory_meta LIMIT 1;")
        t_row = cur.fetchone()
        if not t_row:
            return None
        traj_id, cascade_id = t_row[0], t_row[1]
        
        # 2. Metadata Blob
        cur.execute("SELECT * FROM trajectory_metadata_blob LIMIT 1;")
        b_row = cur.fetchone()
        if not b_row:
            return None
        raw_blob = b_row[1]
        
        # Extract workspace URI info from blob (field 1)
        blob_sub = decode_protobuf(raw_blob)
        ws_bytes = b""
        for num, typ, v in blob_sub:
            if num == 1:
                ws_bytes = v
                break
                
        # 3. Steps & Title
        cur.execute("SELECT MAX(idx) FROM steps;")
        max_idx_row = cur.fetchone()
        max_idx = max_idx_row[0] if max_idx_row and max_idx_row[0] is not None else 0
        step_count = max_idx + 1
        
        # Find Title:
        title = ""
        # Priority 1: Step type 23 (Session title step) -> field 30 -> subfield 4
        cur.execute("SELECT step_payload FROM steps WHERE step_type = 23;")
        for p_row in cur.fetchall():
            if p_row[0]:
                f_list = decode_protobuf(p_row[0])
                for num, typ, v in f_list:
                    if num == 30 and typ == 'length_delimited':
                        sub_f = decode_protobuf(v)
                        for snum, styp, sv in sub_f:
                            if snum == 4 and styp == 'length_delimited':
                                try:
                                    s = sv.decode('utf-8').strip()
                                    if s:
                                        title = s
                                        break
                                except Exception:
                                    pass
                    if title:
                        break
            if title:
                break
                
        # Priority 2: Step type 14 (User input step) -> field 19 -> subfield 2
        if not title:
            cur.execute("SELECT step_payload FROM steps WHERE step_type = 14 LIMIT 1;")
            u_row = cur.fetchone()
            if u_row and u_row[0]:
                u_fields = decode_protobuf(u_row[0])
                for num, typ, v in u_fields:
                    if num == 19 and typ == 'length_delimited':
                        sub_f = decode_protobuf(v)
                        for snum, styp, sv in sub_f:
                            if snum == 2 and styp == 'length_delimited':
                                try:
                                    s = sv.decode('utf-8').strip()
                                    if len(s) > 35:
                                        s = s[:35] + "..."
                                    title = s
                                    break
                                except Exception:
                                    pass
                    if title:
                        break
                        
        if not title:
            title = f"CodonWeb Session ({cascade_id[:8]})"
            
        mtime = os.path.getmtime(db_path)
        
        return {
            "cascade_id": cascade_id,
            "traj_id": traj_id,
            "raw_blob": raw_blob,
            "ws_bytes": ws_bytes,
            "step_count": step_count,
            "title": title,
            "mtime": mtime,
            "db_path": db_path
        }
    except Exception as e:
        return None


# ============================================================================
# 3. TrajectorySummary Entry Builder
# ============================================================================

def build_summary_entry(details: Dict[str, Any]) -> bytes:
    """Build a serialized TrajectorySummaryEntry protobuf message."""
    title = details["title"]
    step_count = details["step_count"]
    mtime = details["mtime"]
    traj_id = details["traj_id"]
    ws_bytes = details["ws_bytes"]
    raw_blob = details["raw_blob"]
    cascade_id = details["cascade_id"]
    
    # 1. Inner Protobuf
    inner_raw = bytearray()
    inner_raw.extend(encode_length_delimited(1, title))
    inner_raw.extend(encode_varint_field(2, step_count))
    inner_raw.extend(encode_length_delimited(3, encode_timestamp(int(mtime))))
    inner_raw.extend(encode_length_delimited(4, traj_id))
    inner_raw.extend(encode_varint_field(5, 1))
    inner_raw.extend(encode_length_delimited(7, encode_timestamp(int(mtime - 1800))))
    if ws_bytes:
        inner_raw.extend(encode_length_delimited(9, ws_bytes))
    inner_raw.extend(encode_length_delimited(10, encode_timestamp(int(mtime))))
    inner_raw.extend(encode_length_delimited(15, ""))
    inner_raw.extend(encode_varint_field(16, min(step_count, 100)))
    inner_raw.extend(encode_length_delimited(17, raw_blob))
    inner_raw.extend(encode_varint_field(21, 1))
    inner_raw.extend(encode_varint_field(22, 4))
    
    inner_b64 = base64.b64encode(bytes(inner_raw)).decode('ascii')
    
    # 2. Middle Protobuf: Field 1 is inner_b64
    data_bytes = encode_length_delimited(1, inner_b64)
    
    # 3. Outer Protobuf: Field 1 = cascade_id, Field 2 = data_bytes
    entry = encode_length_delimited(1, cascade_id) + encode_length_delimited(2, data_bytes)
    return bytes(entry)


# ============================================================================
# 4. Synchronization Engine
# ============================================================================

def sync_conversations(
    vscdb_path: str,
    conv_dir: str,
    target_keyword: str = "codon_web",
    dry_run: bool = False
) -> bool:
    """Synchronize all conversations matching target_keyword into state.vscdb."""
    
    if not os.path.exists(vscdb_path):
        print(f"[ERROR] Target state.vscdb does not exist at: {vscdb_path}")
        return False
        
    if not os.path.exists(conv_dir):
        print(f"[ERROR] Conversations directory does not exist at: {conv_dir}")
        return False

    print("=" * 70)
    print(" Antigravity IDE Conversation Synchronizer")
    print("=" * 70)
    print(f"Target DB:     {vscdb_path}")
    print(f"Convos Path:   {conv_dir}")
    print(f"Workspace Tag: {target_keyword}")
    print(f"Dry Run:       {dry_run}")
    print("-" * 70)

    # 1. Connect and read existing trajectorySummaries
    conn = sqlite3.connect(vscdb_path)
    cur = conn.cursor()
    cur.execute("SELECT value FROM ItemTable WHERE key = 'antigravityUnifiedStateSync.trajectorySummaries';")
    row = cur.fetchone()
    
    existing_entries: Dict[str, bytes] = {}
    entry_mtimes: Dict[str, float] = {}
    
    if row and row[0]:
        try:
            raw_top = base64.b64decode(row[0])
            top_fields = decode_protobuf(raw_top)
            for fnum, ftyp, fval in top_fields:
                if fnum == 1 and ftyp == 'length_delimited':
                    sub = decode_protobuf(fval)
                    if len(sub) >= 2 and sub[0][0] == 1:
                        cid = sub[0][2].decode('utf-8', errors='ignore')
                        existing_entries[cid] = fval
                        # default mtime
                        entry_mtimes[cid] = 0.0
            print(f"[INFO] Existing trajectories in state.vscdb: {len(existing_entries)}")
        except Exception as e:
            print(f"[WARNING] Failed to parse existing trajectorySummaries: {e}")

    # 2. Scan local conversations directory for target conversations
    synced_count = 0
    all_target_cids = []
    
    for f in os.listdir(conv_dir):
        if not f.endswith('.db'):
            continue
        cid = f[:-3]
        db_path = os.path.join(conv_dir, f)
        
        # Check if conversation relates to target_keyword
        try:
            c_conn = sqlite3.connect(db_path)
            c_cur = c_conn.cursor()
            c_cur.execute("SELECT * FROM trajectory_metadata_blob LIMIT 1;")
            b_row = c_cur.fetchone()
            if not b_row or target_keyword.encode('utf-8') not in b_row[1]:
                continue
                
            details = extract_conversation_details(db_path)
            if not details:
                continue
                
            entry_bytes = build_summary_entry(details)
            existing_entries[cid] = entry_bytes
            entry_mtimes[cid] = details["mtime"]
            all_target_cids.append((details["mtime"], cid, details["title"], details["step_count"]))
            synced_count += 1
        except Exception as e:
            print(f"[WARN] Skipping {cid}: {e}")

    print(f"[INFO] Found {synced_count} matching '{target_keyword}' conversations.")
    
    # Sort target conversations by timestamp
    all_target_cids.sort(key=lambda x: x[0], reverse=True)
    print("\nSynchronized Records Summary:")
    print("-" * 70)
    for mtime, cid, title, steps in all_target_cids:
        dt_str = datetime.datetime.fromtimestamp(mtime).strftime('%Y-%m-%d %H:%M:%S')
        print(f" * [{dt_str}] ({steps:3d} steps) {title[:40]:<40} [ID: {cid[:8]}]")
    print("-" * 70)

    # 3. Sort all entries: newer first
    sorted_cids = sorted(existing_entries.keys(), key=lambda c: entry_mtimes.get(c, 0.0), reverse=True)
    
    # Build new top-level protobuf
    new_top_raw = bytearray()
    for cid in sorted_cids:
        entry_bytes = existing_entries[cid]
        new_top_raw.extend(encode_length_delimited(1, entry_bytes))
        
    new_b64 = base64.b64encode(bytes(new_top_raw)).decode('ascii')
    print(f"\n[INFO] Total combined entries: {len(sorted_cids)}")
    print(f"[INFO] New payload size: {len(new_b64)} chars (raw bytes: {len(new_top_raw)})")

    # 4. Verify decoding before writing
    verify_fields = decode_protobuf(bytes(new_top_raw))
    if len(verify_fields) != len(sorted_cids):
        print(f"[ERROR] Verification failed: expected {len(sorted_cids)} fields, got {len(verify_fields)}")
        return False
    print("[PASS] Protobuf serialization roundtrip verification succeeded.")

    if dry_run:
        print("[INFO] Dry run enabled. No changes made to state.vscdb.")
        return True

    # 5. Backup state.vscdb
    backup_path = vscdb_path + f".bak.{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}"
    try:
        shutil.copy2(vscdb_path, backup_path)
        print(f"[BACKUP] Successfully created backup at: {backup_path}")
    except Exception as e:
        print(f"[ERROR] Failed to create backup: {e}")
        return False

    # 6. Write to state.vscdb
    try:
        cur.execute("BEGIN TRANSACTION;")
        cur.execute(
            "UPDATE ItemTable SET value = ? WHERE key = 'antigravityUnifiedStateSync.trajectorySummaries';",
            (new_b64,)
        )
        conn.commit()
        print("[SUCCESS] Successfully updated 'antigravityUnifiedStateSync.trajectorySummaries' in state.vscdb!")
        print("\n" + "=" * 70)
        print(" ACTION REQUIRED:")
        print(" Please reload the Antigravity IDE window to load updated history:")
        print("   -> Press Ctrl+Shift+P (or F1)")
        print("   -> Type 'Developer: Reload Window' and press Enter")
        print("=" * 70)
        return True
    except Exception as e:
        conn.rollback()
        print(f"[ERROR] Failed to write to SQLite database: {e}")
        return False
    finally:
        conn.close()


# ============================================================================
# Main Entry Point
# ============================================================================

if __name__ == '__main__':
    vscdb = os.path.expandvars(r'%APPDATA%\Antigravity IDE\User\globalStorage\state.vscdb')
    conv_dir = os.path.expanduser(r'~/.gemini/antigravity-ide/conversations')
    
    dry_run = "--dry-run" in sys.argv
    success = sync_conversations(vscdb, conv_dir, target_keyword="codon_web", dry_run=dry_run)
    sys.exit(0 if success else 1)
