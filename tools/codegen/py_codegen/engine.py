"""
Template Rendering and Code Generation Engine for CodonWeb Schemas.
Uses Jinja2 declarative templates to eliminate hundreds of lines of string concatenation.
"""
import os
import json
from typing import List, Dict
import jinja2

from tools.codegen.py_codegen.models import ClassMeta, FieldMeta


TEMPLATE_DIR = os.path.join(os.path.dirname(__file__), "templates")


def compute_source_checksum(clean_src: str) -> str:
    """Computes deterministic 64-bit FNV-1a checksum matching CodonWeb runtime."""
    h = 14695981039346656037
    fnv_prime = 1099511628211
    mask = 0xFFFFFFFFFFFFFFFF
    for ch in clean_src:
        h = ((h ^ ord(ch)) * fnv_prime) & mask
    if h >= 0x8000000000000000:
        signed_h = h - 0x10000000000000000
        return f"{signed_h:x}"
    return f"{h:x}"


def extract_user_source(src_text: str) -> str:
    """Extracts purely user-authored source code, stripping generated sections."""
    auto_marker = "# ==================== AUTO-GENERATED METHODS"
    m_idx = src_text.find(auto_marker)
    clean = src_text[:m_idx] if m_idx >= 0 else src_text
    return clean.rstrip()


def extract_existing_checksum(src_text: str) -> str:
    marker = "# CodeGen-Checksum: "
    idx = src_text.find(marker)
    if idx < 0:
        return ""
    start = idx + len(marker)
    line = src_text[start:].split("\n")[0].strip()
    return line


def generate_openapi_json(cls: ClassMeta) -> str:
    """Builds standard OpenAPI 3.0 Schema Object dictionary and serializes to JSON."""
    props: Dict[str, dict] = {}
    reqs: List[str] = []

    for f in cls.fields:
        prop_key = f.alias if f.alias else f.name
        if f.required:
            reqs.append(prop_key)

        p_meta: dict = {}
        if f.is_list:
            p_meta["type"] = "array"
            if f.min_items > 0:
                p_meta["minItems"] = f.min_items
            if f.max_items > 0:
                p_meta["maxItems"] = f.max_items
            if f.list_item_type == "int":
                p_meta["items"] = {"type": "integer"}
            elif f.list_item_type == "float":
                p_meta["items"] = {"type": "number"}
            elif f.list_item_type == "bool":
                p_meta["items"] = {"type": "boolean"}
            elif f.is_submodel:
                p_meta["items"] = {"type": "object", "title": f.list_item_type}
            else:
                p_meta["items"] = {"type": "string"}
        elif f.is_submodel:
            p_meta["type"] = "object"
            p_meta["title"] = f.submodel_name
        elif f.is_int:
            p_meta["type"] = "integer"
            if f.has_ge:
                p_meta["minimum"] = f.ge
            if f.has_le:
                p_meta["maximum"] = f.le
            if f.has_gt:
                p_meta["exclusiveMinimum"] = f.gt
            if f.has_lt:
                p_meta["exclusiveMaximum"] = f.lt
            if f.has_default:
                try:
                    p_meta["default"] = int(f.default_val)
                except Exception:
                    pass
        elif f.is_float:
            p_meta["type"] = "number"
            if f.has_ge_f:
                p_meta["minimum"] = f.ge_f
            if f.has_le_f:
                p_meta["maximum"] = f.le_f
            if f.has_gt_f:
                p_meta["exclusiveMinimum"] = f.gt_f
            if f.has_lt_f:
                p_meta["exclusiveMaximum"] = f.lt_f
            if f.has_default:
                try:
                    p_meta["default"] = float(f.default_val)
                except Exception:
                    pass
        elif f.is_bool:
            p_meta["type"] = "boolean"
            if f.has_default:
                p_meta["default"] = f.default_val.lower() == "true"
        else: # str
            p_meta["type"] = "string"
            if f.min_length > 0:
                p_meta["minLength"] = f.min_length
            if f.max_length > 0:
                p_meta["maxLength"] = f.max_length
            if f.format:
                p_meta["format"] = f.format
            if f.pattern:
                p_meta["pattern"] = f.pattern
            if f.choices:
                p_meta["enum"] = f.choices
            if f.has_default:
                val = f.default_val
                if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                    val = val[1:-1]
                p_meta["default"] = val

        if f.description:
            p_meta["description"] = f.description

        props[prop_key] = p_meta

    schema_obj: dict = {
        "type": "object",
        "title": cls.name,
        "properties": props
    }
    if cls.docstring:
        schema_obj["description"] = cls.docstring
    if reqs:
        schema_obj["required"] = reqs

    return json.dumps(schema_obj, separators=(",", ":"), ensure_ascii=False)


class CodeGenerator:
    def __init__(self, template_dir: str = TEMPLATE_DIR):
        self.env = jinja2.Environment(
            loader=jinja2.FileSystemLoader(template_dir),
            trim_blocks=True,
            lstrip_blocks=True,
            autoescape=False
        )
        self.template = self.env.get_template("model_methods.codon.j2")

    def render_extend_block(self, classes: List[ClassMeta]) -> str:
        openapi_map = {c.name: generate_openapi_json(c).replace("'", "\\'") for c in classes}
        return self.template.render(classes=classes, openapi_schemas=openapi_map)

    def generate_in_place_content(self, original_src: str, classes: List[ClassMeta]) -> str:
        clean_src = extract_user_source(original_src)
        checksum = compute_source_checksum(clean_src)

        rendered_methods = self.render_extend_block(classes)

        sections = [
            clean_src,
            "",
            "",
            "# ==================== AUTO-GENERATED METHODS (DO NOT EDIT) ====================",
            "# Generated by CodonWeb Modern Jinja2 Schema Generator. DO NOT EDIT DIRECTLY.",
            f"# CodeGen-Checksum: {checksum}",
            "# Supports IDE code folding. All modifications above remain 100% preserved.",
            "import re",
            "from src.http.fast_json import FastJson, FastJsonVal, FastJsonDoc, FastJsonWriter",
            "from src.memory.buffer import ByteBuffer",
            "from src.schema.errors import ValidationError, ValidationResult",
            "from src.schema.validator import (",
            "    extract_str_field, extract_opt_str_field,",
            "    extract_int_field, extract_opt_int_field,",
            "    extract_float_field, extract_opt_float_field,",
            "    extract_bool_field, extract_opt_bool_field,",
            "    extract_dict_str, extract_dict_opt_str,",
            "    extract_dict_int, extract_dict_opt_int,",
            "    extract_dict_float, extract_dict_opt_float,",
            "    extract_dict_bool, extract_dict_opt_bool,",
            "    parse_json_payload",
            ")",
            "",
            rendered_methods.strip(),
            "",
            "# =================== END AUTO-GENERATED METHODS ====================",
            ""
        ]
        return "\n".join(sections)
