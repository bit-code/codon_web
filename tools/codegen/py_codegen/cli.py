#!/usr/bin/env python3
"""
Enterprise-Grade Python + Jinja2 Schema CodeGen CLI for CodonWeb.
Replaces brittle manual string parsers with official Python AST & Declarative Templates.
"""
import sys
import os
import argparse
from typing import List, Tuple

# Add repository root to pythonpath
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

from tools.codegen.py_codegen.ast_parser import parse_schemas_from_source
from tools.codegen.py_codegen.engine import (
    CodeGenerator, compute_source_checksum,
    extract_existing_checksum, extract_user_source
)


def process_in_place(file_path: str, generator: CodeGenerator, force: bool = False, check_only: bool = False) -> Tuple[bool, int]:
    """Processes a single schema file in-place with incremental caching.
    Returns (is_cached, class_count).
    """
    if not os.path.exists(file_path):
        print(f"[CodeGen Error] File not found: {file_path}")
        return False, 0

    with open(file_path, "r", encoding="utf-8") as f:
        src_text = f.read()

    clean_src = extract_user_source(src_text)
    curr_checksum = compute_source_checksum(clean_src)
    existing_checksum = extract_existing_checksum(src_text)

    # Incremental check
    if not force and existing_checksum and existing_checksum == curr_checksum:
        print(f"[CodeGen CACHED] {file_path} (up to date, checksum: {curr_checksum})")
        return True, 1

    classes = parse_schemas_from_source(src_text)
    if not classes:
        print(f"[CodeGen Skip] No schema classes detected in: {file_path}")
        return False, 0

    new_content = generator.generate_in_place_content(src_text, classes)

    if check_only:
        if new_content != src_text:
            print(f"[CodeGen Check FAIL] Outdated schema code in: {file_path}")
            return False, len(classes)
        return True, len(classes)

    # Protect mtime if byte-identical
    if not force and new_content == src_text:
        print(f"[CodeGen CACHED] {file_path} (byte-identical, preserved mtime)")
        return True, len(classes)

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    names = ", ".join(c.name for c in classes)
    print(f"[CodeGen UPDATED] Jinja2 rendered @extend for {len(classes)} classes ({names}) in {file_path} (checksum: {curr_checksum})")
    return False, len(classes)


def main():
    parser = argparse.ArgumentParser(
        description="CodonWeb Modern Schema Generator powered by Python AST and Jinja2 Templates."
    )
    parser.add_argument(
        "--in-place", action="store_true", default=True,
        help="Inject generated @extend methods in-place into source files."
    )
    parser.add_argument(
        "--force", "-f", action="store_true",
        help="Force code re-generation bypassing checksum cache."
    )
    parser.add_argument(
        "--check", action="store_true",
        help="CI Check mode: exits with non-zero code if generated code is outdated."
    )
    parser.add_argument(
        "files", nargs="+",
        help="Path to .codon schema files to process."
    )

    args = parser.parse_args()

    generator = CodeGenerator()
    cached = 0
    updated = 0
    total_models = 0
    failed_check = False

    for file_path in args.files:
        is_cached, n_cls = process_in_place(file_path, generator, force=args.force, check_only=args.check)
        if n_cls > 0:
            if is_cached:
                cached += 1
            else:
                updated += 1
                if args.check:
                    failed_check = True
            total_models += n_cls

    if args.check:
        if failed_check:
            print("\n[CodeGen Check FAILED] Schemas are out of date! Run 'make codegen' to synchronize.")
            sys.exit(1)
        print(f"\n[CodeGen Check PASSED] All {total_models} models across {cached} files are up to date.")
        return

    print(f"\n[CodeGen Summary] Jinja2 Generator: {cached} cached, {updated} updated ({total_models} models).")


if __name__ == "__main__":
    main()
